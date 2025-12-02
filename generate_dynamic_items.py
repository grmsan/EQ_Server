#!/usr/bin/env python3
"""
Generate dynamic items in the database for testing or pre-population.
This mimics what DynamicItemManager::InsertItemIntoDatabase does.
"""

import pymysql
import sys

# Database configuration (matches eqemu_config.json)
DB_CONFIG = {
    'host': '127.0.0.1',
    'port': 3308,
    'user': 'root',
    'password': 'eqemu',
    'database': 'peq',
    'charset': 'utf8mb4'
}

def generate_dynamic_id(base_id, level):
    """Generate dynamic ID using format: 5LLLIIIIII"""
    if level == 0:
        return base_id

    LEVEL_OFFSET = 100
    MAX_LEVEL = 250

    if level > MAX_LEVEL:
        print(f"Warning: level {level} exceeds max {MAX_LEVEL}, capping")
        level = MAX_LEVEL

    encoded_level = LEVEL_OFFSET + min(level, MAX_LEVEL)
    encoded_base = base_id % 1000000  # Limit to 6 digits

    # Format: 5LLLIIIIII (5 + 3-digit level + 6-digit base ID)
    dynamic_id = 5000000000 + (encoded_level * 1000000) + encoded_base

    return dynamic_id

def item_exists(cursor, item_id):
    """Check if item exists in database"""
    cursor.execute("SELECT id FROM items WHERE id = %s", (item_id,))
    return cursor.fetchone() is not None

def create_dynamic_item(cursor, base_id, level):
    """Create a dynamic item by copying base item and updating stats"""
    dynamic_id = generate_dynamic_id(base_id, level)

    print(f"Generating: Base ID {base_id} at level {level} -> Dynamic ID {dynamic_id}")

    # Check if already exists
    if item_exists(cursor, dynamic_id):
        print(f"  Item {dynamic_id} already exists, skipping")
        return dynamic_id

    # Step 1: Copy base item row with new ID
    copy_query = """
    INSERT INTO items SELECT %s, name, aagi, ac, accuracy, acha, adex, aint, artifactflag, asta, astr, attack,
    augrestrict, augslot1type, augslot1visible, augslot2type, augslot2visible, augslot3type, augslot3visible,
    augslot4type, augslot4visible, augslot5type, augslot5visible, augslot6type, augslot6visible, augtype,
    avoidance, awis, bagsize, bagslots, bagtype, bagwr, banedmgamt, banedmgbody, banedmgrace, bardtype,
    bardvalue, book, casttime, casttime_, charmfile, charmfileid, classes, color, combateffects, damage, extradmgamt,
    extradmgskill, deity, delay, augdistiller, dotshielding, dr, clicktype, clicklevel2, elemdmgtype,
    elemdmgamt, endur, factionamt1, factionamt2, factionamt3, factionamt4, factionmod1, factionmod2,
    factionmod3, factionmod4, filename, focuseffect, fr, fvnodrop, haste, clicklevel, hp, regen, icon,
    idfile, itemclass, itemtype, ldonprice, ldontheme, ldonsold, light, lore, loregroup, magic, mana,
    manaregen, enduranceregen, material, herosforgemodel, maxcharges, mr, nodrop, norent, pendingloreflag,
    pr, procrate, races, `range`, reclevel, recskill, reqlevel, sellrate, shielding, size, skillmodtype,
    skillmodvalue, slots, clickeffect, spellshield, strikethrough, stunresist, summonedflag, tradeskills,
    favor, weight, unk012, unk013, benefitflag, unk054, unk059, booktype, recastdelay, recasttype, guildfavor,
    unk123, unk124, attuneable, nopet, updated, comment, unk127, pointtype, potionbelt, potionbeltslots,
    stacksize, notransfer, stackable, unk134, unk137, proceffect, proctype, proclevel2, proclevel, unk142,
    worneffect, worntype, wornlevel2, wornlevel, unk147, focustype, focuslevel2, focuslevel, unk152, scrolleffect,
    scrolltype, scrolllevel2, scrolllevel, unk157, serialized, verified, serialization, source, unk033, lorefile,
    unk014, svcorruption, skillmodmax, unk060, augslot1unk2, augslot2unk2, augslot3unk2, augslot4unk2, augslot5unk2,
    augslot6unk2, unk120, unk121, questitemflag, unk132, clickunk5, clickunk6, clickunk7, procunk1, procunk2,
    procunk3, procunk4, procunk6, procunk7, wornunk1, wornunk2, wornunk3, wornunk4, wornunk5, wornunk6, wornunk7,
    focusunk1, focusunk2, focusunk3, focusunk4, focusunk5, focusunk6, focusunk7, scrollunk1, scrollunk2, scrollunk3,
    scrollunk4, scrollunk5, scrollunk6, scrollunk7, unk193, purity, evoitem, evoid, evolvinglevel, evomax, clickname,
    procname, wornname, focusname, scrollname, dsmitigation, heroic_str, heroic_int, heroic_wis, heroic_agi,
    heroic_dex, heroic_sta, heroic_cha, heroic_pr, heroic_dr, heroic_fr, heroic_cr, heroic_mr, heroic_svcorrup,
    healamt, spelldmg, clairvoyance, backstabdmg, created, elitematerial, ldonsellbackrate, scriptfileid,
    expendablearrow, powersourcecapacity, bardeffect, bardeffecttype, bardlevel2, bardlevel, bardunk1, bardunk2,
    bardunk3, bardunk4, bardunk5, bardname, bardunk7, unk214, subtype, unk220, unk221, heirloom, unk223, unk224,
    unk225, unk226, unk227, unk228, unk229, unk230, unk231, unk232, unk233, unk234, placeable, unk236, unk237,
    unk238, unk239, unk240, unk241, epicitem FROM items WHERE id = %s
    """

    try:
        cursor.execute(copy_query, (dynamic_id, base_id))
        print(f"  Copied base item {base_id} to {dynamic_id}")
    except Exception as e:
        print(f"  ERROR copying item: {e}")
        return None

    # Step 2: Update name with level
    cursor.execute("SELECT name FROM items WHERE id = %s", (base_id,))
    result = cursor.fetchone()
    if result:
        base_name = result[0]
        new_name = f"{base_name} +{level}"
        cursor.execute("UPDATE items SET name = %s WHERE id = %s", (new_name, dynamic_id))
        print(f"  Updated name to: {new_name}")

    print(f"  SUCCESS: Created dynamic item {dynamic_id}")
    return dynamic_id

