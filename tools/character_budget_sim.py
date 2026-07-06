#!/usr/bin/env python3
"""
Character budget combat simulator.

Purpose:
  - Start from a target character sheet budget instead of loot-table averages.
  - Project what a focused primary stat looks like for rough combat output.
  - Keep weapon/spell base values and target mitigation explicit.

This is intentionally a design-side approximation tool, not a source-of-truth
replica of every combat code path.
"""

from __future__ import annotations

import argparse
from dataclasses import dataclass


STAT_ORDER = ("str", "sta", "agi", "dex", "int", "wis", "cha")
CLASS_PRIMARY = {
    "warrior": "str",
    "ranger": "dex",
    "wizard": "int",
}

BUILD_PRESETS = {
    "classic_30_focus": {"level": 30, "primary": 200, "other": 100},
    "classic_50_focus": {"level": 50, "primary": 300, "other": 150},
    "kunark_60_focus": {"level": 60, "primary": 400, "other": 200},
    "velious_60_focus": {"level": 60, "primary": 475, "other": 225},
    "luclin_65_focus": {"level": 65, "primary": 600, "other": 275},
    "pop_65_focus": {"level": 65, "primary": 700, "other": 325},
}

TARGET_PROFILES = {
    "trash": {"hit_rate": 0.92, "melee_mitigation": 0.18, "spell_land_rate": 0.96, "spell_mitigation": 0.08},
    "named": {"hit_rate": 0.82, "melee_mitigation": 0.32, "spell_land_rate": 0.88, "spell_mitigation": 0.18},
    "raid": {"hit_rate": 0.72, "melee_mitigation": 0.45, "spell_land_rate": 0.76, "spell_mitigation": 0.30},
}

CLASS_MULT = {
    "wizard": 1.5,
    "ranger": 0.8,
    "warrior": 0.5,
}


@dataclass
class Stats:
    str: float
    sta: float
    agi: float
    dex: float
    int: float
    wis: float
    cha: float

    def primary_value(self, stat_name: str) -> float:
        return getattr(self, stat_name)


def build_focused_stats(class_name: str, primary: float, other: float) -> Stats:
    primary_stat = CLASS_PRIMARY[class_name]
    values = {key: other for key in STAT_ORDER}
    values[primary_stat] = primary
    return Stats(**values)


def strength_bonus(level: int, strength: float, str_level_divisor: float = 60.0, min_level_mult: float = 0.05) -> float:
    level_mult = max(level / str_level_divisor, min_level_mult)
    return strength * level_mult


def weapon_delay_bonus(delay: float, two_handed: bool = False) -> float:
    if two_handed:
        return max(0.0, (delay - 30.0) / 2.5)
    return max(0.0, (delay - 40.0) / 3.0)


def crit_components(level: int, dex: float) -> tuple[float, float, float]:
    raw_crit_pct = (dex * level) / 500.0
    crit_chance = min(raw_crit_pct, 100.0) / 100.0
    overflow_pct = max(0.0, raw_crit_pct - 100.0)
    base_crit_bonus_pct = dex / 20.0
    return crit_chance, overflow_pct, base_crit_bonus_pct


def melee_crit_multiplier(level: int, dex: float) -> float:
    _, overflow_pct, base_crit_bonus_pct = crit_components(level, dex)
    return 2.0 + ((overflow_pct + base_crit_bonus_pct) / 100.0)


def ranger_bow_crit_multiplier(level: int, dex: float) -> float:
    return melee_crit_multiplier(level, dex) + (dex / 500.0)


def twincast_chance(level: int, dex: float) -> float:
    return min((dex * level) / 2000.0, 100.0) / 100.0


def spell_damage_multiplier(class_name: str, intelligence: float) -> float:
    class_mult = CLASS_MULT.get(class_name, 1.0)
    spell_mod_pct = (intelligence / 5.0) * class_mult
    return 1.0 + (spell_mod_pct / 100.0)


