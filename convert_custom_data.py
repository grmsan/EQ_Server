import mysql.connector
import json

# Connect to database
config = {
    'user': 'root',
    'password': '110987',
    'host': 'localhost',
    'port': 3308,
    'database': 'peq'
}

conn = mysql.connector.connect(**config)
cursor = conn.cursor()

# Get all items with JSON custom_data
cursor.execute("""
    SELECT character_id, slot_id, custom_data
    FROM inventory
    WHERE custom_data LIKE '{%}'
""")

rows = cursor.fetchall()
print(f"Found {len(rows)} items with JSON custom_data")

converted = 0
for character_id, slot_id, custom_data in rows:
    if not custom_data or custom_data == '':
        continue

    try:
        # Parse JSON
        data = json.loads(custom_data)

        # Convert to ^-delimited format
        parts = []
        for key, value in data.items():
            parts.append(str(key))
            parts.append(str(value))

        new_format = '^'.join(parts) + '^'

        # Update database
        cursor.execute("""
            UPDATE inventory
            SET custom_data = %s
            WHERE character_id = %s AND slot_id = %s
        """, (new_format, character_id, slot_id))

        converted += 1
        print(f"Converted: character_id={character_id}, slot_id={slot_id}, old='{custom_data}' new='{new_format}'")

    except Exception as e:
        print(f"Error converting character_id={character_id}, slot_id={slot_id}: {e}")

conn.commit()
print(f"\nConverted {converted} items")

# Show some examples
cursor.execute("""
    SELECT character_id, slot_id, custom_data
    FROM inventory
    WHERE custom_data LIKE '%dynamic_level%'
    LIMIT 10
""")
print("\nSample results:")
for row in cursor.fetchall():
    print(row)

cursor.close()
conn.close()
