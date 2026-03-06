#!/usr/bin/env python3
"""
Step 10 — Zone Drop Augs, Named Mob Augs, Daily/Weekly Essence Quests
======================================================================
Generates SQL for:
  1. Zone flavor augments (ultra-rare drops from any mob in a zone via global_loot)
  2. Named mob multi-stat augments (global_loot with rare=1 flag)
  3. Daily kill quest (500 CE + 20 RE) and weekly kill quest (2000 CE + 150 RE)

Scope: Classic (expansion 0) + Kunark (1) + Velious (2) only.
Zones that already have augment drops in their loot tables are skipped.

Usage:
    python tools/generate_zone_augs.py

Output:
    utils/sql/git/optional/step10_zone_augs_and_quests.sql

Design reference: game_design/infinite_progression/AUGMENT_SYSTEM.md §3.3, §5.2, §7.3
"""

import os
import datetime

# ============================================================
# ID Ranges (well above existing data)
# ============================================================
ITEM_ID_BASE        = 201000   # Zone flavor augs start here
NAMED_AUG_ID_BASE   = 201500   # Named mob multi-stat augs
SPELL_ID_BASE       = 65300    # Proc spells for flavor augs that have procs
LOOTTABLE_ID_BASE   = 210000   # Our loottable range
LOOTDROP_ID_BASE    = 210000   # Our lootdrop range
GLOBAL_LOOT_ID_BASE = 100      # global_loot entries (existing max is 20)
TASK_ID_BASE        = 600000   # Daily/weekly tasks

# Currency IDs (matching Steps 4/8/9)
COMMON_ESSENCE_CURRENCY = 100
RARE_ESSENCE_CURRENCY   = 101
COMMON_ESSENCE_ITEM     = 200010
RARE_ESSENCE_ITEM       = 200011

# ============================================================
# Zones that already have aug drops — SKIP these
# ============================================================
ZONES_WITH_EXISTING_AUGS = {
    # Classic
    "ecommons", "everfrost", "feerrott", "gukbottom", "guktop",
    "lavastorm", "mistmoore", "paw", "permafrost",
    "soldunga", "soldungb",
    # Kunark
    "burningwood", "charasis", "chardok", "dalnir", "droga",
    "firiona", "frontiermtns", "kaesora", "lakeofillomen",
    "overthere", "sebilis", "skyfire", "swampofnohope",
    "trakanon", "veeshan", "warslikswood",
    # Velious
    "crystal", "eastwastes", "frozenshadow", "greatdivide",
    "iceclad", "necropolis", "skyshrine", "stonebrunt",
    "velketor", "wakening", "westwastes",
}

# Non-combat zones — also skip
NON_COMBAT_ZONES = {
    "arena", "cshome", "bazaar", "nexus", "shadowrest",
    "tutoriala", "tutorialb", "nedaria",
    # Cities
    "akanon", "erudnext", "erudnint", "felwithea", "felwitheb",
    "freporte", "freportn", "freportw", "grobb", "halas",
    "kaladima", "kaladimb", "neriaka", "neriakb", "neriakc",
    "oggok", "paineel", "qeynos", "qeynos2", "qrg",
    "rivervale", "cabeast", "cabwest", "thurgadina",
    "shadowhaven", "sharvahl", "katta",
}

# ============================================================
# Zone Flavor Augment Definitions
# ============================================================
# Each tuple: (short_name, aug_name, primary_stat, primary_val,
#              secondary_stat, secondary_val, flavor_text, drop_chance_pct)
#
# Stats use item DB column names:
#   heroic_str, heroic_sta, heroic_dex, heroic_agi, heroic_int, heroic_wis, heroic_cha
#   hp, mana, ac, attack, avoidance, manaregen, regen, spelldmg, healamt
#
# drop_chance_pct is the chance in lootdrop_entries (0.1 - 0.5 typical)

