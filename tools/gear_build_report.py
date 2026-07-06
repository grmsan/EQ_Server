#!/usr/bin/env python3
"""
Reusable gear-build report tool.

Purpose:
  - Pull a real loot-table item pool for a chosen expansion and level cap.
  - Build a best-fit character set for any playable class and any stat focus.
  - Apply base/Enchanted/Legendary/Mythic item scaling for balance-side analysis.

This is a design utility, not a full gameplay simulator. It intentionally uses
simple, explicit scoring rules that are easy to tune.
"""

from __future__ import annotations

import argparse
import json
import os
from dataclasses import dataclass

import mysql.connector


ALL_CLASSES_BITMASK = 65535
WEAPON_ITEM_TYPES = {0, 1, 2, 3, 4, 5, 35, 45}
TWO_HANDED_TYPES = {1, 4, 5, 35}
GEAR_ITEM_TYPES = sorted(WEAPON_ITEM_TYPES | {8, 10})
ATTR_COLUMNS = ("astr", "asta", "aagi", "adex", "aint", "awis", "acha")
ATTR_LABELS = {
    "astr": "STR",
    "asta": "STA",
    "aagi": "AGI",
    "adex": "DEX",
    "aint": "INT",
    "awis": "WIS",
    "acha": "CHA",
}
FOCUS_TO_ATTR = {
    "str": "astr",
    "sta": "asta",
    "agi": "aagi",
    "dex": "adex",
    "int": "aint",
    "wis": "awis",
    "cha": "acha",
}
CLASS_IDS = {
    "warrior": 1,
    "cleric": 2,
    "paladin": 3,
    "ranger": 4,
    "shadowknight": 5,
    "druid": 6,
    "monk": 7,
    "bard": 8,
    "rogue": 9,
    "shaman": 10,
    "necromancer": 11,
    "wizard": 12,
    "magician": 13,
    "enchanter": 14,
    "beastlord": 15,
    "berserker": 16,
}
DEFAULT_FOCUS_BY_CLASS = {
    "warrior": "str",
    "cleric": "wis",
    "paladin": "str",
    "ranger": "dex",
    "shadowknight": "str",
    "druid": "wis",
    "monk": "agi",
    "bard": "dex",
    "rogue": "dex",
    "shaman": "wis",
    "necromancer": "int",
    "wizard": "int",
    "magician": "int",
    "enchanter": "int",
    "beastlord": "dex",
    "berserker": "str",
}
BASE_SHEET_STATS = {
    "astr": 75,
    "asta": 75,
    "aagi": 75,
    "adex": 75,
    "aint": 80,
    "awis": 75,
    "acha": 75,
}

SLOT_BITS = {
    "charm": 1,
    "ear": 2 | 16,
    "head": 4,
    "face": 8,
    "neck": 32,
    "shoulders": 64,
    "arms": 128,
    "back": 256,
    "wrist": 512 | 1024,
    "range": 2048,
    "hands": 4096,
    "primary": 8192,
    "secondary": 16384,
    "finger": 32768 | 65536,
    "chest": 131072,
    "legs": 262144,
    "feet": 524288,
    "waist": 1048576,
    "ammo": 4194304,
}
OUTPUT_ORDER = [
    "head",
    "face",
    "neck",
    "shoulders",
    "back",
    "chest",
    "arms",
    "wrist_1",
    "wrist_2",
    "hands",
    "waist",
    "legs",
    "feet",
    "finger_1",
    "finger_2",
    "ear_1",
    "ear_2",
    "primary",
    "secondary",
    "range",
]
DISPLAY_NAMES = {
    "wrist_1": "wrist 1",
    "wrist_2": "wrist 2",
    "finger_1": "finger 1",
    "finger_2": "finger 2",
    "ear_1": "ear 1",
    "ear_2": "ear 2",
}
MIRRORED_SLOTS = {
    "wrist": ("wrist_1", "wrist_2"),
    "finger": ("finger_1", "finger_2"),
    "ear": ("ear_1", "ear_2"),
}
SINGLE_SLOTS = ("head", "face", "neck", "shoulders", "back", "chest", "arms", "hands", "waist", "legs", "feet")