def generate_item_range(cursor, base_id, start_level, end_level):
    """Generate a range of levels for a base item"""
    print(f"\nGenerating levels {start_level}-{end_level} for base item {base_id}")
    created = []
    for level in range(start_level, end_level + 1):
        dynamic_id = create_dynamic_item(cursor, base_id, level)
        if dynamic_id:
            created.append(dynamic_id)
    return created

def main():
    if len(sys.argv) < 2:
        print("Usage:")
        print("  python generate_dynamic_items.py <base_id> [level]")
        print("  python generate_dynamic_items.py <base_id> <start_level> <end_level>")
        print("\nExamples:")
        print("  python generate_dynamic_items.py 1001 1          # Generate Cloth Cap +1")
        print("  python generate_dynamic_items.py 9998 1 10       # Generate Short Sword +1 through +10")
        sys.exit(1)

    base_id = int(sys.argv[1])

    if len(sys.argv) == 3:
        level = int(sys.argv[2])
        start_level = level
        end_level = level
    elif len(sys.argv) == 4:
        start_level = int(sys.argv[2])
        end_level = int(sys.argv[3])
    else:
        print("Invalid arguments")
        sys.exit(1)

    # Connect to database
    print(f"Connecting to database: {DB_CONFIG['host']}:{DB_CONFIG['port']}/{DB_CONFIG['database']}")
    conn = pymysql.connect(**DB_CONFIG)
    cursor = conn.cursor()

    try:
        # Generate items
        created = generate_item_range(cursor, base_id, start_level, end_level)

        # Commit changes
        conn.commit()
        print(f"\nCreated {len(created)} dynamic items")
        print("Don't forget to restart shared_memory to reload items!")

    except Exception as e:
        print(f"ERROR: {e}")
        conn.rollback()
    finally:
        cursor.close()
        conn.close()

if __name__ == '__main__':
    main()
