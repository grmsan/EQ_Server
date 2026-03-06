#!/usr/bin/env python3
"""
Steps 8 & 9 — Essence Vendors + Basic Augments + Merge System
===============================================================
Generates SQL for:
  - Proc spells (DD, lifetap, heal, mana restore) at all merge levels
  - Leveling weapon augments L1-L5 (Combat/Lifetap/Mana Stones)
  - Endgame stat augments L1-L5 (Stone of Might, etc.)
  - Endgame proc augments L1-L5 (Shard of Flame, etc.)
  - Merge Catalysts (4 tiers)
  - Forgemaster Anvil container (4-slot combine bag)
  - Purified Solvent (safe aug removal)
  - Vendor NPCs in Bazaar (Weaponsmith, Provisioner, Artificer, Forgemaster)
  - Merchant lists with alt-currency, platinum, and RE pricing

Also generates a C++ header (zone/augment_merge_data.h) with lookup constants
used by the Forgemaster merge logic.

Re-run this script at any time to regenerate all SQL + the C++ header.
All values are tunable from the CONFIG section below.

Usage:
    python tools/generate_augments.py
    => writes utils/sql/git/optional/step8_9_augments_and_merge.sql
    => writes zone/augment_merge_data.h
"""

import os, math, datetime

# ============================================================
# CONFIG — Tune all values here, re-run to regenerate
# ============================================================

# --- ID Ranges (structured for merge-level math) ---
# Each aug "family" occupies 5 sequential IDs: base + family*5 + (level-1)
LEVEL_AUG_ID_BASE   = 200100  # Leveling weapon augs (27 families × 5 levels = 135)
STAT_AUG_ID_BASE    = 200300  # Endgame stat augs (7 families × 5 levels = 35)
PROC_AUG_ID_BASE    = 200400  # Endgame proc augs (5 families × 5 levels = 25)
CATALYST_ID_BASE    = 200500  # 4 catalyst items
FORGEMASTER_CONTAINER_ID = 200510
SOLVENT_ID          = 200540

# Spell ID ranges
LEVEL_SPELL_ID_BASE   = 65100   # 27 leveling proc spells (shared across merge levels)
ENDGAME_SPELL_ID_BASE = 65200   # 25 endgame proc spells (5 types × 5 levels)

# NPC / spawn ranges
NPC_ID_BASE      = 181200
SPAWNGROUP_START = 3290000
SPAWN2_START     = 3270000

# --- Currency IDs ---
COMMON_ESSENCE_CURRENCY = 100
RARE_ESSENCE_CURRENCY   = 101

# --- Merge System Constants ---
MAX_AUG_LEVEL    = 5
LEVELS_PER_FAMILY = 5

# Stat aug scaling from AUGMENT_SYSTEM.md §6.3 (index = level-1)
# L1: +3 heroic, +5 secondary → L5: +21, +30
STAT_HEROIC_VALUES    = [3, 6, 10, 15, 21]
STAT_SECONDARY_MULT   = [1.0, 2.0, 3.2, 4.4, 6.0]  # multiplied against L1 secondary

# Proc aug damage scaling from §6.4 (index = level-1)
# L1: 100, L2: 175, L3: 275, L4: 400, L5: 550
PROC_DAMAGE_MULT      = [1.0, 1.75, 2.75, 4.0, 5.5]
# Reuse timer reduction per level (ms to subtract from L1 reuse)
PROC_REUSE_REDUCTION   = [0, 1000, 2000, 3000, 4000]

# Leveling aug merge bonuses (per design: "no increased damage, slight proc rate")
LEVELING_PROCRATE_BONUS = [0, 50, 100, 150, 200]
LEVELING_DMG_BONUS      = [0, 1, 2, 3, 4]

# --- Leveling Weapon Augment Tiers ---
# (required_level, name_prefix, dd_damage, lifetap_heal, mana_restore, plat_cost)
# Non-linear scaling: ramps at higher levels per design
LEVELING_TIERS = [
    (1,  "Chipped",     5,    3,    2,    5),
    (10, "Rough",       15,   8,    5,    25),
    (20, "Honed",       30,   15,   10,   75),
    (30, "Tempered",    50,   25,   18,   150),
    (40, "Polished",    80,   40,   28,   300),
    (50, "Reinforced",  120,  60,   40,   500),
    (60, "Masterwork",  250,  130,  85,   1200),
    (65, "Pristine",    500,  260,  170,  2500),
    (70, "Flawless",    1000, 520,  340,  5000),
]

# Leveling aug base bonus damage (flat +DMG stat)
LEVELING_BONUS_DMG = [1, 2, 3, 4, 5, 6, 8, 10, 13]

NUM_LEVEL_TIERS = len(LEVELING_TIERS)  # 9
NUM_LEVEL_FAMILIES = NUM_LEVEL_TIERS * 3  # 27 (combat, lifetap, mana per tier)

# --- Endgame Stat Augments ---
# (key, name, heroic_field, heroic_base, secondary_field, secondary_base, ce_cost, pp_cost)
ENDGAME_STAT_AUGS = [
    ("might",     "Stone of Might",     "heroic_str", 3, "attack",    5,  500,  500),
    ("fortitude", "Stone of Fortitude", "heroic_sta", 3, "ac",        8,  500,  500),
    ("precision", "Stone of Precision", "heroic_dex", 3, "attack",    5,  500,  500),
    ("evasion",   "Stone of Evasion",   "heroic_agi", 3, "avoidance", 5,  500,  500),
    ("insight",   "Stone of Insight",   "heroic_int", 3, "spelldmg",  8,  500,  500),
    ("devotion",  "Stone of Devotion",  "heroic_wis", 3, "healamt",   8,  500,  500),
    ("presence",  "Stone of Presence",  "heroic_cha", 3, "hp",        15, 500,  500),
]
NUM_STAT_FAMILIES = len(ENDGAME_STAT_AUGS)  # 7

# --- Endgame Proc Augments ---
# (key, name, proc_type, base_value, re_cost, pp_cost)
ENDGAME_PROC_AUGS = [
    ("flame",    "Shard of Flame",    "fire_dd",    100, 500, 1000),
    ("frost",    "Shard of Frost",    "cold_dd",    100, 500, 1000),
    ("venom",    "Shard of Venom",    "poison_dot", 40,  500, 1000),
    ("mending",  "Shard of Mending",  "heal",       150, 500, 1000),
    ("siphon",   "Shard of Siphoning","lifetap",    80,  500, 1000),
]
NUM_PROC_FAMILIES = len(ENDGAME_PROC_AUGS)  # 5

# Total families across all categories
TOTAL_FAMILIES = NUM_LEVEL_FAMILIES + NUM_STAT_FAMILIES + NUM_PROC_FAMILIES  # 39

# --- Proc Properties ---
PROC_REUSE = {
    "fire_dd":    8000,   # ms
    "cold_dd":    8000,
    "poison_dot": 10000,
    "heal":       12000,
    "lifetap":    10000,
}
PROC_RESIST = {
    "fire_dd":    2,  # fire
    "cold_dd":    3,  # cold
    "poison_dot": 4,  # poison
    "heal":       0,  # unresistable
    "lifetap":    1,  # magic
}
PROC_TARGET = {
    "fire_dd":    5,   # single target
    "cold_dd":    5,
    "poison_dot": 5,
    "heal":       6,   # self
    "lifetap":    13,  # lifetap (damage + self heal)
}