def wizard_crit_bonus_multiplier(intelligence: float) -> float:
    return (intelligence / 10.0) / 100.0


def cooldown_reduction(intelligence: float) -> float:
    return ((50.0 * intelligence) / (intelligence + 500.0)) / 100.0


def mana_discount(level: int, intelligence: float) -> float:
    return min((intelligence * level) / 2000.0, 50.0) / 100.0


def mana_pool(level: int, intelligence: float, wisdom: float, class_mult: float = 1.0) -> float:
    return (intelligence + wisdom) * level * class_mult


def avg_damage_with_crits(base_damage: float, crit_chance: float, crit_multiplier: float) -> float:
    return base_damage * ((1.0 - crit_chance) + (crit_chance * crit_multiplier))


def avg_damage_with_twincast(base_damage: float, twincast: float) -> float:
    return base_damage * (1.0 + twincast)


def project_warrior(level: int, stats: Stats, weapon_damage: float, weapon_delay: float, swings_per_second: float,
                    hit_rate: float, mitigation: float, two_handed: bool) -> dict[str, float]:
    base = weapon_damage + weapon_delay_bonus(weapon_delay, two_handed) + strength_bonus(level, stats.str)
    crit_chance, _, _ = crit_components(level, stats.dex)
    crit_mult = melee_crit_multiplier(level, stats.dex)
    avg_landed = avg_damage_with_crits(base, crit_chance, crit_mult)
    avg_applied = avg_landed * hit_rate * (1.0 - mitigation)
    return {
        "base_hit": base,
        "crit_chance": crit_chance,
        "crit_multiplier": crit_mult,
        "avg_landed_hit": avg_landed,
        "avg_applied_hit": avg_applied,
        "dps": avg_applied * swings_per_second,
    }


def project_ranger(level: int, stats: Stats, weapon_damage: float, shots_per_second: float,
                   hit_rate: float, mitigation: float) -> dict[str, float]:
    base = weapon_damage + ((stats.dex * level) / 10.0)
    crit_chance, _, _ = crit_components(level, stats.dex)
    crit_mult = ranger_bow_crit_multiplier(level, stats.dex)
    avg_landed = avg_damage_with_crits(base, crit_chance, crit_mult)
    avg_applied = avg_landed * hit_rate * (1.0 - mitigation)
    return {
        "base_shot": base,
        "crit_chance": crit_chance,
        "crit_multiplier": crit_mult,
        "avg_landed_shot": avg_landed,
        "avg_applied_shot": avg_applied,
        "dps": avg_applied * shots_per_second,
    }


def project_wizard(level: int, stats: Stats, spell_base_damage: float, cast_time: float, recast_time: float,
                   spell_land_rate: float, spell_mitigation: float) -> dict[str, float]:
    spell_mult = spell_damage_multiplier("wizard", stats.int)
    dex_crit_chance, _, dex_base_crit_bonus = crit_components(level, stats.dex)
    crit_mult = 2.0 + (dex_base_crit_bonus / 100.0) + wizard_crit_bonus_multiplier(stats.int)
    twin = twincast_chance(level, stats.dex)
    pre_crit = spell_base_damage * spell_mult
    avg_cast = avg_damage_with_crits(pre_crit, dex_crit_chance, crit_mult)
    avg_cast = avg_damage_with_twincast(avg_cast, twin)
    avg_applied_cast = avg_cast * spell_land_rate * (1.0 - spell_mitigation)
    cdr = cooldown_reduction(stats.int)
    cycle_time = cast_time + (recast_time * (1.0 - cdr))
    return {
        "spell_multiplier": spell_mult,
        "crit_chance": dex_crit_chance,
        "crit_multiplier": crit_mult,
        "twincast_chance": twin,
        "avg_landed_cast": avg_cast,
        "avg_applied_cast": avg_applied_cast,
        "cycle_time": cycle_time,
        "dps": avg_applied_cast / max(cycle_time, 0.1),
        "cdr": cdr,
        "mana_discount": mana_discount(level, stats.int),
        "mana_pool": mana_pool(level, stats.int, stats.wis, 1.0),
    }


