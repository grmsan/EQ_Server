#!/usr/bin/env python
"""
Quick stat simulator.

- Loads DB creds from eqemu_config.json (override with CLI flags)
- Fetches a character's base stats + equipped item stats
- Emits derived values using the current design knobs
- Supports a synthetic "gear tier" by multiplying item stats (e.g., simulate level 100 gear)
"""

import argparse
import json
import math
import os
from dataclasses import dataclass, field
from typing import Dict, List, Optional, Tuple

import mysql.connector


DEFAULT_CONFIG_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "eqemu_config.json")


STAT_FIELDS = ["str", "dex", "agi", "sta", "wis", "int", "cha"]
ITEM_FIELDS = ["astr", "adex", "aagi", "asta", "awis", "aint", "acha", "ac", "hp", "mana", "endur"]


@dataclass
class Character:
    id: int
    name: str
    level: int
    cls: int
    base_stats: Dict[str, int]
    items: List[Dict] = field(default_factory=list)


def load_config(path: str) -> Dict[str, str]:
    with open(path, "r", encoding="utf-8") as f:
        cfg = json.load(f)
    db_cfg = cfg["server"]["database"]
    return {
        "host": db_cfg["host"],
        "port": db_cfg["port"],
        "user": db_cfg["username"],
        "password": db_cfg["password"],
        "database": db_cfg["db"],
    }


def connect_db(cfg: Dict[str, str]):
    return mysql.connector.connect(
        host=cfg["host"],
        port=int(cfg["port"]),
        user=cfg["user"],
        password=cfg["password"],
        database=cfg["database"],
    )


def fetch_character(conn, name: Optional[str], char_id: Optional[int]) -> Character:
    cur = conn.cursor(dictionary=True)
    if char_id is None:
        cur.execute(
            """
            SELECT id, name, class, level, str, dex, agi, sta, wis, `int`, cha
            FROM character_data
            WHERE name = %s
            """,
            (name,),
        )
    else:
        cur.execute(
            """
            SELECT id, name, class, level, str, dex, agi, sta, wis, `int`, cha
            FROM character_data
            WHERE id = %s
            """,
            (char_id,),
        )
    row = cur.fetchone()
    if not row:
        raise SystemExit(f"Character not found (name={name}, id={char_id})")
    base_stats = {
        "str": row["str"],
        "dex": row["dex"],
        "agi": row["agi"],
        "sta": row["sta"],
        "wis": row["wis"],
        "int": row["int"],
        "cha": row["cha"],
    }
    return Character(
        id=row["id"],
        name=row["name"],
        level=row["level"],
        cls=row["class"],
        base_stats=base_stats,
    )


def fetch_items(conn, char_id: int) -> List[Dict]:
    cur = conn.cursor(dictionary=True)
    cur.execute(
        f"""
        SELECT i.slot_id, i.item_id, it.Name,
               it.astr, it.adex, it.aagi, it.asta, it.awis, it.aint, it.acha,
               it.ac, it.hp, it.mana, it.endur
        FROM inventory i
        JOIN items it ON it.id = i.item_id
        WHERE i.character_id = %s AND i.item_id > 0
        """,
        (char_id,),
    )
    return cur.fetchall()


def infer_item_tier(name: str) -> Optional[int]:
    """
    Heuristic: look for a trailing "+ <number>" (e.g., "+17") and treat that as the item tier/level.
    Returns None if no match.
    """
    parts = name.rsplit("+", 1)
    if len(parts) != 2:
        return None
    suffix = parts[1].strip()
    if not suffix.isdigit():
        return None
    return int(suffix)


def average_item_tier(items: List[Dict]) -> Optional[float]:
    tiers = []
    for it in items:
        t = infer_item_tier(it["Name"])
        if t is not None:
            tiers.append(t)
    if not tiers:
        return None
    return sum(tiers) / len(tiers)


