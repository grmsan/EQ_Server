
import mysql.connector
import json

def get_db_config():
    with open('eqemu_config.json', 'r') as f:
        config = json.load(f)
    return config['server']['database']

def check_bazaar_spawns():
    db_config = get_db_config()
    conn = mysql.connector.connect(
        host=db_config['host'],
        port=int(db_config['port']),
        user=db_config['username'],
        password=db_config['password'],
        database=db_config['db']
    )
    cursor = conn.cursor(dictionary=True)

    print("--- Spawns in Bazaar (151) ---")
    # Get spawn2 entries for zone 'bazaar'
    cursor.execute("SELECT * FROM spawn2 WHERE zone = 'bazaar'")
    spawns = cursor.fetchall()
    print(f"Found {len(spawns)} spawn2 entries.")

    for spawn in spawns:
        sg_id = spawn['spawngroupID']
        print(f"Spawn2 ID: {spawn['id']}, Spawngroup: {sg_id}")

        # Get spawn entries for this group
        cursor.execute(f"SELECT * FROM spawnentry WHERE spawngroupID = {sg_id}")
        entries = cursor.fetchall()
        for entry in entries:
            npc_id = entry['npcID']
            print(f"  -> NPC ID: {npc_id}")

            # Check if NPC exists
            cursor.execute(f"SELECT name, id FROM npc_types WHERE id = {npc_id}")
            npc = cursor.fetchone()
            if npc:
                print(f"     Name: {npc['name']}")
            else:
                print(f"     [ERROR] NPC ID {npc_id} not found in npc_types!")

    conn.close()

if __name__ == "__main__":
    check_bazaar_spawns()
