#!/usr/bin/env python3
"""
Batch iLevel Calculator — populates `calculated_ilevel` column on the items table.

This script:
  1. Ensures the `calculated_ilevel` column exists (adds it + index if not).
  2. Reads all items from the database.
  3. Calculates iLevel using the same formula as C++ (common/item_ilevel.cpp).
  4. Writes `calculated_ilevel` back to each row.
  5. Prints a summary report.

Algorithm (from ITEM_LEVEL_SYSTEM.md / common/item_ilevel.cpp):
  Weapons: DPS + AC×W_AC + HP×W_HP + Mana×W_MANA + Attrs×W_ATTR
           + Attack + Haste×W_HASTE + Heroics×W_HEROIC
  Armor:   AC×A_AC + HP×A_HP + Mana×A_MANA + Attrs×A_ATTR
           + Resists×A_RESIST + Heroics×A_HEROIC
  iLevel = max(1, floor(power_score))

Weights match the server rule defaults in common/ruletypes.h RULE_CATEGORY(ItemProgression).
Override weights with --config <json> (see --help).

Usage:
  python tools/populate_ilevel.py                 # full run
  python tools/populate_ilevel.py --dry-run       # calculate but don't write
  python tools/populate_ilevel.py --report-only   # just show distribution stats
  python tools/populate_ilevel.py --verify        # compare against existing values

Run from repo root:
  python tools/populate_ilevel.py
"""

import argparse
import json
import math
import os
import sys
import time

try:
    import mysql.connector
except ImportError:
    print("ERROR: mysql-connector-python not installed.")
    print("  pip install mysql-connector-python")
    sys.exit(1)


# ── Defaults matching RULE_CATEGORY(ItemProgression) in ruletypes.h ───

DEFAULT_WEIGHTS = {
    # Weapon formula weights
    "WeaponACWeight":      3.0,
    "WeaponHPWeight":      0.3,
    "WeaponManaWeight":    0.3,
    "WeaponAttrWeight":    2.0,
    "WeaponHasteWeight":   10.0,
    "WeaponHeroicWeight":  5.0,
    # Armor formula weights
    "ArmorACWeight":       15.0,
    "ArmorHPWeight":       0.5,
    "ArmorManaWeight":     0.5,
    "ArmorAttrWeight":     3.0,
    "ArmorResistWeight":   0.5,
    "ArmorHeroicWeight":   5.0,
}

# Weapon item types (must match common/item_ilevel.cpp IsWeaponType)
WEAPON_TYPES = {0, 1, 2, 3, 4, 5, 35, 45}
# 0=1HSlash, 1=2HSlash, 2=1HPiercing, 3=1HBlunt, 4=2HBlunt, 5=Bow, 35=2HPiercing, 45=Martial

STAT_ATTRS   = ['astr', 'asta', 'aagi', 'adex', 'awis', 'aint', 'acha']
STAT_HEROICS = ['heroic_str', 'heroic_sta', 'heroic_agi', 'heroic_dex',
                'heroic_wis', 'heroic_int', 'heroic_cha']
STAT_RESISTS = ['fr', 'cr', 'mr', 'dr', 'pr']


# ── DB Connection ────────────────────────────────────────────────────────

def get_connection():
    config_path = os.path.join(os.path.dirname(__file__), '..', 'eqemu_config.json')
    if not os.path.exists(config_path):
        print(f"ERROR: Cannot find {config_path}")
        sys.exit(1)

    with open(config_path) as f:
        config = json.load(f)
    db_cfg = config['server']['database']

    return mysql.connector.connect(
        host=db_cfg['host'],
        port=int(db_cfg.get('port', 3306)),
        user=db_cfg['username'],
        password=db_cfg['password'],
        database=db_cfg['db'],
    )


# ── Schema Migration ─────────────────────────────────────────────────────

