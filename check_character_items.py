#!/usr/bin/env python3
"""
Check character inventory for items with custom data that might cause client crashes
"""

import json
import sys
from pathlib import Path

try:
    import pymysql
except ImportError:
    print("Installing pymysql...")
    import subprocess
    subprocess.check_call([sys.executable, "-m", "pip", "install", "pymysql"])
    import pymysql

def load_db_config():
    config_path = Path(__file__).parent / "eqemu_config.json"
    with open(config_path, 'r') as f:
        config = json.load(f)
    db = config['server']['database']
    return {
        'host': db['host'],
        'port': db['port'],
        'user': db['username'],
        'password': db['password'],
        'database': db['db']
    }

def check_character_items(char_name=None):
    db_config = load_db_config()
    conn = pymysql.connect(**db_config)
    cursor = conn.cursor()

    # Get characters
    if char_name:
        cursor.execute("SELECT id, name FROM character_data WHERE name = %s", (char_name,))
    else:
        cursor.execute("SELECT id, name FROM character_data ORDER BY id DESC LIMIT 5")

    characters = cursor.fetchall()

    if not characters:
        print(f"No character found: {char_name}")
        return

    print("=" * 80)
    print("Character Item Analysis")
    print("=" * 80)

    for char_id, name in characters:
        print(f"\nCharacter: {name} (ID: {char_id})")
        print("-" * 80)

        # Check inventory items
        cursor.execute("""
            SELECT slot_id, item_id, charges, custom_data, ornament_icon, ornament_idfile, ornament_hero_model
            FROM inventory
            WHERE character_id = %s
            ORDER BY slot_id
        """, (char_id,))

        items = cursor.fetchall()

        if not items:
            print("  No items in inventory")
            continue

        print(f"  Total items: {len(items)}")

        # Check for items with custom data
        custom_items = [item for item in items if item[3]]  # custom_data column
        if custom_items:
            print(f"\n  ⚠️  Items with custom_data: {len(custom_items)}")
            for slotid, itemid, charges, custom_data, ornament_icon, ornament_idfile, ornament_model in custom_items:
                print(f"    Slot {slotid}: Item {itemid} - Custom data length: {len(custom_data) if custom_data else 0}")

        # Check for ornaments (can also cause issues)
        ornament_items = [item for item in items if item[4] or item[5]]
        if ornament_items:
            print(f"\n  Items with ornaments: {len(ornament_items)}")

        # Check for dynamic item IDs (our new system)
        dynamic_items = [item for item in items if item[1] >= 500000000 and item[1] < 600000000]
        if dynamic_items:
            print(f"\n  Dynamic item IDs detected: {len(dynamic_items)}")
            for slotid, itemid, charges, *_ in dynamic_items:
                level = (itemid // 1000000) % 100000
                base_id = itemid % 1000000
                print(f"    Slot {slotid}: ID {itemid} = Base {base_id} +{level}")

    cursor.close()
    conn.close()

if __name__ == "__main__":
    char_name = sys.argv[1] if len(sys.argv) > 1 else None
    check_character_items(char_name)
