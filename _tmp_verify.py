import json, mysql.connector

with open('eqemu_config.json') as f:
    c = json.load(f)
db = c['server']['database']
conn = mysql.connector.connect(host=db['host'], port=int(db.get('port',3306)),
    user=db['username'], password=db['password'], database=db['db'])
cur = conn.cursor(dictionary=True)

ids = [2735, 252735, 502735, 752735]
cur.execute(f"SELECT id, Name, astr, asta, aagi, adex, aint, awis, acha, "
            f"augslot1type, augslot2type, augslot3type, augslot4type, augslot5type, augslot6type "
            f"FROM items WHERE id IN ({','.join(str(i) for i in ids)}) ORDER BY id")

for row in cur.fetchall():
    slots = [row[f'augslot{i}type'] for i in range(1,7)]
    print(f"ID={row['id']:>7}  {row['Name']:<45}  "
          f"STR={row['astr']:>4} AGI={row['aagi']:>4} DEX={row['adex']:>4}  "
          f"Slots={slots}")

conn.close()