ZONE_FLAVOR_AUGS = [
    # ── Classic (expansion 0) ──────────────────────────────────────────
    ("blackburrow",  "Gnoll Fang Chip",          "heroic_str", 2, "hp", 5,
     "A crude tooth fragment from a gnoll champion.", 0.3),
    ("befallen",     "Darkened Bone Shard",       "heroic_int", 2, "manaregen", 3,
     "Necromantic residue clings to this sliver of bone.", 0.3),
    ("unrest",       "Flickering Spirit Gem",     "heroic_wis", 2, "hp", 8,
     "A haunted essence trapped in crystal.", 0.3),
    ("crushbone",    "Orcish War Chip",           "heroic_str", 2, "attack", 5,
     "A fragment of Emperor Crush's war standard.", 0.3),
    ("cazicthule",   "Swamp Terror Shard",        "heroic_sta", 2, "hp", 10,
     "Pulsing with the dread of Cazic-Thule.", 0.2),
    ("highkeep",     "Captain's Honor Chip",      "heroic_cha", 2, "hp", 8,
     "A sliver from the Captain of the Guard's medal.", 0.3),
    ("najena",       "Dark Elf Sigil Chip",       "heroic_int", 2, "spelldmg", 5,
     "Inscribed with arcane symbols of Najena.", 0.3),
    ("runnyeye",     "Evil Eye Lens Fragment",    "heroic_int", 2, "hp", 5,
     "A shard from an Evil Eye's focusing lens.", 0.3),
    ("qcat",         "Sewer Rat Fang",            "heroic_dex", 2, "hp", 5,
     "Surprisingly sharp, it hums with dark energy.", 0.4),
    ("hole",         "Abyssal Crystal Shard",     "heroic_sta", 3, "ac", 5,
     "Gleams with light from the depths of Norrath.", 0.2),
    ("kedge",        "Kedge Scale Fragment",      "heroic_agi", 3, "avoidance", 5,
     "A shimmering scale from a servant of Phinigel.", 0.2),
    ("fearplane",    "Fragment of Dread",         "heroic_str", 3, "heroic_sta", 3,
     "Crystallized terror from the Plane of Fear.", 0.15),
    ("hateplane",    "Spite Shard",               "heroic_str", 4, "attack", 10,
     "Innoruuk's malice given physical form.", 0.15),
    ("beholder",     "Xorbb's Gaze Chip",        "heroic_int", 2, "spelldmg", 3,
     "Radiates the psychic energy of the gorge.", 0.3),
    ("innothule",    "Troll Moss Sliver",         "heroic_sta", 2, "regen", 3,
     "Infused with the regenerative swamp muck.", 0.3),
    ("kithicor",     "Shadow Thorn",              "heroic_dex", 2, "attack", 5,
     "A thorn that drinks in the darkness of Kithicor.", 0.3),
    ("commons",      "Bandit's Lucky Chip",       "heroic_dex", 2, "hp", 5,
     "Lifted from the hoard of a notorious bandit.", 0.4),
    ("northkarana",  "Plains Lion Claw",          "heroic_str", 2, "attack", 3,
     "A razor-sharp claw from a Karana lion.", 0.3),
    ("southkarana",  "Centaur Hoof Chip",         "heroic_agi", 2, "avoidance", 3,
     "Polished smooth by the winds of the plains.", 0.3),
    ("eastkarana",   "Gorge Wind Stone",          "heroic_agi", 2, "hp", 5,
     "Tumbled smooth in the high Karana gorges.", 0.3),
    ("qey2hh1",      "Karana Hawk Talon",         "heroic_dex", 2, "attack", 3,
     "A talon from the great hawks of Western Karana.", 0.3),
    ("highpass",     "Highwayman's Gem Shard",    "heroic_cha", 2, "hp", 5,
     "A piece of a stolen gemstone, still sparkling.", 0.3),
    ("oasis",        "Desert Orc Tusk Chip",      "heroic_str", 2, "hp", 5,
     "A tusk fragment from a sand-bleached orc.", 0.3),
    ("oot",          "Seafarer's Coral Chip",     "heroic_sta", 2, "ac", 3,
     "A fragment of living coral from the Ocean of Tears.", 0.3),
    ("butcher",      "Dwarf Anvil Shard",         "heroic_sta", 2, "ac", 3,
     "A chip from a masterwork dwarven anvil.", 0.3),
    ("cauldron",     "Cauldron Mist Crystal",     "heroic_wis", 2, "manaregen", 2,
     "Condensed from the perpetual fog of Dagnor's Cauldron.", 0.3),
    ("gfaydark",     "Faydark Amber Chip",        "heroic_wis", 2, "hp", 5,
     "Ancient tree sap hardened into golden amber.", 0.4),
    ("lfaydark",     "Brownie Dust Shard",        "heroic_agi", 2, "avoidance", 3,
     "Shimmering fey dust compressed into solid form.", 0.3),
    ("steamfont",    "Clockwork Gear Chip",       "heroic_dex", 2, "attack", 3,
     "A tiny gear still humming with gnomish engineering.", 0.3),
    ("misty",        "Rivervale Lucky Stone",     "heroic_cha", 2, "hp", 5,
     "A halfling charm believed to bring good fortune.", 0.4),
    ("nektulos",     "Darkwood Thorn",            "heroic_str", 2, "hp", 5,
     "A thorn from the twisted trees of Nektulos.", 0.3),
    ("lakerathe",    "Rathe Lake Pearl",          "heroic_wis", 2, "healamt", 3,
     "A tiny pearl from the depths of Lake Rathetear.", 0.3),
    ("rathemtn",     "Hill Giant Knuckle Chip",   "heroic_str", 2, "hp", 8,
     "Chipped from the massive fist of a hill giant.", 0.3),
    ("nro",          "Desert Glass Shard",        "heroic_dex", 2, "spelldmg", 3,
     "Lightning-fused sand from the Northern Desert.", 0.3),
    ("sro",          "Scorched Innothule Chip",   "heroic_sta", 2, "regen", 2,
     "A heat-cracked stone from the southern wastes.", 0.3),
    ("erudsxing",    "Crossing Current Stone",    "heroic_agi", 2, "hp", 5,
     "Polished by the relentless currents of Erud's Crossing.", 0.3),
    ("tox",          "Kobold Fang Sliver",        "heroic_str", 2, "hp", 5,
     "A yellowed fang from a Toxxulia kobold.", 0.3),
    ("kerraridge",   "Kerra Claw Fragment",       "heroic_agi", 2, "avoidance", 3,
     "A curved claw shard from the fierce Kerran.", 0.3),
    ("jaggedpine",   "Pinecone Amber Chip",       "heroic_wis", 2, "regen", 2,
     "Sap-encased pinecone fragment from the ancient forest.", 0.3),
    ("qeytoqrg",     "Bear Claw Sliver",          "heroic_str", 2, "hp", 5,
     "From a massive Qeynos Hills bear.", 0.4),
    ("airplane",     "Skyshard Crystal",          "heroic_wis", 3, "manaregen", 5,
     "Fallen from the islands of the Plane of Sky.", 0.15),

    # ── Kunark (expansion 1) ──────────────────────────────────────────
    ("fieldofbone",  "Iksar Bone Chip",           "heroic_sta", 2, "ac", 3,
     "Ancient bone from the killing fields of Cabilis.", 0.3),
    ("kurn",         "Kurn's Relic Shard",        "heroic_int", 2, "spelldmg", 5,
     "A piece of Kurn's ancient magical defenses.", 0.3),
    ("karnor",       "Drolvarg Fang Chip",        "heroic_str", 3, "attack", 5,
     "A razor-sharp fang from a Drolvarg captain.", 0.25),
    ("citymist",     "Kunzar Jade Sliver",        "heroic_agi", 3, "avoidance", 5,
     "A jade shard from the ruins of the City of Mist.", 0.25),
    ("dreadlands",   "Dreadlands Ember",          "heroic_dex", 2, "attack", 5,
     "A smoldering ember from the scorched Dreadlands.", 0.3),
    ("emeraldjungle","Jungle Vine Crystal",       "heroic_sta", 2, "regen", 3,
     "Crystallized sap from the ancient jungle vines.", 0.3),
    ("nurga",        "Goblin Gem Sliver",         "heroic_int", 2, "hp", 5,
     "A gem fragment from the goblin mines.", 0.3),
    ("timorous",     "Raptor Scale Chip",         "heroic_agi", 2, "ac", 3,
     "An iridescent scale from a Timorous raptor.", 0.3),
    ("soltemple",    "Sun Temple Ember",          "heroic_int", 3, "spelldmg", 8,
     "Radiates the burning power of Solusek Ro.", 0.15),

    # ── Velious (expansion 2) ─────────────────────────────────────────
    ("cobaltscar",   "Cobalt Drake Scale",        "heroic_sta", 3, "ac", 5,
     "A brilliant blue scale from a cobalt drake.", 0.25),
    ("kael",         "Frost Giant Runestone",     "heroic_str", 3, "hp", 10,
     "Inscribed with the war runes of Kael Drakkel.", 0.2),
    ("sirens",       "Siren Song Pearl",          "heroic_wis", 3, "healamt", 5,
     "A pearl that hums with an enchanting melody.", 0.25),
    ("thurgadinb",   "Icewell Crystal",           "heroic_sta", 3, "ac", 8,
     "Formed in the deepest ice of Icewell Keep.", 0.2),
    ("templeveeshan","Dragon Heart Shard",        "heroic_str", 4, "heroic_sta", 3,
     "Pulsing with the ancient power of dragonkind.", 0.1),
    ("growthplane",  "Seed of Growth",            "heroic_wis", 3, "healamt", 8,
     "A living seed from the Plane of Growth.", 0.15),
    ("mischiefplane","Trickster's Gem",           "heroic_cha", 3, "heroic_agi", 3,
     "Shimmers with the chaotic energy of Mischief.", 0.15),
    ("sleeper",      "Sleeper's Eye Fragment",    "heroic_int", 4, "spelldmg", 10,
     "A crystal containing a fraction of Kerafyrm's gaze.", 0.1),
    ("warrens",      "Kobold King Fang",          "heroic_str", 2, "hp", 5,
     "A chipped fang from the Kobold King's crown.", 0.3),
]

