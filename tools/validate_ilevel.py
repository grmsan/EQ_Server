#!/usr/bin/env python3
"""
iLevel Algorithm Validation — tests power-score-based iLevel against live DB items.

Algorithm (from ITEM_LEVEL_SYSTEM.md):
  Weapons: DPS + AC×3 + HP×0.3 + Mana×0.3 + Attrs×2 + Attack + Haste×10 + Heroics×5
  Armor:   AC×15 + HP×0.5 + Mana×0.5 + Attrs×3 + Resists×0.5 + Heroics×5
  iLevel = max(1, int(power_score))          # uncapped
  Essence = max(1, iLevel - 100) for magic items; 0 for non-magic

Run from repo root:
  python tools/validate_ilevel.py
"""

import json
import os
import sys

import mysql.connector


# ── DB connection ──────────────────────────────────────────────

config_path = os.path.join(os.path.dirname(__file__), '..', 'eqemu_config.json')
with open(config_path) as f:
    config = json.load(f)
db_cfg = config['server']['database']

conn = mysql.connector.connect(
    host=db_cfg['host'],
    port=int(db_cfg.get('port', 3306)),
    user=db_cfg['username'],
    password=db_cfg['password'],
    database=db_cfg['db'],
)
cursor = conn.cursor(dictionary=True)


# ── Power score / iLevel ──────────────────────────────────────

WEAPON_TYPES = {0, 1, 2, 3, 4, 5, 35}  # 1HS,1HB,2HS,2HB,2HP,Bow,H2H

STAT_ATTRS = ['astr', 'asta', 'aagi', 'adex', 'awis', 'aint', 'acha']
STAT_HEROICS = ['heroic_str', 'heroic_sta', 'heroic_agi', 'heroic_dex',
                'heroic_wis', 'heroic_int', 'heroic_cha']
STAT_RESISTS = ['fr', 'cr', 'mr', 'dr', 'pr']


def _s(item, key, default=0):
    """Safe stat getter — treats None as 0."""
    v = item.get(key, default)
    return v if v is not None else default


def calc_power(item):
    """Return (power_score, category_str)."""
    itype = _s(item, 'itemtype', 10)
    attrs = sum(_s(item, a) for a in STAT_ATTRS)
    heroics = sum(_s(item, h) for h in STAT_HEROICS)

    if itype in WEAPON_TYPES:
        delay = max(1, _s(item, 'delay', 1))
        dps = (_s(item, 'damage') * 100.0) / delay
        power = (dps
                 + _s(item, 'ac') * 3
                 + _s(item, 'hp') * 0.3
                 + _s(item, 'mana') * 0.3
                 + attrs * 2
                 + _s(item, 'attack')
                 + _s(item, 'haste') * 10
                 + heroics * 5)
        return max(1.0, power), 'WEAPON'
    else:
        resists = sum(_s(item, r) for r in STAT_RESISTS)
        power = (_s(item, 'ac') * 15
                 + _s(item, 'hp') * 0.5
                 + _s(item, 'mana') * 0.5
                 + attrs * 3
                 + resists * 0.5
                 + heroics * 5)
        return max(1.0, power), 'ARMOR'


ESSENCE_OFFSET = 100  # Subtracted from iLevel before Essence calculation


def calc_ilevel(item):
    """iLevel = int(power_score), minimum 1, no cap."""
    power, cat = calc_power(item)
    return max(1, int(power)), cat


def calc_essence(ilevel, magic=1):
    """Essence = max(1, iLevel - offset) for magic items; 0 for non-magic."""
    if not magic:
        return 0
    return max(1, ilevel - ESSENCE_OFFSET)


# ── Queries ───────────────────────────────────────────────────

ITEM_COLUMNS = """
    i.id, i.Name, i.itemtype, i.reclevel, i.magic,
    i.damage, i.delay, i.ac, i.hp, i.mana,
    i.astr, i.asta, i.aagi, i.adex, i.awis, i.aint, i.acha,
    i.attack, i.haste,
    i.heroic_str, i.heroic_sta, i.heroic_agi, i.heroic_dex,
    i.heroic_wis, i.heroic_int, i.heroic_cha,
    i.fr, i.cr, i.mr, i.dr, i.pr
"""