# --- Merge Catalysts ---
# (name, ce_cost, pp_cost, re_cost)  0 = not sold for that currency
CATALYSTS = [
    ("Lesser Merge Catalyst",   250,  100000, 0),     # L1→2: 250 CE or 100pp
    ("Merge Catalyst",          750,  300000, 0),     # L2→3: 750 CE or 300pp
    ("Greater Merge Catalyst",  0,    0,      50),    # L3→4: 50 RE
    ("Superior Merge Catalyst", 0,    0,      125),   # L4→5: 125 RE
]

# --- Augment Item Defaults ---
AUG_ITEM_DEFAULTS = {
    "itemtype":   54,       # augmentation
    "magic":      1,
    "nodrop":     0,        # tradeable
    "norent":     1,
    "weight":     0,
    "size":       0,        # tiny
    "icon":       1057,     # generic gem icon
    "idfile":     "IT63",   # generic augment model
    "classes":    65535,    # all classes
    "races":      65535,    # all races
    "slots":      0,        # aug doesn't equip in armor slots
}

# Weapon aug type: AugType bitmask (8 = WeaponGeneral)
WEAPON_AUG_TYPE = 8
# General stat aug type: AugType bitmask (1 = GeneralSingleStat)
STAT_AUG_TYPE   = 1

# Merchant list defaults
FACTION_REQUIRED = -100

# --- Vendor NPCs ---
VENDOR_NPCS = [
    {
        "id":           NPC_ID_BASE,
        "name":         "Augment_Weaponsmith",
        "merchant_id":  NPC_ID_BASE,
        "last_name":    "Leveling Augments",
        "x":            -100.0,
        "y":            -840.0,
        "z":            3.44,
        "heading":      0.0,
        "race":         1,      # human
        "gender":       0,
        "texture":      3,      # plate armor
        "level":        65,
        "alt_currency": 0,      # platinum only
    },
    {
        "id":           NPC_ID_BASE + 1,
        "name":         "Essence_Provisioner",
        "merchant_id":  NPC_ID_BASE + 1,
        "last_name":    "Essence Augments",
        "x":            -110.0,
        "y":            -840.0,
        "z":            3.44,
        "heading":      0.0,
        "race":         1,
        "gender":       1,
        "texture":      3,
        "level":        65,
        "alt_currency": COMMON_ESSENCE_CURRENCY,
    },
    {
        "id":           NPC_ID_BASE + 2,
        "name":         "Essence_Artificer",
        "merchant_id":  NPC_ID_BASE + 2,
        "last_name":    "Rare Augments",
        "x":            -120.0,
        "y":            -840.0,
        "z":            3.44,
        "heading":      0.0,
        "race":         1,
        "gender":       0,
        "texture":      7,      # robes
        "level":        65,
        "alt_currency": RARE_ESSENCE_CURRENCY,
    },
    {
        "id":           NPC_ID_BASE + 3,
        "name":         "Augment_Forgemaster",
        "merchant_id":  NPC_ID_BASE + 3,
        "last_name":    "Augment Merging",
        "x":            -130.0,
        "y":            -840.0,
        "z":            3.44,
        "heading":      0.0,
        "race":         1,
        "gender":       0,
        "texture":      3,      # plate armor
        "level":        65,
        "alt_currency": COMMON_ESSENCE_CURRENCY,
    },
]

# ============================================================
# ICON MAP
# ============================================================
ICON_MAP = {
    "combat":    1058,  # red
    "lifetap":   1062,  # purple
    "mana":      1063,  # grey
    "flame":     1058,  # red
    "frost":     1059,  # blue
    "venom":     1060,  # green
    "mending":   1061,  # yellow
    "siphon":    1062,  # purple
    "might":     1057,  # white
    "fortitude": 1057,
    "precision": 1057,
    "evasion":   1057,
    "insight":   1059,  # blue
    "devotion":  1061,  # yellow
    "presence":  1057,
    "catalyst":  1064,  # orange/catalyst
    "container": 677,   # anvil/forge
    "solvent":   852,   # potion
}

# Merge level suffixes for item names
LEVEL_SUFFIX = {1: "", 2: " II", 3: " III", 4: " IV", 5: " V"}

# ============================================================
# OUTPUT PATHS
# ============================================================
REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

SQL_OUTPUT = os.path.join(
    REPO_ROOT, "utils", "sql", "git", "optional",
    "step8_9_augments_and_merge.sql",
)

CPP_HEADER_OUTPUT = os.path.join(
    REPO_ROOT, "zone", "augment_merge_data.h",
)


# ============================================================
# SQL HELPERS (unchanged from Step 8)
# ============================================================

def sql_str(s):
    """Escape a string for SQL single-quoted value."""
    return s.replace("'", "''")


def spell_insert_sql(vals):
    """Convert a vals dict to an INSERT statement for spells_new."""
    cols = sorted(vals.keys())
    col_str = ", ".join(f"`{c}`" for c in cols)
    val_str = ", ".join(str(vals[c]) for c in cols)
    return f"INSERT INTO `spells_new` ({col_str}) VALUES ({val_str});"


def item_insert_sql(vals):
    """Convert a vals dict to an INSERT statement for items."""
    cols = sorted(vals.keys())
    col_str = ", ".join(f"`{c}`" for c in cols)
    val_str = ", ".join(str(vals[c]) for c in cols)
    return f"INSERT INTO `items` ({col_str}) VALUES ({val_str});"