# ============================================================
# Named Mob Multi-Stat Augments
# ============================================================
# These drop from any named (rare) mob via global_loot rare=1.
# Grouped by tier so we can filter by NPC level range.

NAMED_MULTI_STAT_AUGS = [
    # (name, stats_dict, lore, min_level, max_level)
    ("Shard of Balance",       {"heroic_str": 2, "heroic_sta": 2, "heroic_dex": 2, "heroic_agi": 2, "heroic_int": 2, "heroic_wis": 2, "heroic_cha": 2},
     "Radiates a perfect equilibrium of power.", 1, 30),
    ("Glowing Shard of Balance", {"heroic_str": 3, "heroic_sta": 3, "heroic_dex": 3, "heroic_agi": 3, "heroic_int": 3, "heroic_wis": 3, "heroic_cha": 3},
     "A brilliant crystal pulsing with balanced energy.", 25, 50),
    ("Brilliant Shard of Balance", {"heroic_str": 4, "heroic_sta": 4, "heroic_dex": 4, "heroic_agi": 4, "heroic_int": 4, "heroic_wis": 4, "heroic_cha": 4},
     "Perfected harmony of all virtues.", 46, 255),

    ("Warlord's Fragment",     {"heroic_str": 3, "heroic_sta": 3, "attack": 8},
     "Forged in the blood of countless battles.", 20, 45),
    ("Champion's Fragment",    {"heroic_str": 4, "heroic_sta": 4, "attack": 12, "hp": 10},
     "A warrior's essence, hardened by war.", 40, 255),

    ("Arcanist's Fragment",    {"heroic_int": 3, "heroic_wis": 3, "manaregen": 5},
     "Shimmers with residual spell energy.", 20, 45),
    ("Archmage's Fragment",    {"heroic_int": 4, "heroic_wis": 4, "manaregen": 8, "spelldmg": 5},
     "Pulses with concentrated arcane power.", 40, 255),

    ("Stalker's Fragment",     {"heroic_dex": 3, "heroic_agi": 3, "avoidance": 5},
     "Light as a whisper, sharp as a blade.", 20, 45),
    ("Predator's Fragment",    {"heroic_dex": 4, "heroic_agi": 4, "avoidance": 8, "attack": 5},
     "The essence of a perfect predator.", 40, 255),

    ("Healer's Fragment",      {"heroic_wis": 3, "heroic_sta": 2, "healamt": 5},
     "Warm to the touch, soothing to the soul.", 20, 45),
    ("Archpriest's Fragment",  {"heroic_wis": 4, "heroic_sta": 3, "healamt": 8, "regen": 3},
     "Blessed by the gods themselves.", 40, 255),
]

