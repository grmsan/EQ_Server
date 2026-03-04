#!/usr/bin/env python3
"""
Raid Economy Model — queries live DB loot tables to validate Essence numbers.

iLevel = raw power score (uncapped).
Essence = max(1, iLevel - 100) for magic items only.  Non-magic items yield 0.
Run from repo root:  python tools/model_raid_economy.py

Design targets:
  - Rusty sword ≈ iLevel 14 (not magic — unsalvageable)
  - Top OoW raid weapon ≈ iLevel 1,000+ (→ ~900 Essence)
  - Full Anguish clear ≈ 29k–37k Essence
  - Tier costs: iLevel² power curve with per-tier floor+scale
    Rusty(14): 3 / 31 / 1,524    Anguish(1106): 9,788 / 40,387 / 149,478

See game_design/infinite_progression/ITEM_LEVEL_SYSTEM.md for the algorithm.
"""

import sys, os, math, json

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

try:
    import mysql.connector
    config_path = os.path.join(os.path.dirname(__file__), '..', 'eqemu_config.json')
    with open(config_path) as f:
        config = json.load(f)
    db_config = config['server']['database']
    conn = mysql.connector.connect(
        host=db_config['host'], port=int(db_config.get('port', 3306)),
        user=db_config['username'], password=db_config['password'],
        database=db_config['db']
    )
    cursor = conn.cursor(dictionary=True)
except Exception as e:
    print(f"DB connection failed: {e}"); sys.exit(1)

WEAPON_TYPES = {0, 1, 2, 3, 4, 5, 35}

def get_category(itemtype):
    return 'WEAPON' if itemtype in WEAPON_TYPES else 'ARMOR'

def calc_power(item, category):
    if category == 'WEAPON':
        delay = max(1, item.get('delay', 0) or 1)
        damage = item.get('damage', 0) or 0
        dps = (damage * 100.0) / delay
        ac = item.get('ac', 0) or 0
        hp = item.get('hp', 0) or 0
        mana = item.get('mana', 0) or 0
        attrs = sum(item.get(a, 0) or 0 for a in ['astr','asta','aagi','adex','awis','aint','acha'])
        attack = item.get('attack', 0) or 0
        haste = item.get('haste', 0) or 0
        heroics = sum(item.get(h, 0) or 0 for h in [
            'heroic_str','heroic_sta','heroic_agi','heroic_dex',
            'heroic_wis','heroic_int','heroic_cha'])
        return dps + (ac * 3) + (hp * 0.3) + (mana * 0.3) + (attrs * 2) + attack + (haste * 10) + (heroics * 5)
    else:
        ac = item.get('ac', 0) or 0
        hp = item.get('hp', 0) or 0
        mana = item.get('mana', 0) or 0
        attrs = sum(item.get(a, 0) or 0 for a in ['astr','asta','aagi','adex','awis','aint','acha'])
        resists = sum(item.get(r, 0) or 0 for r in ['fr','cr','mr','dr','pr'])
        heroics = sum(item.get(h, 0) or 0 for h in [
            'heroic_str','heroic_sta','heroic_agi','heroic_dex',
            'heroic_wis','heroic_int','heroic_cha'])
        return (ac * 15) + (hp * 0.5) + (mana * 0.5) + (attrs * 3) + (resists * 0.5) + (heroics * 5)

def calc_ilevel(item):
    cat = get_category(item.get('itemtype', 10))
    return max(1, int(calc_power(item, cat)))

ESSENCE_OFFSET = 100  # Subtracted from iLevel before Essence calculation

def calc_essence(ilevel, magic=1):
    """Essence = max(1, iLevel - offset) for magic items; 0 for non-magic."""
    if not magic:
        return 0
    return max(1, ilevel - ESSENCE_OFFSET)


# DB queries
def query_zone_items(zone_name):
    cursor.execute("""
    SELECT DISTINCT i.id, i.Name, i.itemtype, i.magic,
           i.damage, i.delay, i.ac, i.hp, i.mana, i.endur,
           i.astr, i.asta, i.aagi, i.adex, i.awis, i.aint, i.acha,
           i.attack, i.haste,
           i.heroic_str, i.heroic_sta, i.heroic_agi, i.heroic_dex,
           i.heroic_wis, i.heroic_int, i.heroic_cha,
           i.fr, i.cr, i.mr, i.dr, i.pr, i.reclevel
    FROM npc_types npc
    JOIN spawnentry se ON se.npcID = npc.id
    JOIN spawn2 s2 ON s2.spawngroupID = se.spawngroupID
    JOIN loottable_entries lte ON npc.loottable_id = lte.loottable_id
    JOIN lootdrop_entries lde ON lte.lootdrop_id = lde.lootdrop_id
    JOIN items i ON i.id = lde.item_id
    WHERE s2.zone = %s
      AND i.itemtype IN (0,1,2,3,4,5,8,10,35)
      AND (i.damage > 0 OR i.ac > 0)
      AND i.id < 1000000000
      AND i.Name NOT LIKE 'Summoned:%%'
    """, (zone_name,))
    return cursor.fetchall()