def build_simple_spell(spell_id, name, damage, resist, target, reuse_ms,
                       is_dot=False, dot_ticks=3, is_heal=False, is_mana=False):
    """Build a minimal spell INSERT with only columns that exist in spells_new."""
    vals = {}
    vals["id"] = spell_id
    vals["name"] = f"'{sql_str(name)}'"
    vals["player_1"] = "'BLUE_TRAIL'"
    vals["teleport_zone"] = "''"
    vals["you_cast"] = "''"
    vals["other_casts"] = "''"
    vals["cast_on_you"] = "''"
    vals["cast_on_other"] = "''"
    vals["spell_fades"] = "''"
    vals["range"] = 200 if target != 6 else 0
    vals["aoerange"] = 0
    vals["pushback"] = 0.0
    vals["pushup"] = 0.0
    vals["cast_time"] = 0
    vals["recovery_time"] = 0
    vals["recast_time"] = reuse_ms
    vals["mana"] = 0
    vals["resisttype"] = resist
    vals["targettype"] = target
    vals["skill"] = 98
    vals["LightType"] = 0
    vals["goodEffect"] = 1 if (is_heal or is_mana) else 0
    vals["Activated"] = 0
    vals["zonetype"] = -1
    vals["EnvironmentType"] = 0
    vals["TimeOfDay"] = 0
    vals["basediff"] = 0
    vals["icon"] = 0
    vals["memicon"] = 0
    vals["spellanim"] = 44 if not (is_heal or is_mana) else 13
    vals["uninterruptable"] = 1
    vals["ResistDiff"] = -200
    vals["deleteable"] = 0
    vals["RecourseLink"] = 0
    vals["short_buff_box"] = -1 if not is_dot else 0
    vals["descnum"] = 0
    vals["typedescnum"] = 0
    vals["effectdescnum"] = 0
    vals["effectdescnum2"] = 0
    vals["npc_no_los"] = 0
    vals["reflectable"] = 0
    vals["bonushate"] = 0
    vals["EndurCost"] = 0
    vals["EndurTimerIndex"] = 0
    vals["IsDiscipline"] = 0
    vals["HateAdded"] = 0
    vals["EndurUpkeep"] = 0
    vals["numhitstype"] = 0
    vals["numhits"] = 0
    vals["pvpresistbase"] = -1
    vals["pvpresistcalc"] = 0
    vals["pvpresistcap"] = 0
    vals["spell_category"] = -99
    vals["aemaxtargets"] = 0
    vals["maxtargets"] = 0
    vals["viral_targets"] = 0
    vals["viral_timer"] = 0
    vals["MinResist"] = 0
    vals["MaxResist"] = 0
    vals["min_dist"] = 0.0
    vals["min_dist_mod"] = 0.0
    vals["max_dist"] = 0.0
    vals["max_dist_mod"] = 0.0
    vals["min_range"] = 0
    vals["persistdeath"] = 0
    vals["no_partial_resist"] = 0
    vals["dot_stacking_exempt"] = 0
    vals["disallow_sit"] = 0
    vals["TravelType"] = 0
    vals["SpellAffectIndex"] = -1
    vals["CastingAnim"] = 44
    vals["TargetAnim"] = 0
    vals["new_icon"] = 0
    vals["ldon_trap"] = 0
    vals["pvp_duration"] = 0
    vals["pvp_duration_cap"] = 0
    vals["pcnpc_only_flag"] = 0
    vals["cast_not_standing"] = 0
    vals["can_mgb"] = 0
    vals["nodispell"] = 0
    vals["npc_category"] = 0
    vals["npc_usefulness"] = 0
    vals["nimbuseffect"] = 0
    vals["ConeStartAngle"] = 0
    vals["ConeStopAngle"] = 0
    vals["sneaking"] = 0
    vals["not_extendable"] = 0
    vals["suspendable"] = 0
    vals["viral_range"] = 0
    vals["songcap"] = 0
    vals["no_block"] = 0
    vals["spellgroup"] = 0
    vals["rank"] = 0
    vals["CastRestriction"] = 0
    vals["allowrest"] = 0
    vals["InCombat"] = 0
    vals["OutofCombat"] = 0

    for i in range(1, 17):
        vals[f"classes{i}"] = 255
    vals["deities0"] = 0
    for i in range(1, 17):
        vals[f"deities{i}"] = 0
    for i in range(1, 5):
        vals[f"components{i}"] = -1
        vals[f"component_counts{i}"] = 1
        vals[f"NoexpendReagent{i}"] = -1
    for i in range(1, 13):
        vals[f"effectid{i}"] = 254
        vals[f"effect_base_value{i}"] = 0
        vals[f"effect_limit_value{i}"] = 0
        vals[f"max{i}"] = 0
        vals[f"formula{i}"] = 100

    VALID_FIELDS = [142,143,152,153,160,163,164,169,170,171,172,198,199,
                    203,204,206,209,210,215,216,217,220,221,222,223,225,
                    226,232,233,234,235,236]
    for f in VALID_FIELDS:
        vals[f"field{f}"] = 0

    if is_dot:
        vals["buffdurationformula"] = 7
        vals["buffduration"] = dot_ticks
        vals["AEDuration"] = 2500
        vals["short_buff_box"] = 0
    else:
        vals["buffdurationformula"] = 0
        vals["buffduration"] = 0
        vals["AEDuration"] = 0

    if is_mana:
        vals["effectid1"] = 15
        vals["effect_base_value1"] = abs(damage)
        vals["goodEffect"] = 1
    elif is_heal:
        vals["effectid1"] = 0
        vals["effect_base_value1"] = abs(damage)
        vals["goodEffect"] = 1
    elif is_dot:
        vals["effectid1"] = 0
        vals["effect_base_value1"] = -abs(damage)
        vals["max1"] = abs(damage) * dot_ticks * 2
    else:
        vals["effectid1"] = 0
        vals["effect_base_value1"] = -abs(damage)
        vals["max1"] = abs(damage) * 2

    return vals


def build_aug_item(item_id, name, reqlevel=0, augtype=STAT_AUG_TYPE,
                   proceffect=-1, proctype=0, proclevel2=0, procrate=0,
                   damage=0, icon=1057, price=0,
                   heroic_str=0, heroic_sta=0, heroic_dex=0, heroic_agi=0,
                   heroic_int=0, heroic_wis=0, heroic_cha=0,
                   attack=0, ac=0, hp=0, spelldmg=0, healamt=0, avoidance=0,
                   lore=""):
    """Build an item dict for an augment."""
    v = dict(AUG_ITEM_DEFAULTS)
    v["id"] = item_id
    v["Name"] = f"'{sql_str(name)}'"
    v["reqlevel"] = reqlevel
    v["augtype"] = augtype
    v["proceffect"] = proceffect
    v["proctype"] = proctype
    v["proclevel2"] = proclevel2
    v["procrate"] = procrate
    v["damage"] = damage
    v["icon"] = icon
    v["price"] = price
    v["heroic_str"] = heroic_str
    v["heroic_sta"] = heroic_sta
    v["heroic_dex"] = heroic_dex
    v["heroic_agi"] = heroic_agi
    v["heroic_int"] = heroic_int
    v["heroic_wis"] = heroic_wis
    v["heroic_cha"] = heroic_cha
    v["attack"] = attack
    v["ac"] = ac
    v["hp"] = hp
    v["spelldmg"] = spelldmg
    v["healamt"] = healamt
    v["avoidance"] = avoidance
    v["lore"] = f"'{sql_str(lore)}'" if lore else "''"
    v["loregroup"] = 0
    v["minstatus"] = 0
    v["accuracy"] = 0
    v["aagi"] = 0
    v["acha"] = 0
    v["adex"] = 0
    v["aint"] = 0
    v["asta"] = 0
    v["astr"] = 0
    v["augrestrict"] = 0
    for s in range(1, 7):
        v[f"augslot{s}type"] = 0
        v[f"augslot{s}visible"] = 0
    v["color"] = 0
    v["fvnodrop"] = 0
    v["artifactflag"] = 0
    v["benefitflag"] = 0
    v["tradeskills"] = 0
    v["material"] = 0
    v["elitematerial"] = 0
    v["augdistiller"] = 0
    v["pendingloreflag"] = 0
    v["stacksize"] = 0
    v["manaregen"] = 0
    v["procunk1"] = 0
    v["procunk2"] = 0
    v["procunk3"] = 0
    v["procunk4"] = 0
    v["procunk6"] = 0
    v["procunk7"] = 0
    v["procname"] = "''"
    v["proclevel"] = 0
    v["lorefile"] = "''"
    v["augslot4unk2"] = 0
    v["augslot5unk2"] = 0
    v["augslot6unk2"] = 0
    for k in ["idfile"]:
        if isinstance(v[k], str) and not v[k].startswith("'"):
            v[k] = f"'{v[k]}'"
    return v