# Drop chance for named aug from a rare mob (percentage in lootdrop_entries)
NAMED_AUG_DROP_CHANCE = 5.0  # 5% from any named mob

# ============================================================
# Task Definitions
# ============================================================
# type 0 = Task (solo), type 1 = Shared, type 2 = Quest
# activitytype 2 = Kill
# replay_timer_seconds: 86400 = 24h, 604800 = 7d

DAILY_TASK_ID  = TASK_ID_BASE
WEEKLY_TASK_ID = TASK_ID_BASE + 1

DAILY_TASK = {
    "id": DAILY_TASK_ID,
    "type": 0,               # Solo task
    "title": "Daily: Essence Hunt",
    "description": "Prove your strength by slaying creatures across Norrath. "
                   "The Essence Provisioner will reward you handsomely.",
    "reward_text": "500 Common Essence, 20 Rare Essence",
    "min_level": 10,
    "max_level": 0,          # 0 = no cap
    "repeatable": 1,
    "replay_timer_seconds": 86400,  # 24 hours
    "activity": {
        "activitytype": 2,   # Kill
        "goalcount": 25,
        "description_override": "Slay 25 creatures",
    },
    "ce_reward": 500,
    "re_reward": 20,
}

WEEKLY_TASK = {
    "id": WEEKLY_TASK_ID,
    "type": 0,
    "title": "Weekly: Champion's Bounty",
    "description": "Embark on an extended hunt. The Essence Provisioner values "
                   "dedication and will bestow a generous Essence reward.",
    "reward_text": "2000 Common Essence, 150 Rare Essence",
    "min_level": 20,
    "max_level": 0,
    "repeatable": 1,
    "replay_timer_seconds": 604800,  # 7 days
    "activity": {
        "activitytype": 2,
        "goalcount": 150,
        "description_override": "Slay 150 creatures",
    },
    "ce_reward": 2000,
    "re_reward": 150,
}

# ============================================================
# Item Builder Helpers
# ============================================================

