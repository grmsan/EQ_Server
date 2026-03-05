import json, mysql.connector

with open('eqemu_config.json') as f:
    c = json.load(f)
db = c['server']['database']
conn = mysql.connector.connect(
    host=db['host'], port=int(db.get('port', 3306)),
    user=db['username'], password=db['password'], database=db['db']
)
cur = conn.cursor()

# Find base fungus tunic
cur.execute("SELECT id, Name, astr, asta, aagi, adex, aint, awis, acha, "
            "augslot1type, augslot2type, augslot3type, augslot4type, augslot5type, augslot6type, "
            "augslot1visible, augslot2visible, augslot3visible, augslot4visible, augslot5visible, augslot6visible "
            "FROM items WHERE Name LIKE '%fungus%scale%tunic%' AND id < 250000")
rows = cur.fetchall()
print("=== Base fungus tunic items ===")
for r in rows:
    print(f"  ID={r[0]}  Name={r[1]}")
    print(f"    STR={r[2]} STA={r[3]} AGI={r[4]} DEX={r[5]} INT={r[6]} WIS={r[7]} CHA={r[8]}")
    print(f"    SlotTypes=[{r[9]},{r[10]},{r[11]},{r[12]},{r[13]},{r[14]}]")
    print(f"    SlotVis  =[{r[15]},{r[16]},{r[17]},{r[18]},{r[19]},{r[20]}]")

    # Now fetch tiered versions
    base_id = r[0]
    for offset, name in [(250000, "Enchanted"), (500000, "Legendary"), (750000, "Mythic")]:
        tid = base_id + offset
        cur.execute("SELECT id, Name, astr, asta, aagi, adex, aint, awis, acha, "
                    "augslot1type, augslot2type, augslot3type, augslot4type, augslot5type, augslot6type, "
                    "augslot1visible, augslot2visible, augslot3visible, augslot4visible, augslot5visible, augslot6visible "
                    "FROM items WHERE id = %s", (tid,))
        tr = cur.fetchone()
        if tr:
            print(f"  {name} ID={tr[0]}  Name={tr[1]}")
            print(f"    STR={tr[2]} STA={tr[3]} AGI={tr[4]} DEX={tr[5]} INT={tr[6]} WIS={tr[7]} CHA={tr[8]}")
            print(f"    SlotTypes=[{tr[9]},{tr[10]},{tr[11]},{tr[12]},{tr[13]},{tr[14]}]")
            print(f"    SlotVis  =[{tr[15]},{tr[16]},{tr[17]},{tr[18]},{tr[19]},{tr[20]}]")

conn.close()
