import pymysql, json
cfg = json.load(open('eqemu_config.json'))['server']['database']
conn = pymysql.connect(host=cfg['host'], port=cfg['port'], user=cfg['username'], password=cfg['password'], database=cfg['db'])
cur = conn.cursor()

# Check what custom_data actually contains
print("Checking custom_data values:")
print("=" * 80)
cur.execute("""
    SELECT cd.name, i.slot_id, i.item_id,
           LENGTH(i.custom_data) as len,
           i.custom_data,
           CHAR_LENGTH(i.custom_data) as char_len,
           i.custom_data IS NOT NULL as is_not_null,
           i.custom_data = '' as is_empty,
           i.custom_data = 'NULL' as is_null_string
    FROM inventory i
    JOIN character_data cd ON i.character_id = cd.id
    ORDER BY cd.id, i.slot_id
    LIMIT 20
""")

for row in cur.fetchall():
    name, slot, item_id, byte_len, data, char_len, is_not_null, is_empty, is_null_str = row
    print(f"\nChar: {name}, Slot: {slot}, Item: {item_id}")
    print(f"  Length: {byte_len} bytes, {char_len} chars")
    print(f"  IS NOT NULL: {is_not_null}, Empty string: {is_empty}, 'NULL' string: {is_null_str}")
    if data:
        print(f"  Value: {repr(data[:50])}")