def print_stats(stats: Stats) -> None:
    print("Sheet Stats")
    for key in STAT_ORDER:
        print(f"- {key.upper()}: {getattr(stats, key):.0f}")


def print_projection(title: str, data: dict[str, float]) -> None:
    print()
    print(title)
    for key, value in data.items():
        if "chance" in key or key in {"cdr", "mana_discount"}:
            print(f"- {key}: {value * 100:.1f}%")
        else:
            print(f"- {key}: {value:.1f}")


def main() -> None:
    parser = argparse.ArgumentParser(description="Budget-first combat simulator for focused stat builds.")
    parser.add_argument("--class", dest="class_name", choices=sorted(CLASS_PRIMARY.keys()), required=True)
    parser.add_argument("--preset", choices=sorted(BUILD_PRESETS.keys()), default="kunark_60_focus")
    parser.add_argument("--primary", type=float, help="Override focused primary stat")
    parser.add_argument("--other", type=float, help="Override non-focused stats")
    parser.add_argument("--level", type=int, help="Override level")
    parser.add_argument("--target", choices=sorted(TARGET_PROFILES.keys()), default="named")

    parser.add_argument("--weapon-damage", type=float, default=None)
    parser.add_argument("--weapon-delay", type=float, default=40.0)
    parser.add_argument("--two-handed", action="store_true")
    parser.add_argument("--swings-per-second", type=float, default=1.25)
    parser.add_argument("--shots-per-second", type=float, default=0.90)

    parser.add_argument("--spell-base-damage", type=float, default=2200.0)
    parser.add_argument("--cast-time", type=float, default=4.0)
    parser.add_argument("--recast-time", type=float, default=2.5)
    args = parser.parse_args()

    preset = BUILD_PRESETS[args.preset]
    level = args.level if args.level is not None else preset["level"]
    primary = args.primary if args.primary is not None else preset["primary"]
    other = args.other if args.other is not None else preset["other"]
    stats = build_focused_stats(args.class_name, primary, other)
    target = TARGET_PROFILES[args.target]

    default_weapon_damage = {
        "warrior": 28.0,
        "ranger": 24.0,
        "wizard": 10.0,
    }
    weapon_damage = args.weapon_damage if args.weapon_damage is not None else default_weapon_damage[args.class_name]

    print(f"Build Preset: {args.preset}")
    print(f"Class: {args.class_name}")
    print(f"Level: {level}")
    print(f"Target Profile: {args.target}")
    print_stats(stats)

    if args.class_name == "warrior":
        projection = project_warrior(
            level=level,
            stats=stats,
            weapon_damage=weapon_damage,
            weapon_delay=args.weapon_delay,
            swings_per_second=args.swings_per_second,
            hit_rate=target["hit_rate"],
            mitigation=target["melee_mitigation"],
            two_handed=args.two_handed,
        )
        print_projection("Warrior Projection", projection)
    elif args.class_name == "ranger":
        projection = project_ranger(
            level=level,
            stats=stats,
            weapon_damage=weapon_damage,
            shots_per_second=args.shots_per_second,
            hit_rate=target["hit_rate"],
            mitigation=target["melee_mitigation"],
        )
        print_projection("Ranger Projection", projection)
    elif args.class_name == "wizard":
        projection = project_wizard(
            level=level,
            stats=stats,
            spell_base_damage=args.spell_base_damage,
            cast_time=args.cast_time,
            recast_time=args.recast_time,
            spell_land_rate=target["spell_land_rate"],
            spell_mitigation=target["spell_mitigation"],
        )
        print_projection("Wizard Projection", projection)


if __name__ == "__main__":
    main()
