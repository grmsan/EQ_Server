#!/usr/bin/env python3
"""
Stat budget report for droppable gear by expansion and NPC level band.

Purpose:
  - Estimate what a player can reasonably expect from live droppable gear.
  - Convert per-item stat values into rough full-set expectations.
  - Highlight where current tier rules do and do not grow raw attributes.

Defaults assume:
  - 20 effective gear slots
  - 75 baseline raw stat before gear
  - Enchanted doubles raw attributes
  - Legendary keeps raw attributes at Enchanted values

This script uses unique item IDs per (expansion, level band) so a commonly reused
loot table item does not dominate the averages purely due to repeated placement.
"""

import argparse
import json
import math
import os
import statistics
import sys
from collections import defaultdict

try:
    import mysql.connector
except ImportError:
    print("ERROR: mysql-connector-python not installed.")
    print("  pip install mysql-connector-python")
    sys.exit(1)


WEAPON_TYPES = {0, 1, 2, 3, 4, 5, 35, 45}
EQUIPPABLE_ITEM_TYPES = sorted(WEAPON_TYPES | {8, 10})
ATTR_COLS = ["astr", "asta", "aagi", "adex", "aint", "awis", "acha"]

EXPANSION_NAMES = {
    0: "Classic",
    1: "Kunark",
    2: "Velious",
    3: "Luclin",
    4: "PoP",
    5: "LDoN",
    6: "Ykesha/Legacy",
    7: "Gates",
    8: "OoW",
}


def get_connection():
    config_path = os.path.join(os.path.dirname(__file__), "..", "eqemu_config.json")
    if not os.path.exists(config_path):
        print(f"ERROR: Cannot find {config_path}")
        sys.exit(1)

    with open(config_path, "r", encoding="utf-8") as handle:
        config = json.load(handle)
    db_cfg = config["server"]["database"]

    return mysql.connector.connect(
        host=db_cfg["host"],
        port=int(db_cfg.get("port", 3306)),
        user=db_cfg["username"],
        password=db_cfg["password"],
        database=db_cfg["db"],
    )


def percentile(values, pct):
    if not values:
        return 0.0
    if len(values) == 1:
        return float(values[0])
    ordered = sorted(values)
    pos = (len(ordered) - 1) * pct
    low = math.floor(pos)
    high = math.ceil(pos)
    if low == high:
        return float(ordered[low])
    low_val = ordered[low]
    high_val = ordered[high]
    return float(low_val + (high_val - low_val) * (pos - low))


