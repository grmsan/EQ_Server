
import mysql.connector
import json

def get_db_config():
    with open('eqemu_config.json', 'r') as f:
        config = json.load(f)
    return config['server']['database']

def fix_launcher_zones():
    db_config = get_db_config()
    conn = mysql.connector.connect(
        host=db_config['host'],
        port=int(db_config['port']),
        user=db_config['username'],
        password=db_config['password'],
        database=db_config['db']
    )
    cursor = conn.cursor()

    print("Removing 'peq' launcher entry for bazaar...")
    cursor.execute("DELETE FROM launcher_zones WHERE zone = 'bazaar' AND launcher = 'peq'")
    conn.commit()
    print(f"Deleted {cursor.rowcount} rows.")

    conn.close()

if __name__ == "__main__":
    fix_launcher_zones()