TIER_CONFIG = {
    "base": {"combat_mult": 1.0, "attr_mult": 1.0, "haste_bonus": 0, "spell_mult": 0, "heal_mult": 0, "weapon_attack_mode": "base"},
    "enchanted": {"combat_mult": 2.0, "attr_mult": 2.0, "haste_bonus": 3, "spell_mult": 1, "heal_mult": 1, "weapon_attack_mode": "scaled"},
    "legendary": {"combat_mult": 2.6, "attr_mult": 2.0, "haste_bonus": 5, "spell_mult": 2, "heal_mult": 2, "weapon_attack_mode": "damage_plus"},
    "mythic": {"combat_mult": 2.6, "attr_mult": 2.0, "haste_bonus": 5, "spell_mult": 2, "heal_mult": 2, "weapon_attack_mode": "damage_plus"},
}


@dataclass
class Item:
    id: int
    name: str
    loregroup: int
    slots: int
    itemtype: int
    damage: int
    delay: int
    ac: int
    hp: int
    mana: int
    attack: int
    haste: int
    astr: int
    asta: int
    aagi: int
    adex: int
    aint: int
    awis: int
    acha: int
    mr: int
    fr: int
    cr: int
    dr: int
    pr: int
    classes: int
    magic: int
    reclevel: int

    @property
    def ratio(self) -> float:
        return (self.damage / self.delay) if self.delay else 0.0

    @property
    def all_stats(self) -> int:
        return sum(getattr(self, col) for col in ATTR_COLUMNS)

    @property
    def resists(self) -> int:
        return self.mr + self.fr + self.cr + self.dr + self.pr

    @property
    def is_lore(self) -> bool:
        return self.loregroup < 0

    @property
    def has_shared_loregroup(self) -> bool:
        return self.loregroup > 0

    @property
    def is_weapon(self) -> bool:
        return self.itemtype in WEAPON_ITEM_TYPES

    @property
    def is_two_handed_or_bow(self) -> bool:
        return self.itemtype in TWO_HANDED_TYPES


def get_connection():
    config_path = os.path.join(os.path.dirname(__file__), "..", "eqemu_config.json")
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


def class_bitmask(class_name: str) -> int:
    return 1 << (CLASS_IDS[class_name] - 1)


def fetch_items(expansion: int, max_level: int, class_name: str, include_tiered: bool) -> list[Item]:
    conn = get_connection()
    cur = conn.cursor(dictionary=True)
    tier_clause = ""
    if not include_tiered:
        tier_clause = """
          AND i.Name NOT LIKE '% (Enchanted)'
          AND i.Name NOT LIKE '% (Legendary)'
          AND i.Name NOT LIKE '% (Mythic)'
        """

    query = f"""
        SELECT DISTINCT
            i.id,
            i.Name AS name,
            i.loregroup,
            i.slots,
            i.itemtype,
            i.damage,
            i.delay,
            i.ac,
            i.hp,
            i.mana,
            i.attack,
            i.haste,
            i.astr,
            i.asta,
            i.aagi,
            i.adex,
            i.aint,
            i.awis,
            i.acha,
            i.mr,
            i.fr,
            i.cr,
            i.dr,
            i.pr,
            i.classes,
            i.magic,
            i.reclevel
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
          AND z.expansion = %s
          AND i.itemtype IN ({",".join(str(t) for t in GEAR_ITEM_TYPES)})
          AND (i.classes = %s OR (i.classes & %s) = %s)
          AND (i.damage > 0 OR i.ac > 0)
          AND i.Name NOT LIKE 'Summoned:%%'
          AND i.Name NOT LIKE 'Fabled%%'
          AND i.reclevel <= %s
          {tier_clause}
    """
    bitmask = class_bitmask(class_name)
    cur.execute(query, (expansion, ALL_CLASSES_BITMASK, bitmask, bitmask, max_level))
    rows = [Item(**row) for row in cur.fetchall()]
    cur.close()
    conn.close()
    return rows


def build_weights(focus_attr: str, weapon_bias: str) -> dict[str, float]:
    weights = {
        "astr": 2.5,
        "asta": 2.0,
        "aagi": 1.5,
        "adex": 2.5,
        "aint": 1.5,
        "awis": 1.5,
        "acha": 0.25,
        "ac": 1.2,
        "hp": 0.35,
        "mana": 0.08,
        "attack": 1.5,
        "haste": 6.0,
        "resists": 0.35,
        "broad": 0.75,
        "weapon_damage": 6.0,
        "weapon_ratio": 180.0,
        "bow_damage": 5.0,
        "bow_ratio": 250.0,
    }
    weights[focus_attr] = 8.0

    if focus_attr == "aint":
        weights["mana"] = 0.18
    elif focus_attr == "awis":
        weights["mana"] = 0.15
    elif focus_attr == "astr":
        weights["attack"] = 2.25
    elif focus_attr == "adex":
        weights["attack"] = 1.75

    if weapon_bias == "ratio":
        weights["weapon_ratio"] *= 1.35
        weights["bow_ratio"] *= 1.35
    elif weapon_bias == "damage":
        weights["weapon_damage"] *= 1.5
        weights["bow_damage"] *= 1.5

    return weights