def aggregate_stats(
    char: Character,
    items: List[Dict],
    gear_mult: float,
    mode: str = "auto",
) -> Tuple[Dict[str, float], Dict[str, float], Optional[str]]:
    """
    Returns (totals_used, item_totals, warning)
    mode:
      - auto: if char stats >> item stats, assume char stats already include gear (use char only)
      - char: use character_data stats only
      - items: use items only
      - sum: add base + items
    """
    base = {k: float(v) for k, v in char.base_stats.items()}
    # ensure non-stat fields exist for downstream printing
    for extra in ["ac", "hp", "mana", "endur"]:
        base.setdefault(extra, 0.0)
    item_totals = {"str": 0.0, "dex": 0.0, "agi": 0.0, "sta": 0.0, "wis": 0.0, "int": 0.0, "cha": 0.0, "ac": 0.0, "hp": 0.0, "mana": 0.0, "endur": 0.0}
    for it in items:
        item_totals["str"] += it["astr"] * gear_mult
        item_totals["dex"] += it["adex"] * gear_mult
        item_totals["agi"] += it["aagi"] * gear_mult
        item_totals["sta"] += it["asta"] * gear_mult
        item_totals["wis"] += it["awis"] * gear_mult
        item_totals["int"] += it["aint"] * gear_mult
        item_totals["cha"] += it["acha"] * gear_mult
        item_totals["ac"] += it["ac"] * gear_mult
        item_totals["hp"] += it["hp"] * gear_mult
        item_totals["mana"] += it["mana"] * gear_mult
        item_totals["endur"] += it["endur"] * gear_mult

    warn = None
    if mode == "char":
        totals = base
    elif mode == "items":
        totals = item_totals
    elif mode == "sum":
        totals = {k: base.get(k, 0.0) + item_totals.get(k, 0.0) for k in set(base) | set(item_totals)}
    else:  # auto
        sum_base = sum(base[k] for k in STAT_FIELDS)
        sum_items = sum(item_totals[k] for k in STAT_FIELDS)
        # Conservative default: assume character_data stats may already include gear if they are close to item totals.
        if sum_base >= sum_items * 0.8:
            totals = base
            warn = "auto-mode: using character_data stats only (base stats are close to item totals; use --mode sum if you want base+items)"
        else:
            totals = {k: base.get(k, 0.0) + item_totals.get(k, 0.0) for k in set(base) | set(item_totals)}
            warn = "auto-mode: summing base + items (base stats well below item stats)"
    return totals, item_totals, warn


def derived(stats: Dict[str, float], level: int) -> Dict[str, float]:
    STR_LEVEL_DIV = 10.0
    STR_MIN_MULT = 0.1
    DEX_CRIT_DIV = 500.0
    DEX_CRIT_DMG_DIV = 20.0
    DEX_RESIST_DIV = 10.0
    DEX_RESIST_CAP = 120.0
    CHA_RESIST_DIV = 10.0
    CHA_RESIST_CAP = 120.0
    RESIST_SHARED_CAP = 180.0
    CHA_RARE_DIV = 20.0
    CHA_RARE_CAP = 50.0
    CHA_DMG_DIV = 2500.0
    CHA_DMG_CAP = 35.0
    AGI_HASTE_CAP = 100.0
    AGI_HASTE_DIV = 400.0
    AGI_AVOID_CAP = 75.0
    AGI_AVOID_DIV = 400.0
    RUN_CAP = 110.0
    RUN_DIV = 200.0
    STA_HP_SCALAR = 0.18  # rough conservative scalar until code constants exist
    # Core math
    str_mult = max(level / STR_LEVEL_DIV, STR_MIN_MULT)
    str_bonus = stats["str"] * str_mult
    crit_chance_raw = (stats["dex"] * level) / DEX_CRIT_DIV
    crit_chance = min(crit_chance_raw, 100.0)
    crit_overflow = max(crit_chance_raw - 100.0, 0.0)
    base_crit_dmg = stats["dex"] / DEX_CRIT_DMG_DIV
    dex_resist = min(stats["dex"] / DEX_RESIST_DIV, DEX_RESIST_CAP)
    cha_resist = min(stats["cha"] / CHA_RESIST_DIV, CHA_RESIST_CAP)
    resist_pen = min(dex_resist + cha_resist, RESIST_SHARED_CAP)
    rare_bonus = min(stats["cha"] / CHA_RARE_DIV, CHA_RARE_CAP)
    dmg_reduct = min((stats["cha"] * level) / CHA_DMG_DIV, CHA_DMG_CAP)
    haste = AGI_HASTE_CAP * stats["agi"] / (stats["agi"] + AGI_HASTE_DIV)
    avoid = AGI_AVOID_CAP * stats["agi"] / (stats["agi"] + AGI_AVOID_DIV)
    run = RUN_CAP * stats["agi"] / (stats["agi"] + RUN_DIV)
    hp_from_sta = stats["sta"] * level * STA_HP_SCALAR
    # Multi-proc expectation
    procs = [stats["dex"] / (stats["dex"] + d) for d in (100.0, 200.0, 400.0, 1500.0)]
    exp_procs = 0.0
    prob = 1.0
    for p in procs:
        exp_procs += prob * p
        prob *= p
    return {
        "str_bonus": str_bonus,
        "crit_chance": crit_chance,
        "crit_overflow": crit_overflow,
        "base_crit_dmg": base_crit_dmg,
        "resist_pen": resist_pen,
        "rare_bonus": rare_bonus,
        "dmg_reduct": dmg_reduct,
        "haste": haste,
        "avoid": avoid,
        "run": run,
        "hp_from_sta": hp_from_sta,
        "exp_procs": exp_procs,
    }