def query_reclevel_items(min_rec, max_rec, min_hp=100):
    cursor.execute("""
    SELECT DISTINCT i.id, i.Name, i.itemtype, i.magic,
           i.damage, i.delay, i.ac, i.hp, i.mana, i.endur,
           i.astr, i.asta, i.aagi, i.adex, i.awis, i.aint, i.acha,
           i.attack, i.haste,
           i.heroic_str, i.heroic_sta, i.heroic_agi, i.heroic_dex,
           i.heroic_wis, i.heroic_int, i.heroic_cha,
           i.fr, i.cr, i.mr, i.dr, i.pr, i.reclevel
    FROM items i
    WHERE i.reclevel BETWEEN %s AND %s
      AND i.hp >= %s
      AND i.itemtype IN (0,1,2,3,4,5,8,10,35)
      AND i.id < 1000000000
      AND i.Name NOT LIKE 'Summoned:%%'
      AND i.Name NOT LIKE 'Fabled%%'
      AND i.Name NOT LIKE '%%+%%'
    ORDER BY i.hp DESC LIMIT 50
    """, (min_rec, max_rec, min_hp))
    return cursor.fetchall()


# ============================================================
# REFERENCE ITEMS
# ============================================================

print("=" * 90)
print("ESSENCE ECONOMY v3: iLevel = Power Score, Essence = max(1, iLevel-100), magic only")
print("=" * 90)

# Hardcoded reference
refs = [
    ("Rusty Long Sword", 0, 5, 35, 0, 0, 0, {}),
    ("Cloth Cap", 10, 0, 0, 2, 0, 0, {}),
]

print(f"\n{'Item':<35} {'Power':>7} {'iLevel':>7} {'Magic':>6} {'Essence':>8}")
print(f"{'-'*35} {'-'*7} {'-'*7} {'-'*6} {'-'*8}")

for name, itype, dmg, dly, ac, hp, mana, extra in refs:
    item = {"Name": name, "itemtype": itype, "damage": dmg, "delay": dly,
            "ac": ac, "hp": hp, "mana": mana, "magic": 0,
            "astr": 0, "asta": 0, "aagi": 0, "adex": 0, "awis": 0, "aint": 0, "acha": 0,
            "attack": 0, "haste": 0,
            "heroic_str": 0, "heroic_sta": 0, "heroic_agi": 0, "heroic_dex": 0,
            "heroic_wis": 0, "heroic_int": 0, "heroic_cha": 0,
            "fr": 0, "cr": 0, "mr": 0, "dr": 0, "pr": 0}
    item.update(extra)
    il = calc_ilevel(item)
    mg = item.get('magic', 0)
    ess = calc_essence(il, mg)
    ess_str = str(ess) if mg else '\u2014'
    print(f"{name:<35} {calc_power(item, get_category(itype)):>7.1f} {il:>7} {'Yes' if mg else 'No':>6} {ess_str:>8}")

# DB reference items
cursor.execute("""
    SELECT *, i.astr, i.asta, i.aagi, i.adex, i.awis, i.aint, i.acha,
           i.heroic_str, i.heroic_sta, i.heroic_agi, i.heroic_dex,
           i.heroic_wis, i.heroic_int, i.heroic_cha,
           i.fr, i.cr, i.mr, i.dr, i.pr, i.magic
    FROM items i WHERE i.id IN (5019, 5157, 28854, 25989, 47322, 47320, 47315)
    ORDER BY i.hp
""")
for item in cursor.fetchall():
    il = calc_ilevel(item)
    cat = get_category(item['itemtype'])
    power = calc_power(item, cat)
    mg = item.get('magic', 0)
    ess = calc_essence(il, mg)
    ess_str = str(ess) if mg else '\u2014'
    print(f"{item['Name'][:35]:<35} {power:>7.1f} {il:>7} {'Yes' if mg else 'No':>6} {ess_str:>8}")


# ============================================================
# ZONE ANALYSIS
# ============================================================

ZONES = [
    ('soldungc', 'Solusek C (Nagafen)', 'Classic'),
    ('fearplane', 'Plane of Fear', 'Classic'),
    ('hateplaneb', 'Plane of Hate', 'Classic'),
    ('veeshan', 'Temple of Veeshan', 'Velious'),
    ('sleeper', "Sleeper's Tomb", 'Velious'),
    ('vexthal', 'Vex Thal', 'Luclin'),
    ('ssratemple', 'Ssraeshza Temple', 'Luclin'),
    ('pofire', 'Plane of Fire', 'PoP'),
    ('podisease', 'Plane of Disease', 'PoP'),
    ('solrotower', 'Sol Ro Tower', 'PoP'),
    ('anguish', 'Anguish', 'OoW'),
    ('txevu', 'Txevu', 'OoW'),
]