TEST_GROUPS = [
    ("Low-Level Weapons (reclevel 1-10)",
     f"SELECT {ITEM_COLUMNS} FROM items i "
     "WHERE i.reclevel > 0 AND i.reclevel <= 10 AND i.damage > 0 "
     "AND i.itemtype IN (0,1,2,3,4,5) AND i.id < 1000000000 "
     "ORDER BY i.reclevel LIMIT 8"),

    ("Mid-Level Weapons (reclevel 40-60)",
     f"SELECT {ITEM_COLUMNS} FROM items i "
     "WHERE i.reclevel BETWEEN 40 AND 60 AND i.damage > 0 "
     "AND i.itemtype IN (0,1,2,3,4,5) AND i.id < 1000000000 "
     "ORDER BY i.reclevel LIMIT 8"),

    ("High-Level Weapons (reclevel 65+)",
     f"SELECT {ITEM_COLUMNS} FROM items i "
     "WHERE i.reclevel >= 65 AND i.damage > 0 "
     "AND i.itemtype IN (0,1,2,3,4,5) AND i.id < 1000000000 "
     "ORDER BY i.reclevel DESC LIMIT 10"),

    ("Low-Level Armor (reclevel 1-10)",
     f"SELECT {ITEM_COLUMNS} FROM items i "
     "WHERE i.reclevel > 0 AND i.reclevel <= 10 AND i.itemtype = 10 "
     "AND i.id < 1000000000 "
     "ORDER BY i.reclevel LIMIT 8"),

    ("High-Level Armor (reclevel 65+)",
     f"SELECT {ITEM_COLUMNS} FROM items i "
     "WHERE i.reclevel >= 65 AND i.itemtype = 10 AND i.ac > 0 "
     "AND i.id < 1000000000 "
     "ORDER BY i.reclevel DESC LIMIT 10"),

    ("Weapons WITHOUT reclevel (highest damage)",
     f"SELECT {ITEM_COLUMNS} FROM items i "
     "WHERE i.reclevel = 0 AND i.damage > 0 AND i.itemtype IN (0,1,2,3,4,5) "
     "AND i.id < 1000000000 "
     "ORDER BY i.damage DESC LIMIT 10"),

    ("Armor WITHOUT reclevel (highest AC)",
     f"SELECT {ITEM_COLUMNS} FROM items i "
     "WHERE i.reclevel = 0 AND i.ac > 0 AND i.itemtype = 10 "
     "AND i.id < 1000000000 "
     "ORDER BY i.ac DESC LIMIT 10"),
]


# ── Reference Items ───────────────────────────────────────────

REFERENCE_IDS = {
    5019:  ("Hategiver",           'Classic',  269),
    5157:  ("Blade of Carnage",    'Classic',  None),
    28854: ("Blade of War",        'OoW',      645),
    25989: ("Time's Antithesis",   'PoP',      None),
    47322: ("Fabled Hategiver",    'Classic',  None),
}


# ── Main ──────────────────────────────────────────────────────