def main():
    parser = argparse.ArgumentParser(description="Stat simulator for EQEmu character.")
    parser.add_argument("--config", default=DEFAULT_CONFIG_PATH, help="Path to eqemu_config.json")
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--name", help="Character name")
    group.add_argument("--id", type=int, help="Character id")
    parser.add_argument("--gear-mult", type=float, default=1.0, help="Multiplier to scale item stats (simulate higher-tier gear)")
    parser.add_argument("--target-level", type=int, help="Override level for simulation")
    parser.add_argument("--show-items", action="store_true", help="Print item breakdown")
    parser.add_argument("--mode", choices=["auto", "char", "items", "sum"], default="auto",
                        help="How to combine character_data stats with item stats (default auto heuristic)")
    args = parser.parse_args()

    cfg = load_config(args.config)
    try:
        conn = connect_db(cfg)
    except Exception as e:
        raise SystemExit(f"[error] DB connection failed: {e}")

    char = fetch_character(conn, args.name, args.id)
    items = fetch_items(conn, char.id)
    totals, item_totals, warn = aggregate_stats(char, items, args.gear_mult, args.mode)
    avg_tier = average_item_tier(items)
    sim_level = args.target_level or char.level
    d = derived(totals, sim_level)

    print(f"Character: {char.name} (id {char.id}) class {char.cls} level {char.level} -> sim level {sim_level}")
    print(f"Base stats: {char.base_stats}")
    print(f"Item count: {len(items)} gear_mult={args.gear_mult}")
    if avg_tier is not None:
        print(f"Average inferred item tier: {avg_tier:.1f}")
    else:
        print("Average inferred item tier: n/a (no '+ <num>' suffixes found)")
    if warn:
        print(f"Mode: {args.mode} ({warn})")
    else:
        print(f"Mode: {args.mode}")
    print("Item-only totals:")
    for k in ["str", "dex", "agi", "sta", "wis", "int", "cha", "ac", "hp", "mana", "endur"]:
        print(f"  {k.upper():4}: {item_totals[k]:.1f}")
    print("Totals (base+items):")
    for k in ["str", "dex", "agi", "sta", "wis", "int", "cha", "ac", "hp", "mana", "endur"]:
        print(f"  {k.upper():4}: {totals[k]:.1f}")
    print("Derived:")
    print(f"  STR bonus to base dmg: {d['str_bonus']:.1f}")
    print(f"  Crit chance: {d['crit_chance']:.1f}%  overflow: {d['crit_overflow']:.1f}%  base crit dmg: {d['base_crit_dmg']:.1f}%")
    print(f"  Resist penetration (DEX+CHA, capped): {d['resist_pen']:.1f}")
    print(f"  Rare loot bonus (CHA): {d['rare_bonus']:.1f}%  incoming dmg reduction (CHA): {d['dmg_reduct']:.1f}%")
    print(f"  Haste from AGI: {d['haste']:.1f}%  Avoidance: {d['avoid']:.1f}%  Run speed: {d['run']:.1f}%")
    print(f"  HP from STA (approx): {d['hp_from_sta']:.1f}")
    print(f"  Expected procs per swing (DEX chain): {d['exp_procs']:.2f}")

    if args.show_items:
        print("\nItems:")
        for it in sorted(items, key=lambda x: x["slot_id"]):
            print(
                f"  slot {it['slot_id']:>4} | {it['Name']:<32} | "
                f"STR {it['astr']} DEX {it['adex']} AGI {it['aagi']} STA {it['asta']} "
                f"WIS {it['awis']} INT {it['aint']} CHA {it['acha']} AC {it['ac']} HP {it['hp']} Mana {it['mana']} End {it['endur']}"
            )


if __name__ == "__main__":
    main()