# Default item column values for augments (augtype 1 = general)
AUG_DEFAULTS = {
    "itemclass": 0,
    "itemtype": 54,        # Aug item type
    "augtype": 65535,      # Fits all slot types
    "augslot1type": 0,
    "augslot2type": 0,
    "augslot3type": 0,
    "augslot4type": 0,
    "augslot5type": 0,
    "augslot6type": 0,
    "augslot1unk2": 0,
    "augslot2unk2": 0,
    "augslot3unk2": 0,
    "augslot4unk2": 0,
    "augslot5unk2": 0,
    "augslot6unk2": 0,
    "weight": 0,
    "norent": 1,
    "nodrop": 1,
    "size": 0,
    "slots": 0,
    "price": 0,
    "icon": 2362,          # Generic gem icon
    "classes": 65535,       # All classes
    "races": 65535,         # All races
    "magic": 1,
    "stacksize": 1,
    "tradeskills": 0,
    "ldonprice": 0,
    "ldonsold": 0,
    "ldontheme": 0,
    "bagtype": 0,
    "bagslots": 0,
    "bagsize": 0,
    "bagwr": 0,
    "booktype": 0,
    "reqlevel": 0,
    "reclevel": 0,
    "banedmgrace": 0,
    "banedmgbody": 0,
    "banedmgamt": 0,
    "delay": 0,
    "damage": 0,
    "range": 0,
    "aagi": 0,
    "acha": 0,
    "adex": 0,
    "aint": 0,
    "asta": 0,
    "astr": 0,
    "awis": 0,
    "ac": 0,
    "hp": 0,
    "mana": 0,
    "endur": 0,
    "attack": 0,
    "avoidance": 0,
    "haste": 0,
    "manaregen": 0,
    "regen": 0,
    "spelldmg": 0,
    "healamt": 0,
    "heroic_str": 0,
    "heroic_sta": 0,
    "heroic_dex": 0,
    "heroic_agi": 0,
    "heroic_int": 0,
    "heroic_wis": 0,
    "heroic_cha": 0,
    "proceffect": 0,
    "proctype": 0,
    "proclevel2": 0,
    "proclevel": 0,
    "procrate": 0,
    "worneffect": 0,
    "worntype": 0,
    "wornlevel2": 0,
    "wornlevel": 0,
    "clickeffect": 0,
    "clicktype": 0,
    "clicklevel2": 0,
    "clicklevel": 0,
    "focuseffect": 0,
    "focustype": 0,
    "focuslevel2": 0,
    "focuslevel": 0,
    "scrolleffect": 0,
    "scrolltype": 0,
    "scrolllevel2": 0,
    "scrolllevel": 0,
    "bardeffect": 0,
    "bardeffecttype": 0,
    "bardlevel2": 0,
    "bardlevel": 0,
}


def esc(s):
    """Escape single quotes for SQL."""
    return str(s).replace("'", "''")


def build_aug_item(item_id, name, lore, stats, nodrop=1):
    """Build an item INSERT dict from defaults + stat overrides."""
    iv = dict(AUG_DEFAULTS)
    iv["id"] = item_id
    iv["Name"] = name
    iv["lore"] = lore[:79] if lore else name[:79]
    iv["idfile"] = "IT63"
    iv["nodrop"] = nodrop
    for k, v in stats.items():
        iv[k] = v
    return iv


def item_insert_sql(iv):
    """Generate a single INSERT INTO items statement from an item dict."""
    cols = list(iv.keys())
    vals = []
    for c in cols:
        v = iv[c]
        if isinstance(v, str):
            vals.append(f"'{esc(v)}'")
        else:
            vals.append(str(v))
    return f"INSERT INTO `items` (`{'`,`'.join(cols)}`) VALUES ({','.join(vals)});"


# ============================================================
# Main Generation
# ============================================================