def build_container_item(item_id, name, slots, icon, price, lore=""):
    """Build a container/bag item dict."""
    v = {}
    v["id"] = item_id
    v["Name"] = f"'{sql_str(name)}'"
    v["itemclass"] = 1      # container
    v["bagslots"] = slots
    v["bagtype"] = 0         # generic
    v["bagsize"] = 10        # giant (fits anything)
    v["bagwr"] = 0
    v["icon"] = icon
    v["idfile"] = "'IT63'"
    v["price"] = price
    v["weight"] = 0
    v["size"] = 0
    v["magic"] = 1
    v["nodrop"] = 0
    v["norent"] = 1
    v["classes"] = 65535
    v["races"] = 65535
    v["slots"] = 0           # no equip slot
    v["itemtype"] = 0        # normal item
    v["lore"] = f"'{sql_str(lore)}'" if lore else "''"
    v["loregroup"] = 0
    v["minstatus"] = 0
    v["stacksize"] = 0
    v["augtype"] = 0
    v["reqlevel"] = 0
    v["damage"] = 0
    v["heroic_str"] = 0
    v["heroic_sta"] = 0
    v["heroic_dex"] = 0
    v["heroic_agi"] = 0
    v["heroic_int"] = 0
    v["heroic_wis"] = 0
    v["heroic_cha"] = 0
    v["attack"] = 0
    v["ac"] = 0
    v["hp"] = 0
    v["spelldmg"] = 0
    v["healamt"] = 0
    v["avoidance"] = 0
    v["accuracy"] = 0
    v["proceffect"] = -1
    v["proctype"] = 0
    v["proclevel2"] = 0
    v["procrate"] = 0
    v["proclevel"] = 0
    v["procunk1"] = 0
    v["procunk2"] = 0
    v["procunk3"] = 0
    v["procunk4"] = 0
    v["procunk6"] = 0
    v["procunk7"] = 0
    v["procname"] = "''"
    v["lorefile"] = "''"
    v["manaregen"] = 0
    v["aagi"] = 0
    v["acha"] = 0
    v["adex"] = 0
    v["aint"] = 0
    v["asta"] = 0
    v["astr"] = 0
    v["augrestrict"] = 0
    for s in range(1, 7):
        v[f"augslot{s}type"] = 0
        v[f"augslot{s}visible"] = 0
    v["augslot4unk2"] = 0
    v["augslot5unk2"] = 0
    v["augslot6unk2"] = 0
    v["color"] = 0
    v["fvnodrop"] = 0
    v["artifactflag"] = 0
    v["benefitflag"] = 0
    v["tradeskills"] = 0
    v["material"] = 0
    v["elitematerial"] = 0
    v["augdistiller"] = 0
    v["pendingloreflag"] = 0
    return v


def build_catalyst_item(item_id, name, icon, price, lore=""):
    """Build a catalyst consumable item dict."""
    v = {}
    v["id"] = item_id
    v["Name"] = f"'{sql_str(name)}'"
    v["itemclass"] = 0       # normal item
    v["itemtype"] = 0        # generic
    v["icon"] = icon
    v["idfile"] = "'IT63'"
    v["price"] = price
    v["weight"] = 0
    v["size"] = 0
    v["magic"] = 1
    v["nodrop"] = 0
    v["norent"] = 1
    v["classes"] = 65535
    v["races"] = 65535
    v["slots"] = 0
    v["lore"] = f"'{sql_str(lore)}'" if lore else "''"
    v["loregroup"] = 0
    v["minstatus"] = 0
    v["stacksize"] = 20
    v["augtype"] = 0
    v["reqlevel"] = 0
    v["damage"] = 0
    v["heroic_str"] = 0
    v["heroic_sta"] = 0
    v["heroic_dex"] = 0
    v["heroic_agi"] = 0
    v["heroic_int"] = 0
    v["heroic_wis"] = 0
    v["heroic_cha"] = 0
    v["attack"] = 0
    v["ac"] = 0
    v["hp"] = 0
    v["spelldmg"] = 0
    v["healamt"] = 0
    v["avoidance"] = 0
    v["accuracy"] = 0
    v["proceffect"] = -1
    v["proctype"] = 0
    v["proclevel2"] = 0
    v["procrate"] = 0
    v["proclevel"] = 0
    v["procunk1"] = 0
    v["procunk2"] = 0
    v["procunk3"] = 0
    v["procunk4"] = 0
    v["procunk6"] = 0
    v["procunk7"] = 0
    v["procname"] = "''"
    v["lorefile"] = "''"
    v["manaregen"] = 0
    v["aagi"] = 0
    v["acha"] = 0
    v["adex"] = 0
    v["aint"] = 0
    v["asta"] = 0
    v["astr"] = 0
    v["augrestrict"] = 0
    for s in range(1, 7):
        v[f"augslot{s}type"] = 0
        v[f"augslot{s}visible"] = 0
    v["augslot4unk2"] = 0
    v["augslot5unk2"] = 0
    v["augslot6unk2"] = 0
    v["color"] = 0
    v["fvnodrop"] = 0
    v["artifactflag"] = 0
    v["benefitflag"] = 0
    v["tradeskills"] = 0
    v["material"] = 0
    v["elitematerial"] = 0
    v["augdistiller"] = 0
    v["pendingloreflag"] = 0
    v["bagslots"] = 0
    v["bagtype"] = 0
    v["bagsize"] = 0
    v["bagwr"] = 0
    return v


def build_solvent_item(item_id, name, icon, price, lore=""):
    """Build the augment solvent item dict."""
    v = dict(AUG_ITEM_DEFAULTS)
    v["id"] = item_id
    v["Name"] = f"'{sql_str(name)}'"
    v["itemtype"] = 55       # augment solvent
    v["icon"] = icon
    v["idfile"] = "'IT63'"
    v["price"] = price
    v["augtype"] = 0
    v["reqlevel"] = 0
    v["stacksize"] = 20
    v["lore"] = f"'{sql_str(lore)}'" if lore else "''"
    v["loregroup"] = 0
    v["minstatus"] = 0
    v["accuracy"] = 0
    v["aagi"] = 0
    v["acha"] = 0
    v["adex"] = 0
    v["aint"] = 0
    v["asta"] = 0
    v["astr"] = 0
    v["augrestrict"] = 0
    for s in range(1, 7):
        v[f"augslot{s}type"] = 0
        v[f"augslot{s}visible"] = 0
    v["augslot4unk2"] = 0
    v["augslot5unk2"] = 0
    v["augslot6unk2"] = 0
    v["color"] = 0
    v["fvnodrop"] = 0
    v["artifactflag"] = 0
    v["benefitflag"] = 0
    v["tradeskills"] = 0
    v["material"] = 0
    v["elitematerial"] = 0
    v["augdistiller"] = 0
    v["pendingloreflag"] = 0
    v["manaregen"] = 0
    v["proceffect"] = -1
    v["proctype"] = 0
    v["proclevel2"] = 0
    v["procrate"] = 0
    v["proclevel"] = 0
    v["procunk1"] = 0
    v["procunk2"] = 0
    v["procunk3"] = 0
    v["procunk4"] = 0
    v["procunk6"] = 0
    v["procunk7"] = 0
    v["procname"] = "''"
    v["lorefile"] = "''"
    v["damage"] = 0
    v["heroic_str"] = 0
    v["heroic_sta"] = 0
    v["heroic_dex"] = 0
    v["heroic_agi"] = 0
    v["heroic_int"] = 0
    v["heroic_wis"] = 0
    v["heroic_cha"] = 0
    v["attack"] = 0
    v["ac"] = 0
    v["hp"] = 0
    v["spelldmg"] = 0
    v["healamt"] = 0
    v["avoidance"] = 0
    return v


# ============================================================
# ID HELPERS
# ============================================================

def level_aug_id(family_index, level):
    """Item ID for a leveling weapon aug (family 0-26, level 1-5)."""
    return LEVEL_AUG_ID_BASE + family_index * LEVELS_PER_FAMILY + (level - 1)


def stat_aug_id(family_index, level):
    """Item ID for an endgame stat aug (family 0-6, level 1-5)."""
    return STAT_AUG_ID_BASE + family_index * LEVELS_PER_FAMILY + (level - 1)