def ensure_column(conn):
    """Add calculated_ilevel column and index if they don't exist."""
    cursor = conn.cursor()

    # Check if column exists
    cursor.execute("""
        SELECT COUNT(*) FROM information_schema.COLUMNS
        WHERE TABLE_SCHEMA = DATABASE()
          AND TABLE_NAME = 'items'
          AND COLUMN_NAME = 'calculated_ilevel'
    """)
    col_exists = cursor.fetchone()[0] > 0

    if not col_exists:
        print("  Adding column: items.calculated_ilevel INT NOT NULL DEFAULT 0 ...")
        cursor.execute("ALTER TABLE items ADD COLUMN calculated_ilevel INT NOT NULL DEFAULT 0")
        conn.commit()
        print("  Column added.")
    else:
        print("  Column items.calculated_ilevel already exists.")

    # Check if index exists
    cursor.execute("""
        SELECT COUNT(*) FROM information_schema.STATISTICS
        WHERE TABLE_SCHEMA = DATABASE()
          AND TABLE_NAME = 'items'
          AND INDEX_NAME = 'idx_calculated_ilevel'
    """)
    idx_exists = cursor.fetchone()[0] > 0

    if not idx_exists:
        print("  Adding index: idx_calculated_ilevel ...")
        cursor.execute("CREATE INDEX idx_calculated_ilevel ON items(calculated_ilevel)")
        conn.commit()
        print("  Index added.")
    else:
        print("  Index idx_calculated_ilevel already exists.")

    cursor.close()


# ── iLevel Calculation ───────────────────────────────────────────────────

def _s(item, key, default=0):
    v = item.get(key, default)
    return v if v is not None else default


def calc_ilevel(item, weights):
    """Calculate iLevel for a single item dict. Returns (ilevel, category_str)."""
    item_type = _s(item, 'itemtype', 10)
    attrs   = sum(_s(item, a) for a in STAT_ATTRS)
    heroics = sum(_s(item, h) for h in STAT_HEROICS)

    if item_type in WEAPON_TYPES:
        delay = max(1, _s(item, 'delay', 1))
        dps = (_s(item, 'damage') * 100.0) / delay
        power = (dps
                 + _s(item, 'ac')   * weights['WeaponACWeight']
                 + _s(item, 'hp')   * weights['WeaponHPWeight']
                 + _s(item, 'mana') * weights['WeaponManaWeight']
                 + attrs            * weights['WeaponAttrWeight']
                 + _s(item, 'attack')
                 + _s(item, 'haste') * weights['WeaponHasteWeight']
                 + heroics          * weights['WeaponHeroicWeight'])
        return max(1, int(math.floor(power))), 'WPN'
    else:
        resists = sum(_s(item, r) for r in STAT_RESISTS)
        power = (_s(item, 'ac')   * weights['ArmorACWeight']
                 + _s(item, 'hp')   * weights['ArmorHPWeight']
                 + _s(item, 'mana') * weights['ArmorManaWeight']
                 + attrs            * weights['ArmorAttrWeight']
                 + resists          * weights['ArmorResistWeight']
                 + heroics          * weights['ArmorHeroicWeight'])
        return max(1, int(math.floor(power))), 'ARM'


# ── Batch Process ────────────────────────────────────────────────────────

ITEM_COLUMNS = """
    id, Name, itemtype, reclevel, magic,
    damage, delay, ac, hp, mana,
    astr, asta, aagi, adex, awis, aint, acha,
    attack, haste,
    heroic_str, heroic_sta, heroic_agi, heroic_dex,
    heroic_wis, heroic_int, heroic_cha,
    fr, cr, mr, dr, pr
"""


def fetch_all_items(conn):
    cursor = conn.cursor(dictionary=True)
    cursor.execute(f"SELECT {ITEM_COLUMNS} FROM items WHERE id < 1000000000 ORDER BY id")
    items = cursor.fetchall()
    cursor.close()
    return items


def batch_update(conn, results, dry_run=False):
    """Write calculated_ilevel to DB. Returns count updated."""
    if dry_run:
        return 0

    cursor = conn.cursor()
    batch_size = 500
    updated = 0

    for i in range(0, len(results), batch_size):
        batch = results[i:i + batch_size]
        # Build a single UPDATE with CASE for efficiency
        cases = " ".join(f"WHEN {r[0]} THEN {r[1]}" for r in batch)
        ids = ",".join(str(r[0]) for r in batch)
        sql = f"UPDATE items SET calculated_ilevel = CASE id {cases} END WHERE id IN ({ids})"
        cursor.execute(sql)
        updated += len(batch)

    conn.commit()
    cursor.close()
    return updated


