#!/usr/bin/env python3
"""
Clear Custom Item Data
Removes custom_data from character inventory to prevent client crashes.
Creates a backup before making changes.
"""

import json
import sys
from pathlib import Path
from datetime import datetime

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

def backup_custom_items(cursor):
    """Create a backup of all items with custom_data"""
    backup_table = f"inventory_custom_backup_{datetime.now().strftime('%Y%m%d_%H%M%S')}"

    print(f"Creating backup table: {backup_table}")
    cursor.execute(f"""
        CREATE TABLE {backup_table} AS
        SELECT * FROM inventory WHERE custom_data IS NOT NULL AND custom_data != ''
    """)

    cursor.execute(f"SELECT COUNT(*) FROM {backup_table}")
    count = cursor.fetchone()[0]
    print(f"✓ Backed up {count} items with custom_data to {backup_table}")
    return backup_table, count

def analyze_custom_items(cursor):
    """Show which characters have custom items"""
    cursor.execute("""
        SELECT cd.id, cd.name, COUNT(*) as item_count
        FROM inventory i
        JOIN character_data cd ON i.character_id = cd.id
        WHERE i.custom_data IS NOT NULL AND i.custom_data != ''
        GROUP BY cd.id, cd.name
        ORDER BY item_count DESC
    """)

    affected = cursor.fetchall()

    if not affected:
        print("\n✓ No characters have items with custom_data")
        return []

    print(f"\nCharacters with custom items:")
    print("-" * 60)
    total_items = 0
    for char_id, name, count in affected:
        print(f"  {name} (ID: {char_id}): {count} items")
        total_items += count
    print("-" * 60)
    print(f"Total: {len(affected)} characters, {total_items} items\n")

    return affected

def clear_custom_data(cursor, character_id=None):
    """Clear custom_data from inventory"""
    if character_id:
        cursor.execute("""
            UPDATE inventory
            SET custom_data = NULL
            WHERE character_id = %s AND custom_data IS NOT NULL AND custom_data != ''
        """, (character_id,))
    else:
        cursor.execute("""
            UPDATE inventory
            SET custom_data = NULL
            WHERE custom_data IS NOT NULL AND custom_data != ''
        """)

    return cursor.rowcount

def main():
    print("=" * 80)
    print("Clear Custom Item Data")
    print("=" * 80)
    print()

    # Parse arguments
    character_name = None
    if len(sys.argv) > 1:
        if sys.argv[1] in ['-h', '--help']:
            print("Usage:")
            print("  python clear_custom_items.py              # Clear all characters")
            print("  python clear_custom_items.py <name>       # Clear specific character")
            print("  python clear_custom_items.py --dry-run    # Show what would be cleared")
            return
        elif sys.argv[1] != '--dry-run':
            character_name = sys.argv[1]

    dry_run = '--dry-run' in sys.argv

    # Connect to database
    db_config = load_db_config()
    conn = pymysql.connect(**db_config)
    cursor = conn.cursor()

    try:
        # Analyze current state
        affected = analyze_custom_items(cursor)

        if not affected:
            return

        if dry_run:
            print("DRY RUN - No changes will be made")
            return

        # Get character ID if name provided
        char_id = None
        if character_name:
            cursor.execute("SELECT id FROM character_data WHERE name = %s", (character_name,))
            result = cursor.fetchone()
            if not result:
                print(f"❌ Character not found: {character_name}")
                return
            char_id = result[0]
            print(f"Targeting character: {character_name} (ID: {char_id})\n")

        # Confirm action
        if char_id:
            response = input(f"Clear custom_data for {character_name}? [y/N]: ")
        else:
            response = input(f"Clear custom_data for ALL {len(affected)} characters? [y/N]: ")

        if response.lower() != 'y':
            print("Cancelled")
            return

        print()

        # Create backup
        backup_table, backup_count = backup_custom_items(cursor)
        conn.commit()

        # Clear custom data
        print("\nClearing custom_data...")
        cleared = clear_custom_data(cursor, char_id)
        conn.commit()

        print(f"✓ Cleared custom_data from {cleared} items")
        print()
        print("=" * 80)
        print("✅ SUCCESS")
        print("=" * 80)
        print(f"Backup table: {backup_table} ({backup_count} items)")
        print(f"Items cleared: {cleared}")
        print()
        print("To restore from backup:")
        print(f"  UPDATE inventory i")
        print(f"  JOIN {backup_table} b ON i.character_id = b.character_id AND i.slot_id = b.slot_id")
        print(f"  SET i.custom_data = b.custom_data")
        print()
        print("Characters should now load without crashing.")

    except Exception as e:
        conn.rollback()
        print(f"\n❌ Error: {e}")
        import traceback
        traceback.print_exc()
    finally:
        cursor.close()
        conn.close()

if __name__ == "__main__":
    main()