def level_band(level):
    return ((max(1, level) - 1) // 10) * 10 + 1


def fetch_rows(conn, min_expansion, max_expansion, max_level, require_primary, include_tiered):
    cursor = conn.cursor(dictionary=True)
    attr_expr = "GREATEST(i.astr, i.asta, i.aagi, i.adex, i.aint, i.awis, i.acha)"
    primary_clause = f"AND {attr_expr} > 0" if require_primary else ""
    tier_clause = ""
    if not include_tiered:
        tier_clause = """
          AND i.Name NOT LIKE '% (Enchanted)'
          AND i.Name NOT LIKE '% (Legendary)'
          AND i.Name NOT LIKE '% (Mythic)'
        """

    query = f"""
        SELECT
            z.expansion,
            npc.level AS npc_level,
            i.id,
            i.Name,
            i.itemtype,
            i.damage,
            i.delay,
            i.ac,
            i.hp,
            i.mana,
            i.endur,
            i.attack,
            i.haste,
            i.reclevel,
            i.magic,
            i.astr, i.asta, i.aagi, i.adex, i.aint, i.awis, i.acha
        FROM zone z
        JOIN spawn2 s2
          ON s2.zone = z.short_name
         AND s2.version = z.version
        JOIN spawnentry se
          ON se.spawngroupID = s2.spawngroupID
        JOIN npc_types npc
          ON npc.id = se.npcID
         AND npc.loottable_id > 0
        JOIN loottable_entries lte
          ON lte.loottable_id = npc.loottable_id
        JOIN lootdrop_entries lde
          ON lde.lootdrop_id = lte.lootdrop_id
        JOIN items i
          ON i.id = lde.item_id
        WHERE z.version = 0
          AND z.expansion BETWEEN %s AND %s
          AND npc.level BETWEEN 1 AND %s
          AND i.id < 1000000000
          AND i.itemtype IN ({",".join(str(t) for t in EQUIPPABLE_ITEM_TYPES)})
          AND (i.damage > 0 OR i.ac > 0)
          AND i.Name NOT LIKE 'Summoned:%%'
          AND i.Name NOT LIKE 'Fabled%%'
          {tier_clause}
          {primary_clause}
    """
    cursor.execute(query, (min_expansion, max_expansion, max_level))
    rows = cursor.fetchall()
    cursor.close()
    return rows


def summarize_group(items, slots, base_stat, enchanted_multiplier, legendary_multiplier, end_aug_per_slot):
    primary_vals = [item["primary"] for item in items]
    all_stat_vals = [item["all_stats"] for item in items]

    avg_primary = statistics.fmean(primary_vals)
    med_primary = statistics.median(primary_vals)
    p75_primary = percentile(primary_vals, 0.75)
    p90_primary = percentile(primary_vals, 0.90)
    avg_all_stats = statistics.fmean(all_stat_vals)

    def total_raw(per_item_value, tier_mult, aug_per_slot=0.0):
        return base_stat + slots * ((per_item_value * tier_mult) + aug_per_slot)

    return {
        "items": len(items),
        "avg_primary": avg_primary,
        "med_primary": med_primary,
        "p75_primary": p75_primary,
        "p90_primary": p90_primary,
        "avg_all_stats": avg_all_stats,
        "starter_total": total_raw(med_primary, 1.0),
        "mid_total": total_raw(med_primary, enchanted_multiplier),
        "end_total": total_raw(p90_primary, legendary_multiplier, end_aug_per_slot),
    }


def build_report(rows, slots, base_stat, enchanted_multiplier, legendary_multiplier, end_aug_per_slot):
    grouped = defaultdict(dict)

    for row in rows:
        exp = int(row["expansion"])
        band = level_band(int(row["npc_level"]))
        item_id = int(row["id"])

        attrs = [int(row[col] or 0) for col in ATTR_COLS]
        primary = max(attrs)
        all_stats = sum(attrs)

        grouped[(exp, band)][item_id] = {
            "id": item_id,
            "name": row["Name"],
            "primary": primary,
            "all_stats": all_stats,
        }

    report = []
    for (exp, band), item_map in sorted(grouped.items()):
        items = list(item_map.values())
        report.append(
            {
                "expansion": exp,
                "expansion_name": EXPANSION_NAMES.get(exp, f"Expansion {exp}"),
                "level_band": band,
                **summarize_group(
                    items,
                    slots,
                    base_stat,
                    enchanted_multiplier,
                    legendary_multiplier,
                    end_aug_per_slot,
                ),
            }
        )
    return report


def print_band_table(report):
    print(
        f"{'Exp':<3} {'Era':<10} {'Lvl':<7} {'Items':>5} "
        f"{'AvgPri':>7} {'MedPri':>7} {'P90Pri':>7} {'AvgAll':>7}"
    )
    print(
        f"{'-'*3} {'-'*10} {'-'*7} {'-'*5} "
        f"{'-'*7} {'-'*7} {'-'*7} {'-'*7}"
    )
    for row in report:
        band_label = f"{row['level_band']:02d}-{row['level_band'] + 9:02d}"
        print(
            f"{row['expansion']:<3} {row['expansion_name']:<10} {band_label:<7} {row['items']:>5} "
            f"{row['avg_primary']:>7.2f} {row['med_primary']:>7.2f} {row['p90_primary']:>7.2f} {row['avg_all_stats']:>7.2f}"
        )


def print_capstone_table(report):
    by_expansion = {}
    for row in report:
        cur = by_expansion.get(row["expansion"])
        if cur is None or row["level_band"] > cur["level_band"]:
            by_expansion[row["expansion"]] = row

    print()
    print("Expansion-cap full-set estimate (20-slot model by default)")
    print(
        f"{'Exp':<3} {'Era':<10} {'Band':<7} {'Starter':>8} {'Mid':>8} {'End':>8} {'Note':<24}"
    )
    print(
        f"{'-'*3} {'-'*10} {'-'*7} {'-'*8} {'-'*8} {'-'*8} {'-'*24}"
    )
    for exp in sorted(by_expansion):
        row = by_expansion[exp]
        note = "Legendary raw = Enchanted raw"
        band_label = f"{row['level_band']:02d}-{row['level_band'] + 9:02d}"
        print(
            f"{exp:<3} {row['expansion_name']:<10} {band_label:<7} "
            f"{row['starter_total']:>8.0f} {row['mid_total']:>8.0f} {row['end_total']:>8.0f} {note:<24}"
        )


def print_findings(report, enchanted_multiplier, legendary_multiplier):
    print()
    print("Findings")
    print(
        f"- `Enchanted` multiplies raw attributes by {enchanted_multiplier:.2f}x, but the current raw-attribute endgame ceiling is effectively {legendary_multiplier:.2f}x base."
    )
    print(
        "- If you want starter-to-endgame growth inside one expansion to come mostly from raw STR/STA/DEX/AGI/INT/WIS/CHA, current tiering underdelivers after Enchanted."
    )
    print(
        "- If you are comfortable with endgame growth shifting into heroics, AC/HP/Mana, haste, attack, and augs, the current item-tier model is coherent but needs that to be stated explicitly in balance docs."
    )


def main():
    parser = argparse.ArgumentParser(description="Estimate stat budgets by expansion and level band.")
    parser.add_argument("--min-expansion", type=int, default=0, help="Minimum DB expansion id (default: 0)")
    parser.add_argument("--max-expansion", type=int, default=4, help="Maximum DB expansion id (default: 4)")
    parser.add_argument("--max-level", type=int, default=65, help="Maximum NPC level to include (default: 65)")
    parser.add_argument("--slots", type=int, default=20, help="Effective gear slots to model (default: 20)")
    parser.add_argument("--base-stat", type=float, default=75.0, help="Baseline raw stat before gear (default: 75)")
    parser.add_argument("--enchanted-multiplier", type=float, default=2.0, help="Raw attr multiplier at Enchanted (default: 2.0)")
    parser.add_argument("--legendary-multiplier", type=float, default=2.0, help="Raw attr multiplier at Legendary/Mythic (default: 2.0)")
    parser.add_argument("--end-aug-per-slot", type=float, default=0.0, help="Extra raw-stat-equivalent budget per slot for endgame aug investment (default: 0)")
    parser.add_argument("--include-zero-primary", action="store_true", help="Include equippable drops with no primary stats")
    parser.add_argument("--include-tiered", action="store_true", help="Include generated Enchanted/Legendary/Mythic item rows")
    args = parser.parse_args()

    conn = get_connection()
    try:
        rows = fetch_rows(
            conn,
            args.min_expansion,
            args.max_expansion,
            args.max_level,
            require_primary=not args.include_zero_primary,
            include_tiered=args.include_tiered,
        )
    finally:
        conn.close()

    report = build_report(
        rows,
        slots=args.slots,
        base_stat=args.base_stat,
        enchanted_multiplier=args.enchanted_multiplier,
        legendary_multiplier=args.legendary_multiplier,
        end_aug_per_slot=args.end_aug_per_slot,
    )

    print_band_table(report)
    print_capstone_table(report)
    print_findings(report, args.enchanted_multiplier, args.legendary_multiplier)


if __name__ == "__main__":
    main()