def slot_score(item: Item, slot_name: str, focus_attr: str, weights: dict[str, float]) -> float:
    score = 0.0
    for attr in ATTR_COLUMNS:
        score += getattr(item, attr) * weights[attr]

    score += item.ac * weights["ac"]
    score += item.hp * weights["hp"]
    score += item.mana * weights["mana"]
    score += item.attack * weights["attack"]
    score += item.haste * weights["haste"]
    score += item.resists * weights["resists"]

    # Broad-stat bias so "9 focus + broad support" can beat "10 focus only".
    score += max(0, item.all_stats - getattr(item, focus_attr)) * weights["broad"]

    if slot_name == "range":
        score += item.ratio * weights["bow_ratio"]
        score += item.damage * weights["bow_damage"]
    elif slot_name in {"primary", "secondary"}:
        score += item.damage * weights["weapon_damage"]
        score += item.ratio * weights["weapon_ratio"]

    return score


def scale_value(base_value: int, multiplier: float) -> int:
    return int(round(base_value * multiplier))


def tier_view(item: Item, tier_name: str) -> dict[str, int]:
    tier = TIER_CONFIG[tier_name]

    if tier["weapon_attack_mode"] == "base":
        attack_value = item.attack
    elif tier["weapon_attack_mode"] == "scaled":
        attack_value = scale_value(item.attack, tier["combat_mult"]) if item.attack > 0 else 0
    else:
        attack_value = item.attack + (item.damage * 2) if item.damage > 0 else scale_value(item.attack, tier["combat_mult"])

    data = {
        "ac": scale_value(item.ac, tier["combat_mult"]),
        "hp": scale_value(item.hp, tier["combat_mult"]),
        "mana": scale_value(item.mana, tier["combat_mult"]),
        "attack": attack_value,
        "haste": item.haste + tier["haste_bonus"] if item.haste > 0 else 0,
        "spelldmg": item.aint * tier["spell_mult"] if item.aint > 0 else 0,
        "healamt": item.awis * tier["heal_mult"] if item.awis > 0 else 0,
        "damage": scale_value(item.damage, tier["combat_mult"]) if item.damage > 0 and tier_name != "base" else item.damage,
        "delay": item.delay,
        "mr": scale_value(item.mr, tier["attr_mult"]) if item.mr > 0 else item.mr,
        "fr": scale_value(item.fr, tier["attr_mult"]) if item.fr > 0 else item.fr,
        "cr": scale_value(item.cr, tier["attr_mult"]) if item.cr > 0 else item.cr,
        "dr": scale_value(item.dr, tier["attr_mult"]) if item.dr > 0 else item.dr,
        "pr": scale_value(item.pr, tier["attr_mult"]) if item.pr > 0 else item.pr,
    }
    for attr in ATTR_COLUMNS:
        base_value = getattr(item, attr)
        data[attr] = scale_value(base_value, tier["attr_mult"]) if base_value > 0 else base_value
    return data


def choose_unique(candidates: list[Item], count: int, slot_name: str, focus_attr: str, weights: dict[str, float]) -> list[Item]:
    ranked = sorted(candidates, key=lambda current: slot_score(current, slot_name, focus_attr, weights), reverse=True)
    chosen: list[Item] = []
    used_ids: set[int] = set()
    used_loregroups: set[int] = set()

    for item in ranked:
        if item.id in used_ids:
            continue
        if item.has_shared_loregroup and item.loregroup in used_loregroups:
            continue
        chosen.append(item)
        used_ids.add(item.id)
        if item.has_shared_loregroup:
            used_loregroups.add(item.loregroup)
        if len(chosen) >= count:
            break

    if len(chosen) < count:
        for item in ranked:
            if item.is_lore:
                continue
            chosen.append(item)
            if len(chosen) >= count:
                break

    return chosen


def best_item(candidates: list[Item], slot_name: str, focus_attr: str, weights: dict[str, float]) -> Item | None:
    if not candidates:
        return None
    return max(candidates, key=lambda item: slot_score(item, slot_name, focus_attr, weights))


