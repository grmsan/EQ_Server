#!/usr/bin/env python3
"""
Batch Tiered Item Generator — creates Enchanted/Legendary/Mythic item rows in the DB.

For every base item (id < 1,000,000), this script generates up to three new rows with:
  - Scaled stats per tier formulas (see INFINITE_ITEM_PROGRESSION_DESIGN.md §3)
  - Name suffixed with " (Enchanted)", " (Legendary)", or " (Mythic)"
  - Aug slots opened per tier
  - Predictable IDs:  Enchanted = base_id + 250,000
                       Legendary = base_id + 500,000
                       Mythic    = base_id + 750,000

These IDs stay within the RoF2 item-link 20-bit mask (max 1,048,575) for base IDs up to ~298K.
Items with base_id > 298,575 are skipped (only a handful of pet armory items).

This eliminates client-side caching issues — each tier is a real DB item,
not a runtime overlay that the client can ignore.

Usage:
  python tools/generate_tiered_items.py                 # full run
  python tools/generate_tiered_items.py --dry-run       # show what would be inserted
  python tools/generate_tiered_items.py --verify        # check existing tiered items
  python tools/generate_tiered_items.py --item 28854    # generate tiers for one item
  python tools/generate_tiered_items.py --clean         # delete all generated tiered items

Run from repo root:
  python tools/generate_tiered_items.py
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

# ── Constants ────────────────────────────────────────────────────────────

TIER_OFFSET = 250_000     # Enchanted = base + 250K, …  (fits 20-bit item link mask 0xFFFFF)
MAX_BASE_ID = 999_999    # Only process items below this
# Items whose Mythic ID would exceed the 20-bit link limit (1,048,575) are skipped.
# base_id + 3 * TIER_OFFSET must be ≤ 1,048,575  →  base_id ≤ 298,575
MAX_LINKABLE_BASE = 1_048_575 - 3 * TIER_OFFSET  # 298,575

TIER_NAMES     = {1: "Enchanted", 2: "Legendary", 3: "Mythic"}
TIER_SUFFIXES  = {1: " (Enchanted)", 2: " (Legendary)", 3: " (Mythic)"}

# Weapon item types (must match common/item_ilevel.cpp IsWeaponType)
WEAPON_TYPES = {0, 1, 2, 3, 4, 5, 35, 45}
# 2H weapon / Bow types (get double aug slots)
TWO_HAND_TYPES = {1, 4, 5, 35}  # 2HSlash=1, 2HBlunt=4, Bow=5, 2HPiercing=35

# ── Default Tier Scaling Values (match common/ruletypes.h) ───────────────

DEFAULTS = {
    # Enchanted
    "EnchantedMultiplier":       2.0,
    "EnchantedHasteBonus":       3,
    # Legendary / Mythic
    "LegendaryCombatMultiplier":   2.6,
    "LegendaryAttributeMultiplier": 2.0,
    "LegendaryHasteBonus":       5,
    "LegendaryCombatEffectBonus": 1,
    # Aug slot counts — 1H / Armor
    "AugSlots1HBase":       1,
    "AugSlots1HEnchanted":  2,
    "AugSlots1HLegendary":  3,
    "AugSlots1HMythic":     4,
    # Aug slot counts — 2H / Bow
    "AugSlots2HBase":       2,
    "AugSlots2HEnchanted":  4,
    "AugSlots2HLegendary":  5,
    "AugSlots2HMythic":     6,
    # Default aug slot type for opened slots
    "TierAugSlotType":      1,
}

# ── Stat columns we read and write ──────────────────────────────────────

# Columns that get the "combat" multiplier at Legendary
COMBAT_COLS = ['damage', 'ac', 'hp', 'mana', 'endur']

# Columns that get the "attribute" multiplier (both Ench and Leg)
ATTR_COLS = ['astr', 'asta', 'aagi', 'adex', 'aint', 'awis', 'acha']

# Resist columns — scale same as attributes
RESIST_COLS = ['mr', 'fr', 'cr', 'dr', 'pr']

# Regen columns — scale same as combat stats
REGEN_COLS = ['regen', 'manaregen', 'enduranceregen']

# Combat effect columns — get small additive bonus at Legendary
COMBAT_EFFECT_COLS = ['combateffects', 'shielding', 'strikethrough', 'stunresist',
                      'spellshield', 'avoidance', 'accuracy']

# Aug slot columns (type and visible, slots 1-6)
AUG_SLOT_TYPES   = [f'augslot{i}type' for i in range(1, 7)]
AUG_SLOT_VISIBLE = [f'augslot{i}visible' for i in range(1, 7)]

# ALL stat-related columns we need to read
STAT_READ_COLS = (COMBAT_COLS + ATTR_COLS + RESIST_COLS + REGEN_COLS +
                  COMBAT_EFFECT_COLS + ['haste', 'attack', 'spelldmg', 'healamt',
                  'damageshield', 'heroic_str', 'heroic_sta', 'heroic_agi',
                  'heroic_dex', 'heroic_int', 'heroic_wis', 'heroic_cha'] +
                  AUG_SLOT_TYPES + AUG_SLOT_VISIBLE)

# Metadata columns needed for logic
META_COLS = ['id', 'Name', 'itemtype', 'delay', 'lore', 'magic']


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


# ── Tier Scaling Logic ──────────────────────────────────────────────────

def clamp8(val):
    """Clamp to int8 range [-128, 127]."""
    return max(-128, min(127, int(round(val))))


def is_2h_or_bow(item_type):
    return item_type in TWO_HAND_TYPES


def get_tier_aug_slots(tier, is_2h):
    key_prefix = "AugSlots2H" if is_2h else "AugSlots1H"
    tier_name = {0: "Base", 1: "Enchanted", 2: "Legendary", 3: "Mythic"}[tier]
    return DEFAULTS[f"{key_prefix}{tier_name}"]


def safe_int(val, default=0):
    """Coerce a DB value to int, handling None, Decimal, str."""
    if val is None:
        return default
    try:
        return int(val)
    except (ValueError, TypeError):
        return default


def scale_item(base_row, tier, cfg=None):
    """
    Given a base item dict and a tier (1/2/3), return a dict of column→value
    changes to apply to create the tiered version.

    Returns the full row dict with all columns from base, plus modifications.
    """
    if cfg is None:
        cfg = DEFAULTS

    r = dict(base_row)  # copy all base columns

    # Coerce all numeric stat columns to int for safe arithmetic
    all_numeric = (COMBAT_COLS + ATTR_COLS + RESIST_COLS + REGEN_COLS +
                   COMBAT_EFFECT_COLS + ['haste', 'attack', 'spelldmg', 'healamt',
                   'damageshield', 'heroic_str', 'heroic_sta', 'heroic_agi',
                   'heroic_dex', 'heroic_int', 'heroic_wis', 'heroic_cha', 'damage'] +
                   AUG_SLOT_TYPES + AUG_SLOT_VISIBLE)
    for col in all_numeric:
        if col in base_row:
            base_row[col] = safe_int(base_row[col])
            r[col] = safe_int(r[col])

    base_id   = base_row['id']
    base_name = base_row['Name']
    item_type = safe_int(base_row['itemtype'])

    # New ID and name
    r['id']   = base_id + (tier * TIER_OFFSET)
    r['Name'] = truncate_name(base_name, TIER_SUFFIXES[tier])
    r['lore'] = truncate_lore(base_row.get('lore', base_name), TIER_SUFFIXES[tier])

    if tier == 1:  # Enchanted
        mult = cfg["EnchantedMultiplier"]
        haste_bonus = cfg["EnchantedHasteBonus"]

        # Combat stats × mult
        for col in COMBAT_COLS:
            r[col] = int(round(base_row[col] * mult))

        # Attributes × mult
        for col in ATTR_COLS:
            r[col] = clamp8(base_row[col] * mult)

        # Resists × mult
        for col in RESIST_COLS:
            r[col] = clamp8(base_row[col] * mult)

        # Haste: additive (only if base has haste)
        if base_row['haste'] > 0:
            r['haste'] = base_row['haste'] + haste_bonus

        # Regen × mult
        for col in REGEN_COLS:
            if base_row[col] > 0:
                r[col] = int(round(base_row[col] * mult))

        # SpellDmg appears if item has INT
        if base_row['aint'] > 0:
            r['spelldmg'] = int(base_row['aint'])

        # HealAmt appears if item has WIS
        if base_row['awis'] > 0:
            r['healamt'] = int(base_row['awis'])

        # Attack × mult (if present)
        if base_row['attack'] > 0:
            r['attack'] = int(round(base_row['attack'] * mult))

        # DamageShield × mult
        if base_row['damageshield'] > 0:
            r['damageshield'] = int(round(base_row['damageshield'] * mult))

    elif tier >= 2:  # Legendary / Mythic (same stats)
        combat_mult = cfg["LegendaryCombatMultiplier"]
        attr_mult   = cfg["LegendaryAttributeMultiplier"]
        haste_bonus = cfg["LegendaryHasteBonus"]
        ce_bonus    = cfg["LegendaryCombatEffectBonus"]

        # Combat stats × combat_mult
        for col in COMBAT_COLS:
            r[col] = int(round(base_row[col] * combat_mult))

        # Attributes × attr_mult
        for col in ATTR_COLS:
            r[col] = clamp8(base_row[col] * attr_mult)

        # Resists × attr_mult
        for col in RESIST_COLS:
            r[col] = clamp8(base_row[col] * attr_mult)

        # Heroic stats = base attribute (1:1)
        r['heroic_str'] = int(base_row['astr'])
        r['heroic_sta'] = int(base_row['asta'])
        r['heroic_agi'] = int(base_row['aagi'])
        r['heroic_dex'] = int(base_row['adex'])
        r['heroic_int'] = int(base_row['aint'])
        r['heroic_wis'] = int(base_row['awis'])
        r['heroic_cha'] = int(base_row['acha'])

        # Haste: additive (only if base has haste)
        if base_row['haste'] > 0:
            r['haste'] = base_row['haste'] + haste_bonus

        # Regen × combat_mult
        for col in REGEN_COLS:
            if base_row[col] > 0:
                r[col] = int(round(base_row[col] * combat_mult))

        # Attack += base_damage × 2
        if base_row['damage'] > 0:
            r['attack'] = int(base_row['attack'] + base_row['damage'] * 2)
        elif base_row['attack'] > 0:
            r['attack'] = int(round(base_row['attack'] * combat_mult))

        # SpellDmg = base INT × 2
        if base_row['aint'] > 0:
            r['spelldmg'] = int(base_row['aint']) * 2

        # HealAmt = base WIS × 2
        if base_row['awis'] > 0:
            r['healamt'] = int(base_row['awis']) * 2

        # DamageShield × combat_mult
        if base_row['damageshield'] > 0:
            r['damageshield'] = int(round(base_row['damageshield'] * combat_mult))

        # Combat effect bonuses (only if base has the stat)
        for col in COMBAT_EFFECT_COLS:
            if base_row[col] > 0:
                r[col] = clamp8(base_row[col] + ce_bonus)

    # Aug slots — apply tier-appropriate count
    _2h = is_2h_or_bow(item_type)
    target_slots = get_tier_aug_slots(tier, _2h)

    # Count existing base slots and find the last used aug type
    base_slot_count = 0
    last_base_type = 0
    for i in range(6):
        t = base_row[AUG_SLOT_TYPES[i]]
        if t != 0:
            base_slot_count += 1
            last_base_type = t

    final_slots = max(base_slot_count, target_slots)
    final_slots = min(final_slots, 6)  # Cap at 6

    # Determine Legendary slot count (used to identify Mythic-specific extra slots)
    legendary_slots = max(base_slot_count, get_tier_aug_slots(2, _2h))
    legendary_slots = min(legendary_slots, 6)

    # Default fill type: match last base slot type, or fall back to config default
    fill_type = last_base_type if last_base_type > 0 else cfg["TierAugSlotType"]

    for i in range(6):
        col_type = AUG_SLOT_TYPES[i]
        col_vis  = AUG_SLOT_VISIBLE[i]
        if i < final_slots:
            # Keep existing type or fill new slots
            if r[col_type] == 0:
                # Mythic-specific extra slots (beyond Legendary count) get type 4
                if tier == 3 and i >= legendary_slots:
                    r[col_type] = 4
                else:
                    r[col_type] = fill_type
            r[col_vis] = 1
        # Don't close existing slots

    return r


def truncate_name(name, suffix, max_len=63):
    """Truncate name to fit within DB column limits (64 chars including null)."""
    if len(name) + len(suffix) > max_len:
        name = name[:max_len - len(suffix)]
    return name + suffix


def truncate_lore(lore, suffix, max_len=79):
    """Truncate lore to fit within DB column limits (80 chars)."""
    if not lore:
        return suffix.strip()
    if len(lore) + len(suffix) > max_len:
        lore = lore[:max_len - len(suffix)]
    return lore + suffix


# ── Main ─────────────────────────────────────────────────────────────────

def get_all_columns(conn):
    """Get all column names from items table."""
    cur = conn.cursor()
    cur.execute("SHOW COLUMNS FROM items")
    cols = [r[0] for r in cur.fetchall()]
    cur.close()
    return cols


def fetch_base_items(conn, item_id=None):
    """Fetch base items from DB. Returns list of dicts."""
    cur = conn.cursor(dictionary=True)

    if item_id:
        cur.execute(f"SELECT * FROM items WHERE id = %s", (item_id,))
    else:
        cur.execute(f"SELECT * FROM items WHERE id < {MAX_BASE_ID + 1}")

    rows = cur.fetchall()
    cur.close()
    return rows


def delete_tiered_items(conn, dry_run=False):
    """Delete all generated tiered items (IDs in the tier ranges, excluding known high-ID base items)."""
    cur = conn.cursor()

    # Only delete IDs within the three tier ranges that correspond to valid base items.
    # This avoids deleting high-ID base items (e.g. pet armory items at 899K-900K).
    tier_ranges = " OR ".join(
        f"(id >= {t * TIER_OFFSET} AND id < {(t + 1) * TIER_OFFSET})"
        for t in [1, 2, 3]
    )
    where = f"({tier_ranges}) AND (id % {TIER_OFFSET}) < {TIER_OFFSET}"

    cur.execute(f"SELECT COUNT(*) FROM items WHERE {where}")
    count = cur.fetchone()[0]

    if count == 0:
        print("  No tiered items found to delete.")
        cur.close()
        return 0

    if dry_run:
        print(f"  [DRY RUN] Would delete {count} tiered items (id >= {TIER_OFFSET}).")
        cur.close()
        return count

    cur.execute(f"DELETE FROM items WHERE {where}")
    conn.commit()
    print(f"  Deleted {count} tiered items.")
    cur.close()
    return count


def insert_tiered_items(conn, all_columns, tiered_rows, dry_run=False, batch_size=500):
    """Insert tiered item rows into the database."""
    if not tiered_rows:
        return 0

    if dry_run:
        return len(tiered_rows)

    cur = conn.cursor()

    # Build INSERT ... ON DUPLICATE KEY UPDATE (upsert) so re-runs are safe
    col_list = ', '.join(f'`{c}`' for c in all_columns)
    placeholders = ', '.join(['%s'] * len(all_columns))

    # ON DUPLICATE KEY UPDATE — update all non-id columns
    update_parts = ', '.join(f'`{c}` = VALUES(`{c}`)' for c in all_columns if c != 'id')

    sql = f"INSERT INTO items ({col_list}) VALUES ({placeholders}) ON DUPLICATE KEY UPDATE {update_parts}"

    inserted = 0
    for i in range(0, len(tiered_rows), batch_size):
        batch = tiered_rows[i:i + batch_size]
        values = []
        for row in batch:
            values.append(tuple(row.get(c) for c in all_columns))

        cur.executemany(sql, values)
        conn.commit()
        inserted += len(batch)

        if inserted % 10000 == 0 or inserted == len(tiered_rows):
            print(f"  Inserted/updated {inserted}/{len(tiered_rows)} tiered items...")

    cur.close()
    return inserted


def print_sample(base_row, tiered_rows):
    """Print a before/after sample for one item."""
    print(f"\n  Base: [{base_row['id']}] {base_row['Name']}")
    print(f"    DMG={base_row['damage']}  AC={base_row['ac']}  HP={base_row['hp']}  "
          f"STR={base_row['astr']}  STA={base_row['asta']}  INT={base_row['aint']}  "
          f"WIS={base_row['awis']}  Haste={base_row['haste']}  SpellDmg={base_row['spelldmg']}  "
          f"HealAmt={base_row['healamt']}")

    for t in tiered_rows:
        tier = (t['id'] - base_row['id']) // TIER_OFFSET
        print(f"  Tier {tier} ({TIER_NAMES[tier]}): [{t['id']}] {t['Name']}")
        print(f"    DMG={t['damage']}  AC={t['ac']}  HP={t['hp']}  "
              f"STR={t['astr']}  STA={t['asta']}  INT={t['aint']}  "
              f"WIS={t['awis']}  Haste={t['haste']}  SpellDmg={t['spelldmg']}  "
              f"HealAmt={t['healamt']}")
        print(f"    HeroicSTR={t['heroic_str']}  HeroicSTA={t['heroic_sta']}  "
              f"Attack={t['attack']}  slots=[{t['augslot1type']},{t['augslot2type']},"
              f"{t['augslot3type']},{t['augslot4type']},{t['augslot5type']},{t['augslot6type']}]")


def main():
    parser = argparse.ArgumentParser(description="Generate tiered item rows in the database.")
    parser.add_argument('--dry-run', action='store_true', help="Calculate but don't write to DB")
    parser.add_argument('--verify', action='store_true', help="Check existing tiered items")
    parser.add_argument('--item', type=int, help="Process a single base item ID")
    parser.add_argument('--clean', action='store_true', help="Delete all generated tiered items")
    parser.add_argument('--sample', type=int, default=3, help="Number of sample items to show (default: 3)")
    parser.add_argument('--config', type=str, help="JSON file with scaling overrides")
    args = parser.parse_args()

    cfg = dict(DEFAULTS)
    if args.config:
        with open(args.config) as f:
            overrides = json.load(f)
        cfg.update(overrides)
        print(f"  Loaded config overrides from {args.config}")

    print("=" * 60)
    print("Tiered Item Generator")
    print("=" * 60)

    conn = get_connection()
    print(f"  Connected to database.")

    if args.clean:
        delete_tiered_items(conn, dry_run=args.dry_run)
        conn.close()
        return

    if args.verify:
        cur = conn.cursor()
        for tier_num, tier_name in TIER_NAMES.items():
            lo = tier_num * TIER_OFFSET
            hi = (tier_num + 1) * TIER_OFFSET
            cur.execute(f"SELECT COUNT(*) FROM items WHERE id >= {lo} AND id < {hi}")
            count = cur.fetchone()[0]
            print(f"  {tier_name} items (id {lo}-{hi}): {count}")
        cur.close()
        conn.close()
        return

    # Get all column names for INSERT
    all_columns = get_all_columns(conn)

    # Fetch base items
    print(f"  Fetching base items...")
    base_items = fetch_base_items(conn, item_id=args.item)
    print(f"  Found {len(base_items)} base items.")

    if len(base_items) == 0:
        print("  No items to process.")
        conn.close()
        return

    # Generate tiered versions
    print(f"  Generating tiered items...")
    t0 = time.time()
    all_tiered = []
    samples_shown = 0
    skipped_high_id = 0

    for base_row in base_items:
        base_id = safe_int(base_row['id'])
        # Skip items whose Mythic ID would exceed the 20-bit item-link mask
        if base_id > MAX_LINKABLE_BASE:
            skipped_high_id += 1
            continue

        item_tiers = []
        for tier in [1, 2, 3]:
            tiered = scale_item(base_row, tier, cfg)
            item_tiers.append(tiered)
            all_tiered.append(tiered)

        # Show samples
        if samples_shown < args.sample:
            # Only show items that have some stats
            if base_row['damage'] > 0 or base_row['ac'] > 0 or base_row['hp'] > 0:
                print_sample(base_row, item_tiers)
                samples_shown += 1

    if skipped_high_id:
        print(f"  Skipped {skipped_high_id} items with base_id > {MAX_LINKABLE_BASE} (would overflow link mask).")

    elapsed = time.time() - t0
    print(f"\n  Generated {len(all_tiered)} tiered items in {elapsed:.2f}s.")

    # Insert into DB
    if args.dry_run:
        print(f"  [DRY RUN] Would insert/update {len(all_tiered)} rows.")
    else:
        print(f"  Inserting into database (upsert mode)...")
        t0 = time.time()
        count = insert_tiered_items(conn, all_columns, all_tiered, dry_run=False)
        elapsed = time.time() - t0
        print(f"  Done. {count} rows inserted/updated in {elapsed:.1f}s.")

    print(f"\n  ID scheme: Enchanted = base_id + {TIER_OFFSET}")
    print(f"             Legendary = base_id + {TIER_OFFSET * 2}")
    print(f"             Mythic    = base_id + {TIER_OFFSET * 3}")
    print(f"  Example: base 28854 -> Ench {28854 + TIER_OFFSET}, Leg {28854 + TIER_OFFSET*2}, Myth {28854 + TIER_OFFSET*3}")

    conn.close()
    print("  Done.")


if __name__ == '__main__':
    main()