def run():
    w = 105
    print("=" * w)
    print("iLevel Algorithm Validation  (uncapped power-score system)")
    print("=" * w)

    # ── Reference items ──
    ref_ids = list(REFERENCE_IDS.keys())
    if ref_ids:
        placeholders = ','.join(['%s'] * len(ref_ids))
        cursor.execute(
            f"SELECT {ITEM_COLUMNS} FROM items i WHERE i.id IN ({placeholders}) ORDER BY i.hp",
            ref_ids)
        ref_items = cursor.fetchall()

        print(f"\nReference Items")
        print("-" * w)
        hdr = f"{'ID':<8} {'Name':<35} {'Cat':<6} {'RecLvl':>6} {'Magic':>6} {'Power':>8} {'iLevel':>7} {'Essence':>8}"
        print(hdr)
        print("-" * w)
        for item in ref_items:
            il, cat = calc_ilevel(item)
            power, _ = calc_power(item)
            mg = _s(item, 'magic', 0)
            ess = calc_essence(il, mg)
            ess_str = str(ess) if mg else '\u2014'
            print(f"{item['id']:<8} {item['Name'][:34]:<35} {cat:<6} "
                  f"{_s(item, 'reclevel'):>6} {'Yes' if mg else 'No':>6} {power:>8.1f} {il:>7} {ess_str:>8}")

    # ── Test groups ──
    total_items = 0
    for group_name, query in TEST_GROUPS:
        cursor.execute(query)
        items = cursor.fetchall()
        if not items:
            continue

        print(f"\n{group_name}")
        print("-" * w)
        print(f"{'ID':<8} {'Name':<35} {'Cat':<6} {'RecLvl':>6} {'Magic':>6} {'Power':>8} {'iLevel':>7} {'Essence':>8}")
        print("-" * w)

        for item in items:
            il, cat = calc_ilevel(item)
            power, _ = calc_power(item)
            mg = _s(item, 'magic', 0)
            ess = calc_essence(il, mg)
            ess_str = str(ess) if mg else '\u2014'
            print(f"{item['id']:<8} {item['Name'][:34]:<35} {cat:<6} "
                  f"{_s(item, 'reclevel'):>6} {'Yes' if mg else 'No':>6} {power:>8.1f} {il:>7} {ess_str:>8}")
            total_items += 1

    # ── Distribution summary ──
    print(f"\n{'=' * w}")
    print("ILEVEL DISTRIBUTION (all equippable items)")
    print(f"{'=' * w}\n")

    cursor.execute(f"""
        SELECT {ITEM_COLUMNS} FROM items i
        WHERE i.itemtype IN (0,1,2,3,4,5,8,10,35)
          AND (i.damage > 0 OR i.ac > 0)
          AND i.id < 1000000000
          AND i.Name NOT LIKE 'Summoned:%%'
    """)
    all_items = cursor.fetchall()

    buckets = {}
    for item in all_items:
        il, _ = calc_ilevel(item)
        bucket = (il // 100) * 100  # 0-99, 100-199, ...
        buckets[bucket] = buckets.get(bucket, 0) + 1

    print(f"{'iLevel Range':<20} {'Count':>7} {'Bar'}")
    print("-" * 60)
    max_count = max(buckets.values()) if buckets else 1
    for b in sorted(buckets.keys()):
        label = f"{b}-{b + 99}"
        bar_len = int(40 * buckets[b] / max_count)
        print(f"{label:<20} {buckets[b]:>7}  {'#' * bar_len}")

    total_all = len(all_items)
    ilevels = [calc_ilevel(i)[0] for i in all_items]
    if ilevels:
        avg_il = sum(ilevels) / len(ilevels)
        med_il = sorted(ilevels)[len(ilevels) // 2]
        print(f"\nTotal equippable items: {total_all:,}")
        print(f"iLevel range: {min(ilevels)} - {max(ilevels)}")
        print(f"Average iLevel: {avg_il:.0f}   Median: {med_il}")

    # ── Essence economy spot-check ──
    print(f"\n{'=' * w}")
    print("ESSENCE ECONOMY SPOT-CHECK (iLevel-scaled costs, magic only, offset=100)")
    print(f"{'=' * w}\n")

    TIER_PARAMS = [
        (1,    0.008),   # Base -> Enchanted
        (25,   0.033),   # Enchanted -> Legendary
        (1500, 0.121),   # Legendary -> Mythic
    ]

    def calc_tier_cost(il, tier_idx):
        floor_val, scale = TIER_PARAMS[tier_idx]
        return max(1, floor_val + round(il * il * scale))

    print("  Tier cost formula: FLOOR + round(iLevel^2 x SCALE)")
    print(f"  {'Item':<25} {'iLevel':>7} {'Base->Ench':>10} {'Ench->Leg':>10} {'Leg->Myth':>10}")
    print(f"  {'-'*25} {'-'*7} {'-'*10} {'-'*10} {'-'*10}")

    ref_costs = [(14, "Rusty Sword"), (310, "Primal Velium 2H"),
                 (645, "Blade of War"), (735, "Greatblade of Chaos"),
                 (1106, "Anguish avg")]
    for il, name in ref_costs:
        print(f"  {name:<25} {il:>7} {calc_tier_cost(il, 0):>10,} "
              f"{calc_tier_cost(il, 1):>10,} {calc_tier_cost(il, 2):>10,}")

    print()

    # How many average items to reach each tier for an Anguish item
    if ilevels:
        magic_items_all = [i for i in all_items if _s(i, 'magic', 0)]
        magic_ilevels = [calc_ilevel(i)[0] for i in magic_items_all]
        essences = [calc_essence(il, 1) for il in magic_ilevels]
        avg_ess = sum(essences) / len(essences) if essences else 0
        print(f"  Magic items: {len(magic_items_all):,} of {total_all:,} total")
        print(f"  Average Essence per magic item: {avg_ess:.0f}\n")
        anguish_leg = calc_tier_cost(1106, 1)
        items_needed = anguish_leg / avg_ess if avg_ess > 0 else float('inf')
        print(f"  Anguish avg Ench->Leg ({anguish_leg:,} Essence): ~{items_needed:.0f} avg magic items to salvage")

    cursor.close()
    conn.close()
    print(f"\nValidated {total_items} items across {len(TEST_GROUPS)} test groups.  Done.")


if __name__ == '__main__':
    run()