def build_set(items: list[Item], focus_attr: str, weights: dict[str, float], prefer_bow: bool) -> dict[str, Item]:
    by_slot: dict[str, list[Item]] = {}
    for slot_name, bitmask in SLOT_BITS.items():
        by_slot[slot_name] = [item for item in items if item.slots & bitmask]

    build: dict[str, Item] = {}

    for mirrored_name, target_slots in MIRRORED_SLOTS.items():
        picks = choose_unique(by_slot[mirrored_name], len(target_slots), mirrored_name, focus_attr, weights)
        if len(picks) < len(target_slots) and by_slot[mirrored_name]:
            fallback = sorted(by_slot[mirrored_name], key=lambda item: slot_score(item, mirrored_name, focus_attr, weights), reverse=True)
            while len(picks) < len(target_slots):
                picks.append(fallback[min(len(picks), len(fallback) - 1)])
        for output_slot, item in zip(target_slots, picks):
            build[output_slot] = item

    for slot_name in SINGLE_SLOTS:
        item = best_item(by_slot[slot_name], slot_name, focus_attr, weights)
        if item:
            build[slot_name] = item

    range_candidates = by_slot["range"]
    if prefer_bow:
        range_candidates = [item for item in range_candidates if item.itemtype == 5]
    range_item = best_item(range_candidates, "range", focus_attr, weights)
    if range_item:
        build["range"] = range_item

    primary_candidates = [item for item in by_slot["primary"] if item.is_weapon]
    offhand_candidates = [item for item in by_slot["secondary"] if item.is_weapon]
    primary_item = best_item(primary_candidates, "primary", focus_attr, weights)
    if primary_item:
        build["primary"] = primary_item

    if primary_item and primary_item.is_two_handed_or_bow:
        return build

    secondary_sorted = sorted(offhand_candidates, key=lambda item: slot_score(item, "secondary", focus_attr, weights), reverse=True)
    for item in secondary_sorted:
        if primary_item and item.id == primary_item.id:
            continue
        if primary_item and item.has_shared_loregroup and primary_item.has_shared_loregroup and item.loregroup == primary_item.loregroup:
            continue
        build["secondary"] = item
        break

    return build


def sum_totals(build: dict[str, Item], tier_name: str) -> dict[str, int]:
    totals = {
        "astr": 0,
        "asta": 0,
        "aagi": 0,
        "adex": 0,
        "aint": 0,
        "awis": 0,
        "acha": 0,
        "ac": 0,
        "hp": 0,
        "mana": 0,
        "attack": 0,
        "haste": 0,
        "spelldmg": 0,
        "healamt": 0,
        "mr": 0,
        "fr": 0,
        "cr": 0,
        "dr": 0,
        "pr": 0,
    }
    for item in build.values():
        tiered = tier_view(item, tier_name)
        for key in totals:
            totals[key] += tiered[key]
    return totals


def render_stat_snapshot(item: Item, tiered: dict[str, int], include_ratio: bool) -> tuple[str, str]:
    base_parts = [
        f"DEX {item.adex}",
        f"STR {item.astr}",
        f"STA {item.asta}",
        f"AGI {item.aagi}",
        f"INT {item.aint}",
        f"WIS {item.awis}",
        f"CHA {item.acha}",
        f"AC {item.ac}",
        f"HP {item.hp}",
    ]
    if item.mana:
        base_parts.append(f"Mana {item.mana}")
    if item.haste:
        base_parts.append(f"Haste {item.haste}")
    if item.attack:
        base_parts.append(f"ATK {item.attack}")
    if item.is_weapon:
        base_parts.append(f"DMG {item.damage}")
        base_parts.append(f"Delay {item.delay}")
        if include_ratio and item.delay:
            base_parts.append(f"Ratio {item.ratio:.3f}")

    tier_parts = [
        f"DEX {tiered['adex']}",
        f"STR {tiered['astr']}",
        f"STA {tiered['asta']}",
        f"AGI {tiered['aagi']}",
        f"INT {tiered['aint']}",
        f"WIS {tiered['awis']}",
        f"CHA {tiered['acha']}",
        f"AC {tiered['ac']}",
        f"HP {tiered['hp']}",
    ]
    if tiered["mana"]:
        tier_parts.append(f"Mana {tiered['mana']}")
    if tiered["haste"]:
        tier_parts.append(f"Haste {tiered['haste']}")
    if tiered["attack"]:
        tier_parts.append(f"ATK {tiered['attack']}")
    if tiered["spelldmg"]:
        tier_parts.append(f"SD {tiered['spelldmg']}")
    if tiered["healamt"]:
        tier_parts.append(f"Heal {tiered['healamt']}")
    if item.is_weapon:
        tier_parts.append(f"DMG {tiered['damage']}")
        tier_parts.append(f"Delay {tiered['delay']}")

    return ", ".join(base_parts), ", ".join(tier_parts)


