import json, mysql.connector

with open('eqemu_config.json') as f:
    c = json.load(f)
db = c['server']['database']
conn = mysql.connector.connect(host=db['host'], port=int(db.get('port',3306)),
    user=db['username'], password=db['password'], database=db['db'])
cur = conn.cursor(dictionary=True)

# Fungus Covered Scale Tunic (armor, ID 2735) and a weapon (Short Sword, ID 5023)
# Also check a known weapon: Rusty Bastard Sword (2H, ID 5028)
ids = [2735, 252735, 502735, 752735, 5023, 255023, 505023, 755023, 5028, 255028, 505028, 755028]
cur.execute(f"SELECT id, Name, itemtype, "
            f"augslot1type, augslot2type, augslot3type, augslot4type, augslot5type, augslot6type "
            f"FROM items WHERE id IN ({','.join(str(i) for i in ids)}) ORDER BY id")

for row in cur.fetchall():
    slots = [row[f'augslot{i}type'] for i in range(1,7)]
    print(f"ID={row['id']:>7} type={row['itemtype']:>2}  {row['Name']:<45}  Slots={slots}")

conn.close()