def generate():
    lines = []
    ts = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    lines.append("-- ============================================================")
    lines.append("-- Step 10: Zone Drop Augs, Named Mob Augs, Daily/Weekly Quests")
    lines.append(f"-- Generated: {ts}")
    lines.append("-- Re-run tools/generate_zone_augs.py to regenerate")
    lines.append("-- ============================================================")
    lines.append("")

    # ── Cleanup ──
    lines.append("-- Clean up previous run")
    lines.append(f"DELETE FROM `items` WHERE id BETWEEN {ITEM_ID_BASE} AND {ITEM_ID_BASE + 999};")
    lines.append(f"DELETE FROM `items` WHERE id BETWEEN {NAMED_AUG_ID_BASE} AND {NAMED_AUG_ID_BASE + 999};")
    lines.append(f"DELETE FROM `lootdrop_entries` WHERE lootdrop_id BETWEEN {LOOTDROP_ID_BASE} AND {LOOTDROP_ID_BASE + 999};")
    lines.append(f"DELETE FROM `loottable_entries` WHERE loottable_id BETWEEN {LOOTTABLE_ID_BASE} AND {LOOTTABLE_ID_BASE + 999};")
    lines.append(f"DELETE FROM `lootdrop` WHERE id BETWEEN {LOOTDROP_ID_BASE} AND {LOOTDROP_ID_BASE + 999};")
    lines.append(f"DELETE FROM `loottable` WHERE id BETWEEN {LOOTTABLE_ID_BASE} AND {LOOTTABLE_ID_BASE + 999};")
    lines.append(f"DELETE FROM `global_loot` WHERE id BETWEEN {GLOBAL_LOOT_ID_BASE} AND {GLOBAL_LOOT_ID_BASE + 999};")
    lines.append(f"DELETE FROM `tasks` WHERE id IN ({DAILY_TASK_ID}, {WEEKLY_TASK_ID});")
    lines.append(f"DELETE FROM `task_activities` WHERE taskid IN ({DAILY_TASK_ID}, {WEEKLY_TASK_ID});")
    lines.append("")

    # ── Zone Flavor Augments ──
    lines.append("-- ============================================================")
    lines.append("-- ZONE FLAVOR AUGMENTS (ultra-rare global_loot drops)")
    lines.append("-- ============================================================")
    lines.append("")

    item_count = 0
    global_loot_count = 0

    for idx, (zone, aug_name, pstat, pval, sstat, sval, flavor, chance) in enumerate(ZONE_FLAVOR_AUGS):
        item_id = ITEM_ID_BASE + idx
        loottable_id = LOOTTABLE_ID_BASE + idx
        lootdrop_id = LOOTDROP_ID_BASE + idx
        global_loot_id = GLOBAL_LOOT_ID_BASE + idx

        stats = {pstat: pval, sstat: sval}
        # Tradeable zone drops per design doc
        iv = build_aug_item(item_id, aug_name, flavor, stats, nodrop=0)
        lines.append(f"-- {zone}: {aug_name}")
        lines.append(item_insert_sql(iv))

        # Lootdrop + entry
        safe_aug = aug_name.replace("'", "").replace(" ", "_")
        lines.append(f"INSERT INTO `lootdrop` (`id`,`name`) VALUES ({lootdrop_id}, 'ZoneAug_{zone}_{safe_aug}');")
        lines.append(f"INSERT INTO `lootdrop_entries` (`lootdrop_id`,`item_id`,`item_charges`,`equip_item`,`chance`,`multiplier`) "
                      f"VALUES ({lootdrop_id}, {item_id}, 1, 0, {chance}, 1);")

        # Loottable + entry (droplimit=1, mindrop=0 = 0-1 of this drop per kill)
        lines.append(f"INSERT INTO `loottable` (`id`,`name`) VALUES ({loottable_id}, 'LT_ZoneAug_{zone}');")
        lines.append(f"INSERT INTO `loottable_entries` (`loottable_id`,`lootdrop_id`,`multiplier`,`droplimit`,`mindrop`,`probability`) "
                      f"VALUES ({loottable_id}, {lootdrop_id}, 1, 1, 0, 100);")

        # Global loot entry — zone-specific
        lines.append(f"INSERT INTO `global_loot` (`id`,`description`,`loottable_id`,`enabled`,`min_level`,`max_level`,`zone`) "
                      f"VALUES ({global_loot_id}, 'Step10_ZoneAug_{zone}', {loottable_id}, 1, 0, 0, '{esc(zone)}');")
        lines.append("")

        item_count += 1
        global_loot_count += 1

    # ── Named Mob Multi-Stat Augments ──
    lines.append("-- ============================================================")
    lines.append("-- NAMED MOB MULTI-STAT AUGMENTS (global_loot with rare=1)")
    lines.append("-- ============================================================")
    lines.append("")

    named_start_idx = len(ZONE_FLAVOR_AUGS)  # offset past zone augs
    named_item_count = 0

    for idx, (name, stats, lore, min_lv, max_lv) in enumerate(NAMED_MULTI_STAT_AUGS):
        item_id = NAMED_AUG_ID_BASE + idx
        lootdrop_id = LOOTDROP_ID_BASE + named_start_idx + idx
        loottable_id = LOOTTABLE_ID_BASE + named_start_idx + idx
        global_loot_id = GLOBAL_LOOT_ID_BASE + named_start_idx + idx

        iv = build_aug_item(item_id, name, lore, stats, nodrop=0)
        lines.append(f"-- Named aug: {name} (level {min_lv}-{max_lv})")
        lines.append(item_insert_sql(iv))

        # Each named aug gets its own lootdrop so we can control per-aug chance
        safe_name = name.replace("'", "").replace(" ", "_")
        lines.append(f"INSERT INTO `lootdrop` (`id`,`name`) VALUES ({lootdrop_id}, 'NamedAug_{safe_name}');")
        lines.append(f"INSERT INTO `lootdrop_entries` (`lootdrop_id`,`item_id`,`item_charges`,`equip_item`,`chance`,`multiplier`) "
                      f"VALUES ({lootdrop_id}, {item_id}, 1, 0, 100, 1);")
        lines.append("")

        named_item_count += 1

    # Now create loottables that group named augs by level tier, with droplimit=1
    # Tier groupings: low (1-30), mid (20-50), high (40+)
    tiers = [
        ("low",  1,  30),
        ("mid",  20, 50),
        ("high", 40, 255),
    ]

    named_lt_base = LOOTTABLE_ID_BASE + named_start_idx + len(NAMED_MULTI_STAT_AUGS)
    named_gl_base = GLOBAL_LOOT_ID_BASE + named_start_idx + len(NAMED_MULTI_STAT_AUGS)

    for tier_idx, (tier_name, tier_min, tier_max) in enumerate(tiers):
        lt_id = named_lt_base + tier_idx
        gl_id = named_gl_base + tier_idx

        # Collect lootdrop IDs for augs that overlap this tier
        matching_drops = []
        for idx, (name, stats, lore, min_lv, max_lv) in enumerate(NAMED_MULTI_STAT_AUGS):
            if min_lv <= tier_max and max_lv >= tier_min:
                ld_id = LOOTDROP_ID_BASE + named_start_idx + idx
                matching_drops.append(ld_id)

        if not matching_drops:
            continue

        lines.append(f"-- Named aug loottable: {tier_name} tier (NPC level {tier_min}-{tier_max})")
        lines.append(f"INSERT INTO `loottable` (`id`,`name`) VALUES ({lt_id}, 'LT_NamedAug_{tier_name}');")
        for ld_id in matching_drops:
            # droplimit=1 across the whole loottable = pick at most 1 aug total
            lines.append(f"INSERT INTO `loottable_entries` (`loottable_id`,`lootdrop_id`,`multiplier`,`droplimit`,`mindrop`,`probability`) "
                          f"VALUES ({lt_id}, {ld_id}, 1, 1, 0, {NAMED_AUG_DROP_CHANCE});")

        # Global loot: rare=1 means only rare/named mobs get this
        lines.append(f"INSERT INTO `global_loot` (`id`,`description`,`loottable_id`,`enabled`,`min_level`,`max_level`,`rare`) "
                      f"VALUES ({gl_id}, 'Step10_NamedAug_{tier_name}', {lt_id}, 1, {tier_min}, {tier_max}, 1);")
        lines.append("")

        global_loot_count += 1

    # ── Daily & Weekly Tasks ──
    lines.append("-- ============================================================")
    lines.append("-- DAILY & WEEKLY ESSENCE QUESTS")
    lines.append("-- ============================================================")
    lines.append("")

    for task_def in [DAILY_TASK, WEEKLY_TASK]:
        tid = task_def["id"]
        act = task_def["activity"]

        # The task rewards Essence via reward_id_list (item rewards).
        # Format: item_id1|item_id2  (pipe-separated item IDs)
        # We give stacks: CE item * ce_reward, RE item * re_reward
        # But EQ task system reward_id_list gives 1 of each item listed.
        # For currency rewards, we use reward_point_type + reward_points for one currency,
        # and we'll use a Lua quest script for the second currency.
        #
        # Actually, the cleanest way is to use alternate currency rewards via
        # a completion script. But since this is SQL-only, we'll reward Common
        # Essence via reward_point_type (alt currency ID) + reward_points, and
        # handle Rare Essence via a small reward item stack.
        #
        # Simpler approach: reward_method=2 means "all items in list".
        # reward_id_list can be pipe-separated item_id values but only gives 1 each.
        #
        # Best approach for dual-currency: Use faction_reward / faction_amount for one,
        # and an alternate currency for the other... but these don't map cleanly.
        #
        # PRACTICAL SOLUTION: We'll award Common Essence via the task's built-in
        # reward_point_type (alternate currency), and create a one-time "Rare Essence
        # Pouch" reward item that gives RE on click (or just award RE item via
        # reward_id_list). Since the task system can give items, we'll create a
        # "Bag of Rare Essence" consumable that awards RE on use... but that's complex.
        #
        # SIMPLEST: Use Lua quest script for the completion reward. We'll create a
        # minimal Lua global player script that hooks EVENT_TASK_COMPLETE.
        #
        # ACTUALLY SIMPLEST: reward_point_type supports alternate currencies directly.
        # We award CE via that. For RE, the task gives a "Rare Essence Cache" item
        # (a stackable item the player clicks to receive RE... or we just award the
        # Rare Essence currency item itself as a stack).
        #
        # Let's use: reward_point_type = CE currency ID, reward_points = CE amount.
        # And: reward_id_list = RE currency item ID (200011), with a Lua script to
        # give the right stack count...
        #
        # FINAL DECISION: Use Lua EVENT_TASK_COMPLETE hook. It's 10 lines and handles
        # both currencies cleanly. The task itself just shows the reward_text.

        lines.append(f"-- {task_def['title']}")
        lines.append(
            f"INSERT INTO `tasks` (`id`,`type`,`title`,`description`,`reward_text`,"
            f"`min_level`,`max_level`,`repeatable`,`replay_timer_seconds`,`replay_timer_group`,"
            f"`reward_method`,`reward_points`,`reward_point_type`,`enabled`) "
            f"VALUES ({tid}, {task_def['type']}, '{esc(task_def['title'])}', "
            f"'{esc(task_def['description'])}', '{esc(task_def['reward_text'])}', "
            f"{task_def['min_level']}, {task_def['max_level']}, {task_def['repeatable']}, "
            f"{task_def['replay_timer_seconds']}, {tid}, "  # replay_timer_group = task id
            f"0, 0, 0, 1);"  # reward handled by Lua script
        )
        lines.append(
            f"INSERT INTO `task_activities` (`taskid`,`activityid`,`req_activity_id`,`step`,"
            f"`activitytype`,`goalcount`,`description_override`) "
            f"VALUES ({tid}, 0, -1, 0, {act['activitytype']}, {act['goalcount']}, "
            f"'{esc(act['description_override'])}');"
        )
        lines.append("")

    # ── Summary ──
    total_items = item_count + named_item_count
    lines.append(f"-- Summary: {item_count} zone flavor augs, {named_item_count} named mob augs, "
                  f"{global_loot_count} global_loot entries, 2 tasks")

    sql_path = os.path.join(os.path.dirname(__file__), "..",
                            "utils", "sql", "git", "optional",
                            "step10_zone_augs_and_quests.sql")
    sql_path = os.path.normpath(sql_path)

    with open(sql_path, "w", encoding="utf-8") as f:
        f.write("\n".join(lines) + "\n")
    print(f"Generated {sql_path}")
    print(f"  {item_count} zone flavor augs")
    print(f"  {named_item_count} named mob multi-stat augs")
    print(f"  {global_loot_count} global_loot entries")
    print(f"  2 tasks (daily + weekly)")

    # ── Generate Lua completion script ──
    lua_lines = []
    lua_lines.append("-- Step 10: Daily/Weekly Essence Quest completion rewards")
    lua_lines.append("-- Place in quests/global/player.lua (or append to existing)")
    lua_lines.append("")
    lua_lines.append("function event_task_complete(e)")
    daily_ce = DAILY_TASK['ce_reward']
    daily_re = DAILY_TASK['re_reward']
    weekly_ce = WEEKLY_TASK['ce_reward']
    weekly_re = WEEKLY_TASK['re_reward']
    lua_lines.append(f"    if e.task_id == {DAILY_TASK_ID} then")
    lua_lines.append(f"        e.self:AddAlternateCurrencyValue({COMMON_ESSENCE_CURRENCY}, {daily_ce})")
    lua_lines.append(f"        e.self:AddAlternateCurrencyValue({RARE_ESSENCE_CURRENCY}, {daily_re})")
    lua_lines.append(f'        e.self:Message(15, "You receive {daily_ce} Common Essence and {daily_re} Rare Essence!")')
    lua_lines.append(f"    elseif e.task_id == {WEEKLY_TASK_ID} then")
    lua_lines.append(f"        e.self:AddAlternateCurrencyValue({COMMON_ESSENCE_CURRENCY}, {weekly_ce})")
    lua_lines.append(f"        e.self:AddAlternateCurrencyValue({RARE_ESSENCE_CURRENCY}, {weekly_re})")
    lua_lines.append(f'        e.self:Message(15, "You receive {weekly_ce} Common Essence and {weekly_re} Rare Essence!")')
    lua_lines.append("    end")
    lua_lines.append("end")

    lua_path = os.path.join(os.path.dirname(__file__), "..",
                            "quests", "global", "player_task_rewards.lua")
    lua_path = os.path.normpath(lua_path)

    os.makedirs(os.path.dirname(lua_path), exist_ok=True)
    with open(lua_path, "w", encoding="utf-8") as f:
        f.write("\n".join(lua_lines) + "\n")
    print(f"Generated {lua_path}")


if __name__ == "__main__":
    generate()