def print_report(build: dict[str, Item], class_name: str, expansion: int, max_level: int, focus: str, tier_name: str):
    print(f"# Gear Build Report: {class_name.title()} / Expansion {expansion} / Level {max_level} / Focus {focus.upper()} / {tier_name.title()}")
    print()
    print("Method")
    print(f"- Base pool: `expansion = {expansion}` loot-table items only.")
    print(f"- Filters: `{class_name}`-usable, `reclevel <= {max_level}`, base items only.")
    print(f"- Focus stat: `{focus.upper()}`.")
    print("- Scoring: focus-first, but broad stats/AC/HP/haste/resists are kept meaningful.")
    print()
    print("| Slot | Item | Base Stats | Tier Snapshot |")
    print("|---|---|---|---|")

    for slot_name in OUTPUT_ORDER:
        item = build.get(slot_name)
        if not item:
            continue
        tiered = tier_view(item, tier_name)
        slot_label = DISPLAY_NAMES.get(slot_name, slot_name)
        base_text, tier_text = render_stat_snapshot(item, tiered, include_ratio=True)
        print(f"| {slot_label} | {item.name} | {base_text} | {tier_text} |")

    totals = sum_totals(build, tier_name)
    print()
    print(f"## {tier_name.title()} Gear Totals")
    print()
    for attr in ATTR_COLUMNS:
        print(f"- {ATTR_LABELS[attr]}: {totals[attr]}")
    print(f"- AC: {totals['ac']}")
    print(f"- HP: {totals['hp']}")
    print(f"- Mana: {totals['mana']}")
    print(f"- Attack: {totals['attack']}")
    print(f"- Haste: {totals['haste']}")
    print(f"- Spell Damage: {totals['spelldmg']}")
    print(f"- Heal Power: {totals['healamt']}")
    print(f"- Resists: MR {totals['mr']}, FR {totals['fr']}, CR {totals['cr']}, DR {totals['dr']}, PR {totals['pr']}")
    print()
    print("## Sheet Totals With Baseline Stats")
    print()
    for attr in ATTR_COLUMNS:
        print(f"- {ATTR_LABELS[attr]}: {BASE_SHEET_STATS[attr] + totals[attr]}")


def parse_args():
    parser = argparse.ArgumentParser(description="Build reusable gear reports by class, expansion, and focus stat.")
    parser.add_argument("--class", dest="class_name", choices=sorted(CLASS_IDS), required=True, help="Playable class to build for")
    parser.add_argument("--expansion", type=int, required=True, help="DB expansion id to evaluate (0=Classic, 1=Kunark, etc.)")
    parser.add_argument("--max-level", type=int, required=True, help="Maximum recommended level on items to include")
    parser.add_argument("--focus", choices=sorted(FOCUS_TO_ATTR), help="Primary stat focus. Defaults by class if omitted.")
    parser.add_argument("--tier", choices=sorted(TIER_CONFIG), default="legendary", help="Tier math to apply for totals/output")
    parser.add_argument("--weapon-bias", choices=("balanced", "ratio", "damage"), default="balanced", help="How hard to push weapon ratio vs raw damage")
    parser.add_argument("--allow-ranged-nonbows", action="store_true", help="Allow non-bow range items to compete in the range slot")
    parser.add_argument("--include-tiered", action="store_true", help="Include generated Enchanted/Legendary/Mythic rows in the source pool")
    return parser.parse_args()


def main():
    args = parse_args()
    focus = args.focus or DEFAULT_FOCUS_BY_CLASS[args.class_name]
    focus_attr = FOCUS_TO_ATTR[focus]
    weights = build_weights(focus_attr, args.weapon_bias)
    items = fetch_items(args.expansion, args.max_level, args.class_name, args.include_tiered)
    build = build_set(items, focus_attr, weights, prefer_bow=not args.allow_ranged_nonbows)
    print_report(build, args.class_name, args.expansion, args.max_level, focus, args.tier)


if __name__ == "__main__":
    main()