def print_distribution(results):
    """Print iLevel distribution histogram."""
    buckets = {}
    for _, ilevel, _ in results:
        bucket = (ilevel // 100) * 100
        buckets[bucket] = buckets.get(bucket, 0) + 1

    if not buckets:
        print("  No items to report.")
        return

    print(f"\n  {'iLevel Range':<20} {'Count':>7}  Bar")
    print(f"  {'-'*20} {'-'*7}  {'-'*40}")
    max_count = max(buckets.values())
    for b in sorted(buckets.keys()):
        label = f"{b}-{b + 99}"
        bar_len = int(40 * buckets[b] / max_count)
        print(f"  {label:<20} {buckets[b]:>7}  {'#' * bar_len}")


def print_summary_stats(results):
    """Print summary statistics."""
    if not results:
        return

    ilevels = [r[1] for r in results]
    wpns = [r for r in results if r[2] == 'WPN']
    arms = [r for r in results if r[2] == 'ARM']

    total = len(ilevels)
    avg = sum(ilevels) / total
    med = sorted(ilevels)[total // 2]
    mn, mx = min(ilevels), max(ilevels)

    print(f"\n  Total items:    {total:>8,}")
    print(f"  Weapons:        {len(wpns):>8,}")
    print(f"  Armor/Other:    {len(arms):>8,}")
    print(f"  iLevel range:   {mn} - {mx}")
    print(f"  Average iLevel: {avg:.0f}")
    print(f"  Median iLevel:  {med}")

    # Top 10
    top = sorted(results, key=lambda r: r[1], reverse=True)[:10]
    print(f"\n  Top 10 iLevel items:")
    print(f"  {'ID':<8} {'iLevel':>7} {'Cat':<4} Name")
    print(f"  {'-'*8} {'-'*7} {'-'*4} {'-'*30}")
    # Need names — re-query or store in results
    # We don't have names in results tuple, but we'll add them


def print_top_items(items, results, n=15):
    """Print top-N items by iLevel."""
    # Build lookup: id -> item
    by_id = {item['id']: item for item in items}
    top = sorted(results, key=lambda r: r[1], reverse=True)[:n]

    print(f"\n  Top {n} items by iLevel:")
    print(f"  {'ID':<8} {'Name':<40} {'Cat':<4} {'RecLvl':>6} {'Magic':>5} {'iLevel':>7}")
    print(f"  {'-'*8} {'-'*40} {'-'*4} {'-'*6} {'-'*5} {'-'*7}")
    for item_id, ilevel, cat in top:
        item = by_id.get(item_id, {})
        name = str(item.get('Name', '?'))[:39]
        rec = _s(item, 'reclevel')
        mag = 'Y' if _s(item, 'magic') else 'N'
        print(f"  {item_id:<8} {name:<40} {cat:<4} {rec:>6} {mag:>5} {ilevel:>7}")


# ── Main ─────────────────────────────────────────────────────────────────

def main():
    parser = argparse.ArgumentParser(
        description="Batch calculate and populate calculated_ilevel on the items table."
    )
    parser.add_argument('--dry-run', action='store_true',
                        help="Calculate iLevels but do not write to DB.")
    parser.add_argument('--report-only', action='store_true',
                        help="Show distribution of existing calculated_ilevel values (no recalc).")
    parser.add_argument('--verify', action='store_true',
                        help="Recalculate and compare against existing DB values.")
    parser.add_argument('--config', type=str, default=None,
                        help="JSON file with weight overrides (keys match ItemProgression rule names).")
    parser.add_argument('--top', type=int, default=15,
                        help="Number of top items to show (default: 15).")
    parser.add_argument('--no-migrate', action='store_true',
                        help="Skip column/index migration check.")
    args = parser.parse_args()

    # Load weights
    weights = dict(DEFAULT_WEIGHTS)
    if args.config:
        with open(args.config) as f:
            overrides = json.load(f)
        weights.update(overrides)
        print(f"Loaded weight overrides from {args.config}")

    w = 80
    print("=" * w)
    print("iLevel Batch Calculator — populate_ilevel.py")
    print("=" * w)

    # Connect
    conn = get_connection()
    print(f"Connected to database.")

    # Ensure schema
    if not args.no_migrate:
        ensure_column(conn)

    if args.report_only:
        # Just read existing values
        cursor = conn.cursor(dictionary=True)
        cursor.execute("""
            SELECT calculated_ilevel as il, COUNT(*) as cnt
            FROM items
            WHERE id < 1000000000
            GROUP BY calculated_ilevel
            ORDER BY calculated_ilevel
        """)
        rows = cursor.fetchall()
        cursor.close()

        if not rows or all(r['il'] == 0 for r in rows):
            print("\n  No calculated_ilevel values found. Run without --report-only first.")
        else:
            buckets = {}
            total = 0
            for r in rows:
                bucket = (r['il'] // 100) * 100
                buckets[bucket] = buckets.get(bucket, 0) + r['cnt']
                total += r['cnt']

            print(f"\n  Existing calculated_ilevel distribution ({total:,} items):")
            print(f"  {'iLevel Range':<20} {'Count':>7}  Bar")
            print(f"  {'-'*20} {'-'*7}  {'-'*40}")
            max_count = max(buckets.values())
            for b in sorted(buckets.keys()):
                label = f"{b}-{b + 99}"
                bar_len = int(40 * buckets[b] / max_count)
                print(f"  {label:<20} {buckets[b]:>7}  {'#' * bar_len}")

        conn.close()
        return

    # Fetch all items
    print(f"\nFetching items...")
    items = fetch_all_items(conn)
    print(f"  Loaded {len(items):,} items.")

    # Calculate
    print(f"Calculating iLevels...")
    t0 = time.time()
    results = []  # [(id, ilevel, category), ...]
    for item in items:
        ilevel, cat = calc_ilevel(item, weights)
        results.append((item['id'], ilevel, cat))
    elapsed = time.time() - t0
    print(f"  Calculated {len(results):,} items in {elapsed:.2f}s.")

    if args.verify:
        # Compare against existing DB values
        cursor = conn.cursor(dictionary=True)
        cursor.execute("SELECT id, calculated_ilevel FROM items WHERE id < 1000000000 ORDER BY id")
        db_rows = {r['id']: r['calculated_ilevel'] for r in cursor.fetchall()}
        cursor.close()

        mismatches = 0
        for item_id, ilevel, cat in results:
            db_val = db_rows.get(item_id, 0)
            if db_val != ilevel:
                mismatches += 1
                if mismatches <= 20:
                    item = next((i for i in items if i['id'] == item_id), {})
                    print(f"  MISMATCH id={item_id} name='{item.get('Name', '?')}' "
                          f"db={db_val} calc={ilevel} ({cat})")

        if mismatches == 0:
            print(f"\n  All {len(results):,} items match. Database is up to date.")
        else:
            print(f"\n  {mismatches:,} mismatches found out of {len(results):,} items.")
            if mismatches > 20:
                print(f"  (showing first 20)")
        conn.close()
        return

    # Distribution + summary
    print_distribution(results)

    ilevels = [r[1] for r in results]
    wpns = [r for r in results if r[2] == 'WPN']
    arms = [r for r in results if r[2] == 'ARM']

    total = len(ilevels)
    avg = sum(ilevels) / total if total else 0
    srt = sorted(ilevels)
    med = srt[total // 2] if total else 0

    print(f"\n  Total items:    {total:>8,}")
    print(f"  Weapons:        {len(wpns):>8,}")
    print(f"  Armor/Other:    {len(arms):>8,}")
    print(f"  iLevel range:   {min(ilevels)} - {max(ilevels)}")
    print(f"  Average iLevel: {avg:.0f}")
    print(f"  Median iLevel:  {med}")

    print_top_items(items, results, n=args.top)

    # Write
    mode = "DRY RUN" if args.dry_run else "WRITING"
    print(f"\n{mode}: updating calculated_ilevel...")
    updated = batch_update(conn, results, dry_run=args.dry_run)

    if args.dry_run:
        print(f"  Dry run complete — {len(results):,} items calculated, 0 written.")
    else:
        print(f"  Updated {updated:,} rows.")

    conn.close()
    print("\nDone.")


if __name__ == '__main__':
    main()