print(f"\n\n{'='*90}")
print(f"{'Zone':<28} {'Era':<8} {'Items':>5} {'AvgPwr':>7} {'MaxPwr':>7} {'AvgEss':>7} {'TotalEss':>9}")
print(f"{'-'*28} {'-'*8} {'-'*5} {'-'*7} {'-'*7} {'-'*7} {'-'*9}")

zone_data = {}
for zs, zn, era in ZONES:
    items = query_zone_items(zs)
    if not items:
        print(f"{zn:<28} {era:<8} {'N/A':>5}")
        continue
    # Only magic items can be salvaged
    magic_items = [i for i in items if i.get('magic', 0)]
    powers = [calc_ilevel(i) for i in magic_items]
    essences = [calc_essence(p, 1) for p in powers]
    total = sum(essences)
    zone_data[zs] = {'total': total, 'n': len(magic_items), 'all_n': len(items),
                     'avg_p': sum(powers)/len(powers) if powers else 0,
                     'max_p': max(powers) if powers else 0,
                     'avg_e': total/len(essences) if essences else 0}
    z = zone_data[zs]
    print(f"{zn:<28} {era:<8} {z['n']:>5} {z['avg_p']:>7.0f} {z['max_p']:>7} {z['avg_e']:>7.0f} {z['total']:>9,}")

# Reclevel samples
print()
rec_ranges = [
    (60, 65, 150, "PoP token rewards (60-65)", "PoP"),
    (65, 70, 200, "OoW/DoN (rec 65-70)", "OoW"),
]
for min_r, max_r, min_hp, label, era in rec_ranges:
    items = query_reclevel_items(min_r, max_r, min_hp)
    if not items:
        continue
    magic_items = [i for i in items if i.get('magic', 0)]
    if not magic_items:
        continue
    powers = [calc_ilevel(i) for i in magic_items]
    essences = [calc_essence(p, 1) for p in powers]
    total = sum(essences)
    avg_p = sum(powers)/len(powers)
    print(f"{label:<28} {era:<8} {len(magic_items):>5} {avg_p:>7.0f} {max(powers):>7} {total/len(essences):>7.0f} {total:>9,}")


# ============================================================
# iLEVEL-SCALED TIER COST ANALYSIS
# ============================================================

print(f"\n\n{'='*90}")
print("iLEVEL-SCALED TIER COSTS")
print(f"{'='*90}\n")

# Tier cost formula: cost = FLOOR + round(iLevel² × SCALE)
TIER_PARAMS = [
    (1,    0.008),   # Base -> Enchanted:    floor=1,    scale=0.008
    (25,   0.033),   # Enchanted -> Legendary: floor=25,   scale=0.033
    (1500, 0.121),   # Legendary -> Mythic:    floor=1500, scale=0.121
]

def calc_tier_cost(ilevel, tier_idx):
    """tier_idx: 0=B->E, 1=E->L, 2=L->M"""
    floor_val, scale = TIER_PARAMS[tier_idx]
    return max(1, floor_val + round(ilevel * ilevel * scale))

print("Representative tier-up costs (iLevel-squared power curve):")
print("  Formula: cost = FLOOR + round(iLevel^2 x SCALE)")
print(f"{'Item':<25} {'iLevel':>7} {'Base->Ench':>11} {'Ench->Leg':>11} {'Leg->Myth':>11} {'Total':>11}")
print(f"{'-'*25} {'-'*7} {'-'*11} {'-'*11} {'-'*11} {'-'*11}")

ref_items = [
    ("Rusty Sword", 14),
    ("Primal Velium 2H", 310),
    ("Blade of War", 645),
    ("Greatblade of Chaos", 735),
    ("Anguish avg", 1106),
]

for name, il in ref_items:
    c_e = calc_tier_cost(il, 0)
    c_l = calc_tier_cost(il, 1)
    c_m = calc_tier_cost(il, 2)
    print(f"{name:<25} {il:>7} {c_e:>11,} {c_l:>11,} {c_m:>11,} {c_e+c_l+c_m:>11,}")

print(f"\n\n{'='*90}")
print("WHAT THIS FEELS LIKE PER ERA (iLevel-scaled)")
print(f"{'='*90}\n")

print(f"{'Zone':<28} {'Era':<8} {'TotalEss':>9} {'AvgIL':>6}  {'Ench->Leg':>10} {'Clears':>7}")
print(f"{'(one full clear)':<28} {'':>8} {'':>9} {'':>6}  {'(avg item)':>10} {'needed':>7}")
print(f"{'-'*28} {'-'*8} {'-'*9} {'-'*6}  {'-'*10} {'-'*7}")

for zs, zn, era in ZONES:
    z = zone_data.get(zs)
    if not z:
        continue
    avg_il = int(z['avg_p'])
    leg_cost = calc_tier_cost(avg_il, 1)
    clears = leg_cost / z['total'] if z['total'] > 0 else 0
    print(f"{zn:<28} {era:<8} {z['total']:>9,} {avg_il:>6}  {leg_cost:>10,} {clears:>7.1f}")

conn.close()
print("\nDone.")