def proc_aug_id(family_index, level):
    """Item ID for an endgame proc aug (family 0-4, level 1-5)."""
    return PROC_AUG_ID_BASE + family_index * LEVELS_PER_FAMILY + (level - 1)


def endgame_spell_id(proc_family_index, level):
    """Spell ID for an endgame proc aug at a given level."""
    return ENDGAME_SPELL_ID_BASE + proc_family_index * LEVELS_PER_FAMILY + (level - 1)


def merchant_entry(merchant_id, slot, item_id, level_req=0, alt_cost=0,
                   classes=65535, probability=100):
    """Build a merchantlist INSERT."""
    return (
        f"INSERT INTO `merchantlist` (merchantid, slot, item, faction_required, "
        f"level_required, alt_currency_cost, classes_required, probability) "
        f"VALUES ({merchant_id}, {slot}, {item_id}, {FACTION_REQUIRED}, "
        f"{level_req}, {alt_cost}, {classes}, {probability});"
    )


# ============================================================
# GENERATE SQL
# ============================================================

def generate():
    """Generate all SQL and C++ header, write to output files."""
    lines = []
    lines.append("-- ============================================================")
    lines.append("-- Steps 8 & 9: Augments + Vendors + Merge System")
    lines.append(f"-- Generated: {datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    lines.append("-- Re-run tools/generate_augments.py to regenerate")
    lines.append("-- ============================================================")
    lines.append("")

    # ------------------------------------------------------------------
    # CLEANUP (idempotent — deletes previous run's data)
    # ------------------------------------------------------------------
    lines.append("-- Clean up previous run")
    lines.append(f"DELETE FROM `items` WHERE id BETWEEN {LEVEL_AUG_ID_BASE} AND {LEVEL_AUG_ID_BASE + 999};")
    lines.append(f"DELETE FROM `items` WHERE id BETWEEN {STAT_AUG_ID_BASE} AND {STAT_AUG_ID_BASE + 999};")
    lines.append(f"DELETE FROM `items` WHERE id BETWEEN {PROC_AUG_ID_BASE} AND {PROC_AUG_ID_BASE + 999};")
    lines.append(f"DELETE FROM `items` WHERE id BETWEEN {CATALYST_ID_BASE} AND {CATALYST_ID_BASE + 99};")
    lines.append(f"DELETE FROM `items` WHERE id = {FORGEMASTER_CONTAINER_ID};")
    lines.append(f"DELETE FROM `items` WHERE id = {SOLVENT_ID};")
    lines.append(f"DELETE FROM `spells_new` WHERE id BETWEEN {LEVEL_SPELL_ID_BASE} AND {LEVEL_SPELL_ID_BASE + 999};")
    lines.append(f"DELETE FROM `spells_new` WHERE id BETWEEN {ENDGAME_SPELL_ID_BASE} AND {ENDGAME_SPELL_ID_BASE + 999};")
    # Also clean up old Step 8 spell range if it overlapped
    lines.append(f"DELETE FROM `spells_new` WHERE id BETWEEN 65100 AND 65199;")
    for npc in VENDOR_NPCS:
        lines.append(f"DELETE FROM `merchantlist` WHERE merchantid = {npc['merchant_id']};")
        lines.append(f"DELETE FROM `spawnentry` WHERE npcID = {npc['id']};")
        lines.append(f"DELETE FROM `npc_types` WHERE id = {npc['id']};")
    lines.append(f"DELETE FROM `spawngroup` WHERE id BETWEEN {SPAWNGROUP_START} AND {SPAWNGROUP_START + 99};")
    lines.append(f"DELETE FROM `spawn2` WHERE id BETWEEN {SPAWN2_START} AND {SPAWN2_START + 99};")
    # Clean up old Step 8 items range
    lines.append(f"DELETE FROM `items` WHERE id BETWEEN 200100 AND 200199;")
    lines.append("")

    # Track merchant slots
    merchant_slots = {}
    for npc in VENDOR_NPCS:
        merchant_slots[npc["merchant_id"]] = 0

    # ------------------------------------------------------------------
    # 1. PROC SPELLS — Leveling (27 spells, shared across merge levels)
    # ------------------------------------------------------------------
    lines.append("-- ============================================================")
    lines.append("-- PROC SPELLS: Leveling Weapon Augments (27 spells)")
    lines.append("-- ============================================================")
    lines.append("")

    combat_spell_ids  = {}  # tier_idx -> spell_id
    lifetap_spell_ids = {}
    mana_spell_ids    = {}
    next_level_spell  = LEVEL_SPELL_ID_BASE

    for tier_idx, (req_lv, prefix, dd_dmg, lt_heal, mn_restore, _) in enumerate(LEVELING_TIERS):
        # Combat DD proc
        sid = next_level_spell
        combat_spell_ids[tier_idx] = sid
        sv = build_simple_spell(sid, f"{prefix} Combat Proc", dd_dmg,
                                resist=2, target=5, reuse_ms=8000)
        lines.append(spell_insert_sql(sv))
        next_level_spell += 1

        # Lifetap heal proc
        sid = next_level_spell
        lifetap_spell_ids[tier_idx] = sid
        sv = build_simple_spell(sid, f"{prefix} Lifetap Proc", lt_heal,
                                resist=0, target=6, reuse_ms=10000, is_heal=True)
        lines.append(spell_insert_sql(sv))
        next_level_spell += 1

        # Mana restore proc
        sid = next_level_spell
        mana_spell_ids[tier_idx] = sid
        sv = build_simple_spell(sid, f"{prefix} Mana Proc", mn_restore,
                                resist=0, target=6, reuse_ms=6000, is_mana=True)
        lines.append(spell_insert_sql(sv))
        next_level_spell += 1

    lines.append("")

    # ------------------------------------------------------------------
    # 2. PROC SPELLS — Endgame L1-L5 (25 spells)
    # ------------------------------------------------------------------
    lines.append("-- ============================================================")
    lines.append("-- PROC SPELLS: Endgame Proc Augments L1-L5 (25 spells)")
    lines.append("-- ============================================================")
    lines.append("")

    # endgame_spell_map[proc_family][level] = spell_id
    endgame_spell_map = {}

    for fi, (key, name, proc_type, base_val, _, _) in enumerate(ENDGAME_PROC_AUGS):
        endgame_spell_map[fi] = {}
        is_dot  = proc_type == "poison_dot"
        is_heal = proc_type == "heal"
        resist  = PROC_RESIST[proc_type]
        target  = PROC_TARGET[proc_type]
        base_reuse = PROC_REUSE[proc_type]

        for lv in range(1, MAX_AUG_LEVEL + 1):
            sid = endgame_spell_id(fi, lv)
            endgame_spell_map[fi][lv] = sid

            scaled_dmg = int(round(base_val * PROC_DAMAGE_MULT[lv - 1]))
            scaled_reuse = max(1000, base_reuse - PROC_REUSE_REDUCTION[lv - 1])

            suffix = LEVEL_SUFFIX[lv]
            sv = build_simple_spell(
                sid, f"{name} Proc{suffix}", scaled_dmg,
                resist=resist, target=target, reuse_ms=scaled_reuse,
                is_dot=is_dot, is_heal=is_heal
            )
            lines.append(spell_insert_sql(sv))

    lines.append("")

    # ------------------------------------------------------------------
    # 3. ITEMS — Leveling Weapon Augments L1-L5 (135 items)
    # ------------------------------------------------------------------
    lines.append("-- ============================================================")
    lines.append("-- ITEMS: Leveling Weapon Augments L1-L5 (135 items)")
    lines.append("-- ============================================================")
    lines.append("")

    weaponsmith_id = VENDOR_NPCS[0]["merchant_id"]

    # Family layout: families 0-8 = Combat, 9-17 = Lifetap, 18-26 = Mana
    aug_types_leveling = [
        ("Combat Stone",  "combat",  combat_spell_ids,  lambda ti: LEVELING_TIERS[ti][2]),
        ("Lifetap Stone", "lifetap", lifetap_spell_ids, lambda ti: LEVELING_TIERS[ti][3]),
        ("Mana Stone",    "mana",    mana_spell_ids,    lambda ti: LEVELING_TIERS[ti][4]),
    ]

    for type_idx, (type_name, icon_key, spell_map, _) in enumerate(aug_types_leveling):
        for tier_idx, (req_lv, prefix, dd_dmg, lt_heal, mn_restore, pp_cost) in enumerate(LEVELING_TIERS):
            family_idx = type_idx * NUM_LEVEL_TIERS + tier_idx
            base_dmg = LEVELING_BONUS_DMG[tier_idx]
            copper_cost = pp_cost * 1000

            for lv in range(1, MAX_AUG_LEVEL + 1):
                iid = level_aug_id(family_idx, lv)
                suffix = LEVEL_SUFFIX[lv]
                aug_name = f"{prefix} {type_name}{suffix}"

                # Leveling merge: same spell, better proc rate + tiny DMG
                merged_dmg = base_dmg + LEVELING_DMG_BONUS[lv - 1]
                merged_procrate = LEVELING_PROCRATE_BONUS[lv - 1]

                iv = build_aug_item(
                    iid, aug_name,
                    reqlevel=req_lv, augtype=WEAPON_AUG_TYPE,
                    proceffect=spell_map[tier_idx], proctype=0,
                    proclevel2=req_lv, procrate=merged_procrate,
                    damage=merged_dmg, icon=ICON_MAP[icon_key],
                    price=copper_cost,
                    lore=f"A {prefix.lower()} weapon stone (Merge Level {lv})"
                )
                lines.append(item_insert_sql(iv))

            # Only L1 on the vendor
            l1_id = level_aug_id(family_idx, 1)
            slot = merchant_slots[weaponsmith_id]
            lines.append(merchant_entry(weaponsmith_id, slot, l1_id, level_req=req_lv))
            merchant_slots[weaponsmith_id] += 1

    lines.append("")

    # ------------------------------------------------------------------
    # 4. ITEMS — Endgame Stat Augments L1-L5 (35 items)
    # ------------------------------------------------------------------
    lines.append("-- ============================================================")
    lines.append("-- ITEMS: Endgame Stat Augments L1-L5 (35 items)")
    lines.append("-- ============================================================")
    lines.append("")

    provisioner_id = VENDOR_NPCS[1]["merchant_id"]
    artificer_id   = VENDOR_NPCS[2]["merchant_id"]

    for fi, (key, name, hero_field, hero_base, sec_field, sec_base, ce_cost, pp_cost) in enumerate(ENDGAME_STAT_AUGS):
        for lv in range(1, MAX_AUG_LEVEL + 1):
            iid = stat_aug_id(fi, lv)
            suffix = LEVEL_SUFFIX[lv]

            # Scale heroic stat from the fixed table
            heroic_val = STAT_HEROIC_VALUES[lv - 1]
            # Scale secondary from multiplier
            secondary_val = int(round(sec_base * STAT_SECONDARY_MULT[lv - 1]))

            kwargs = {hero_field: heroic_val, sec_field: secondary_val}

            iv = build_aug_item(
                iid, f"{name}{suffix}",
                reqlevel=0, augtype=STAT_AUG_TYPE,
                icon=ICON_MAP[key],
                price=pp_cost * 1000,
                lore=f"{name} (Merge Level {lv})",
                **kwargs,
            )
            lines.append(item_insert_sql(iv))

        # Provisioner sells L1 for CE
        l1_id = stat_aug_id(fi, 1)
        slot = merchant_slots[provisioner_id]
        lines.append(merchant_entry(provisioner_id, slot, l1_id, alt_cost=ce_cost))
        merchant_slots[provisioner_id] += 1

        # Artificer sells L2 for RE (skip-merge shortcut) at 2x L1 RE-equivalent
        l2_id = stat_aug_id(fi, 2)
        slot = merchant_slots[artificer_id]
        lines.append(merchant_entry(artificer_id, slot, l2_id, alt_cost=1000))
        merchant_slots[artificer_id] += 1

    lines.append("")

    # ------------------------------------------------------------------
    # 5. ITEMS — Endgame Proc Augments L1-L5 (25 items)
    # ------------------------------------------------------------------
    lines.append("-- ============================================================")
    lines.append("-- ITEMS: Endgame Proc Augments L1-L5 (25 items)")
    lines.append("-- ============================================================")
    lines.append("")

    for fi, (key, name, proc_type, base_val, re_cost, pp_cost) in enumerate(ENDGAME_PROC_AUGS):
        base_reuse = PROC_REUSE[proc_type]

        for lv in range(1, MAX_AUG_LEVEL + 1):
            iid = proc_aug_id(fi, lv)
            suffix = LEVEL_SUFFIX[lv]
            sid = endgame_spell_map[fi][lv]

            iv = build_aug_item(
                iid, f"{name}{suffix}",
                reqlevel=0, augtype=WEAPON_AUG_TYPE,
                proceffect=sid, proctype=0,
                proclevel2=0, procrate=0,
                icon=ICON_MAP[key],
                price=pp_cost * 1000,
                lore=f"{name} (Merge Level {lv})",
            )
            lines.append(item_insert_sql(iv))

        # Artificer sells L1 endgame proc augs for RE
        l1_id = proc_aug_id(fi, 1)
        slot = merchant_slots[artificer_id]
        lines.append(merchant_entry(artificer_id, slot, l1_id, alt_cost=re_cost))
        merchant_slots[artificer_id] += 1

    lines.append("")

    # ------------------------------------------------------------------
    # 6. ITEMS — Merge Catalysts (4 items)
    # ------------------------------------------------------------------
    lines.append("-- ============================================================")
    lines.append("-- ITEMS: Merge Catalysts (4 tiers)")
    lines.append("-- ============================================================")
    lines.append("")

    forgemaster_id = VENDOR_NPCS[3]["merchant_id"]
    catalyst_tiers = ["L1->L2", "L2->L3", "L3->L4", "L4->L5"]

    for ci, (cat_name, ce_cost, pp_cost, re_cost) in enumerate(CATALYSTS):
        cat_id = CATALYST_ID_BASE + ci
        iv = build_catalyst_item(
            cat_id, cat_name,
            icon=ICON_MAP.get("catalyst", 1064),
            price=pp_cost,
            lore=f"Merge catalyst ({catalyst_tiers[ci]})"
        )
        lines.append(item_insert_sql(iv))

        # Add to vendors based on currency type
        if ce_cost > 0:
            # Forgemaster (CE)
            slot = merchant_slots[forgemaster_id]
            lines.append(merchant_entry(forgemaster_id, slot, cat_id, alt_cost=ce_cost))
            merchant_slots[forgemaster_id] += 1

        if pp_cost > 0:
            # Weaponsmith (PP) — convenience path
            slot = merchant_slots[weaponsmith_id]
            lines.append(merchant_entry(weaponsmith_id, slot, cat_id))
            merchant_slots[weaponsmith_id] += 1

        if re_cost > 0:
            # Artificer (RE)
            slot = merchant_slots[artificer_id]
            lines.append(merchant_entry(artificer_id, slot, cat_id, alt_cost=re_cost))
            merchant_slots[artificer_id] += 1

    lines.append("")

    # ------------------------------------------------------------------
    # 7. ITEM — Forgemaster Anvil Container (4-slot combine bag)
    # ------------------------------------------------------------------
    lines.append("-- ============================================================")
    lines.append("-- ITEM: Forgemaster Anvil (4-slot combine container)")
    lines.append("-- ============================================================")
    lines.append("")

    cv = build_container_item(
        FORGEMASTER_CONTAINER_ID, "Augment Forge", slots=4,
        icon=ICON_MAP["container"],
        price=10000,  # 10pp
        lore="Place 3 matching augments and a catalyst, then combine"
    )
    lines.append(item_insert_sql(cv))

    # Sell on Forgemaster for 1 CE (essentially free)
    slot = merchant_slots[forgemaster_id]
    lines.append(merchant_entry(forgemaster_id, slot, FORGEMASTER_CONTAINER_ID, alt_cost=1))
    merchant_slots[forgemaster_id] += 1

    # Also sell on Weaponsmith for 10pp
    slot = merchant_slots[weaponsmith_id]
    lines.append(merchant_entry(weaponsmith_id, slot, FORGEMASTER_CONTAINER_ID))
    merchant_slots[weaponsmith_id] += 1

    lines.append("")

    # ------------------------------------------------------------------
    # 8. ITEM — Purified Solvent
    # ------------------------------------------------------------------
    lines.append("-- ============================================================")
    lines.append("-- ITEM: Purified Solvent (safe aug removal)")
    lines.append("-- ============================================================")
    lines.append("")

    sv = build_solvent_item(
        SOLVENT_ID, "Purified Solvent",
        icon=ICON_MAP["solvent"],
        price=100 * 1000,  # 100pp
        lore="Safely removes an augment from equipment"
    )
    lines.append(item_insert_sql(sv))

    # Add to Provisioner (100 CE) and Weaponsmith (100pp)
    slot = merchant_slots[provisioner_id]
    lines.append(merchant_entry(provisioner_id, slot, SOLVENT_ID, alt_cost=100))
    merchant_slots[provisioner_id] += 1

    slot = merchant_slots[weaponsmith_id]
    lines.append(merchant_entry(weaponsmith_id, slot, SOLVENT_ID))
    merchant_slots[weaponsmith_id] += 1

    lines.append("")

    # ------------------------------------------------------------------
    # 9. VENDOR NPCs (4 NPCs in Bazaar)
    # ------------------------------------------------------------------
    lines.append("-- ============================================================")
    lines.append("-- VENDOR NPCs in Bazaar")
    lines.append("-- ============================================================")
    lines.append("")

    for idx, npc in enumerate(VENDOR_NPCS):
        npc_id = npc["id"]
        alt_currency_id = npc["alt_currency"]

        lines.append(
            f"INSERT INTO `npc_types` (id, name, lastname, level, race, class, bodytype, "
            f"hp, mana, gender, texture, helmtexture, size, "
            f"merchant_id, alt_currency_id, "
            f"loottable_id, npc_faction_id, "
            f"mindmg, maxdmg, "
            f"npc_spells_id, npc_spells_effects_id, "
            f"d_melee_texture1, d_melee_texture2, "
            f"runspeed, walkspeed, "
            f"aggroradius, assistradius, findable, trackable) "
            f"VALUES ("
            f"{npc_id}, '{sql_str(npc['name'])}', '{sql_str(npc['last_name'])}', "
            f"{npc['level']}, {npc['race']}, 41, 1, "
            f"100000, 0, {npc['gender']}, {npc['texture']}, {npc['texture']}, 6.0, "
            f"{npc['merchant_id']}, {alt_currency_id}, "
            f"0, 0, "
            f"10, 20, "
            f"0, 0, "
            f"0, 0, "
            f"1.25, 0.6, "
            f"0, 0, 1, 1);"
        )

        sg_id = SPAWNGROUP_START + idx
        lines.append(
            f"INSERT INTO `spawngroup` (id, name, spawn_limit, dist) "
            f"VALUES ({sg_id}, 'bazaar_aug_vendor_{idx}', 0, 0.0);"
        )
        lines.append(
            f"INSERT INTO `spawnentry` (spawngroupID, npcID, chance) "
            f"VALUES ({sg_id}, {npc_id}, 100);"
        )
        s2_id = SPAWN2_START + idx
        lines.append(
            f"INSERT INTO `spawn2` (id, spawngroupID, zone, version, x, y, z, heading, respawntime) "
            f"VALUES ({s2_id}, {sg_id}, 'bazaar', 0, {npc['x']}, {npc['y']}, {npc['z']}, {npc['heading']}, 640);"
        )
        lines.append("")

    # ------------------------------------------------------------------
    # SUMMARY
    # ------------------------------------------------------------------
    total_spells = (next_level_spell - LEVEL_SPELL_ID_BASE) + \
                   (NUM_PROC_FAMILIES * MAX_AUG_LEVEL)
    total_items = (NUM_LEVEL_FAMILIES * MAX_AUG_LEVEL) + \
                  (NUM_STAT_FAMILIES * MAX_AUG_LEVEL) + \
                  (NUM_PROC_FAMILIES * MAX_AUG_LEVEL) + \
                  len(CATALYSTS) + 1 + 1  # catalysts + container + solvent

    lines.append("-- ============================================================")
    lines.append(f"-- Summary: {total_spells} spells, {total_items} items, {len(VENDOR_NPCS)} NPCs")
    lines.append(f"-- Leveling aug IDs: {LEVEL_AUG_ID_BASE}-{LEVEL_AUG_ID_BASE + NUM_LEVEL_FAMILIES * LEVELS_PER_FAMILY - 1}")
    lines.append(f"-- Stat aug IDs:     {STAT_AUG_ID_BASE}-{STAT_AUG_ID_BASE + NUM_STAT_FAMILIES * LEVELS_PER_FAMILY - 1}")
    lines.append(f"-- Proc aug IDs:     {PROC_AUG_ID_BASE}-{PROC_AUG_ID_BASE + NUM_PROC_FAMILIES * LEVELS_PER_FAMILY - 1}")
    lines.append(f"-- Catalyst IDs:     {CATALYST_ID_BASE}-{CATALYST_ID_BASE + len(CATALYSTS) - 1}")
    lines.append(f"-- Container ID:     {FORGEMASTER_CONTAINER_ID}")
    lines.append(f"-- Solvent ID:       {SOLVENT_ID}")
    lines.append(f"-- NPC IDs:          {NPC_ID_BASE}-{NPC_ID_BASE + len(VENDOR_NPCS) - 1}")
    lines.append("-- ============================================================")

    # Write SQL
    os.makedirs(os.path.dirname(SQL_OUTPUT), exist_ok=True)
    with open(SQL_OUTPUT, "w", encoding="utf-8") as f:
        f.write("\n".join(lines) + "\n")

    print(f"Generated {SQL_OUTPUT}")
    print(f"  {total_spells} proc spells")
    print(f"  {total_items} items ({NUM_LEVEL_FAMILIES * MAX_AUG_LEVEL} leveling + "
          f"{NUM_STAT_FAMILIES * MAX_AUG_LEVEL} stat + "
          f"{NUM_PROC_FAMILIES * MAX_AUG_LEVEL} proc + "
          f"{len(CATALYSTS)} catalysts + container + solvent)")
    print(f"  {len(VENDOR_NPCS)} vendor NPCs")

    # ------------------------------------------------------------------
    # GENERATE C++ HEADER
    # ------------------------------------------------------------------
    generate_cpp_header()


def generate_cpp_header():
    """Generate zone/augment_merge_data.h with ID constants for the merge system."""
    h = []
    h.append("// Auto-generated by tools/generate_augments.py — DO NOT EDIT")
    h.append(f"// Generated: {datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    h.append("// Defines ID ranges and lookup helpers for the augment merge system.")
    h.append("#pragma once")
    h.append("")
    h.append("#include <cstdint>")
    h.append("")
    h.append("namespace AugMergeData {")
    h.append("")
    h.append(f"// Leveling weapon augs: {NUM_LEVEL_FAMILIES} families × {LEVELS_PER_FAMILY} levels")
    h.append(f"static constexpr uint32_t LEVEL_AUG_BASE     = {LEVEL_AUG_ID_BASE};")
    h.append(f"static constexpr int      LEVEL_AUG_FAMILIES = {NUM_LEVEL_FAMILIES};")
    h.append("")
    h.append(f"// Endgame stat augs: {NUM_STAT_FAMILIES} families × {LEVELS_PER_FAMILY} levels")
    h.append(f"static constexpr uint32_t STAT_AUG_BASE      = {STAT_AUG_ID_BASE};")
    h.append(f"static constexpr int      STAT_AUG_FAMILIES  = {NUM_STAT_FAMILIES};")
    h.append("")
    h.append(f"// Endgame proc augs: {NUM_PROC_FAMILIES} families × {LEVELS_PER_FAMILY} levels")
    h.append(f"static constexpr uint32_t PROC_AUG_BASE      = {PROC_AUG_ID_BASE};")
    h.append(f"static constexpr int      PROC_AUG_FAMILIES  = {NUM_PROC_FAMILIES};")
    h.append("")
    h.append(f"static constexpr int LEVELS_PER_FAMILY = {LEVELS_PER_FAMILY};")
    h.append(f"static constexpr int MAX_AUG_LEVEL     = {MAX_AUG_LEVEL};")
    h.append(f"static constexpr int TOTAL_FAMILIES     = {TOTAL_FAMILIES};")
    h.append("")

    # Catalyst IDs
    h.append("// Merge Catalysts — index 0 = L1→L2, 1 = L2→L3, 2 = L3→L4, 3 = L4→L5")
    cat_ids = ", ".join(str(CATALYST_ID_BASE + i) for i in range(len(CATALYSTS)))
    h.append(f"static constexpr uint32_t CATALYST_IDS[]  = {{ {cat_ids} }};")
    h.append(f"static constexpr int      NUM_CATALYSTS   = {len(CATALYSTS)};")
    h.append("")

    # Forgemaster container
    h.append(f"static constexpr uint32_t FORGEMASTER_CONTAINER_ID = {FORGEMASTER_CONTAINER_ID};")
    h.append("")

    # Lookup functions
    h.append("// Decode an item ID into (family, level). Returns false if not a mergeable aug.")
    h.append("inline bool GetAugInfo(uint32_t item_id, int& out_family, int& out_level) {")
    h.append(f"    if (item_id >= LEVEL_AUG_BASE && item_id < LEVEL_AUG_BASE + LEVEL_AUG_FAMILIES * LEVELS_PER_FAMILY) {{")
    h.append( "        uint32_t offset = item_id - LEVEL_AUG_BASE;")
    h.append( "        out_family = static_cast<int>(offset / LEVELS_PER_FAMILY);")
    h.append( "        out_level  = static_cast<int>(offset % LEVELS_PER_FAMILY) + 1;")
    h.append( "        return true;")
    h.append( "    }")
    h.append(f"    if (item_id >= STAT_AUG_BASE && item_id < STAT_AUG_BASE + STAT_AUG_FAMILIES * LEVELS_PER_FAMILY) {{")
    h.append( "        uint32_t offset = item_id - STAT_AUG_BASE;")
    h.append( "        out_family = static_cast<int>(offset / LEVELS_PER_FAMILY) + LEVEL_AUG_FAMILIES;")
    h.append( "        out_level  = static_cast<int>(offset % LEVELS_PER_FAMILY) + 1;")
    h.append( "        return true;")
    h.append( "    }")
    h.append(f"    if (item_id >= PROC_AUG_BASE && item_id < PROC_AUG_BASE + PROC_AUG_FAMILIES * LEVELS_PER_FAMILY) {{")
    h.append( "        uint32_t offset = item_id - PROC_AUG_BASE;")
    h.append( "        out_family = static_cast<int>(offset / LEVELS_PER_FAMILY) + LEVEL_AUG_FAMILIES + STAT_AUG_FAMILIES;")
    h.append( "        out_level  = static_cast<int>(offset % LEVELS_PER_FAMILY) + 1;")
    h.append( "        return true;")
    h.append( "    }")
    h.append( "    return false;")
    h.append("}")
    h.append("")

    h.append("// Encode a family + level back to an item ID. Returns 0 if invalid.")
    h.append("inline uint32_t GetItemID(int family, int level) {")
    h.append("    if (level < 1 || level > MAX_AUG_LEVEL) return 0;")
    h.append("    if (family < LEVEL_AUG_FAMILIES) {")
    h.append("        return LEVEL_AUG_BASE + family * LEVELS_PER_FAMILY + (level - 1);")
    h.append("    }")
    h.append("    int f = family - LEVEL_AUG_FAMILIES;")
    h.append("    if (f < STAT_AUG_FAMILIES) {")
    h.append("        return STAT_AUG_BASE + f * LEVELS_PER_FAMILY + (level - 1);")
    h.append("    }")
    h.append("    f -= STAT_AUG_FAMILIES;")
    h.append("    if (f < PROC_AUG_FAMILIES) {")
    h.append("        return PROC_AUG_BASE + f * LEVELS_PER_FAMILY + (level - 1);")
    h.append("    }")
    h.append("    return 0;")
    h.append("}")
    h.append("")

    h.append("// Get the required catalyst item ID for merging from current_level to current_level+1.")
    h.append("// Returns 0 if already at max level.")
    h.append("inline uint32_t GetRequiredCatalyst(int current_level) {")
    h.append("    if (current_level >= 1 && current_level < MAX_AUG_LEVEL) {")
    h.append("        return CATALYST_IDS[current_level - 1];")
    h.append("    }")
    h.append("    return 0;")
    h.append("}")
    h.append("")

    h.append("// Check if an item is a catalyst. Returns the target merge level (2-5) or 0.")
    h.append("inline int GetCatalystTier(uint32_t item_id) {")
    h.append("    for (int i = 0; i < NUM_CATALYSTS; i++) {")
    h.append("        if (item_id == CATALYST_IDS[i]) return i + 2;")
    h.append("    }")
    h.append("    return 0;")
    h.append("}")
    h.append("")
    h.append("} // namespace AugMergeData")
    h.append("")

    os.makedirs(os.path.dirname(CPP_HEADER_OUTPUT), exist_ok=True)
    with open(CPP_HEADER_OUTPUT, "w", encoding="utf-8") as f:
        f.write("\n".join(h) + "\n")

    print(f"Generated {CPP_HEADER_OUTPUT}")


if __name__ == "__main__":
    generate()
