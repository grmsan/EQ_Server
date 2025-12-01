
import mysql.connector
import json

def get_db_config():
    with open('eqemu_config.json', 'r') as f:
        config = json.load(f)
    return config['server']['database']

def delete_bad_spawns():
    db_config = get_db_config()
    conn = mysql.connector.connect(
        host=db_config['host'],
        port=int(db_config['port']),
        user=db_config['username'],
        password=db_config['password'],
        database=db_config['db']
    )
    cursor = conn.cursor()

    # IDs of spawn2 entries that have NPC ID 0 (from previous output)
    bad_ids = [58848, 58849, 58850, 58851, 58852, 58853, 58854]

    if bad_ids:
        format_strings = ','.join(['%s'] * len(bad_ids))
        print(f"Deleting bad spawn2 entries: {bad_ids}")
        cursor.execute(f"DELETE FROM spawn2 WHERE id IN ({format_strings})", tuple(bad_ids))
        print(f"Deleted {cursor.rowcount} rows.")
        conn.commit()

    conn.close()

if __name__ == "__main__":
    delete_bad_spawns()
