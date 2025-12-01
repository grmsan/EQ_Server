
import mysql.connector
import json

def get_db_config():
    with open('eqemu_config.json', 'r') as f:
        config = json.load(f)
    return config['server']['database']

def check_launcher_config():
    db_config = get_db_config()
    conn = mysql.connector.connect(
        host=db_config['host'],
        port=int(db_config['port']),
        user=db_config['username'],
        password=db_config['password'],
        database=db_config['db']
    )
    cursor = conn.cursor(dictionary=True)

    print("--- Launcher Configuration ---")
    cursor.execute("SELECT * FROM launcher WHERE name = 'zone'")
    print(cursor.fetchall())

    print("\n--- Launcher Zones (tutorialb) ---")
    cursor.execute("SELECT * FROM launcher_zones WHERE zone = 'tutorialb'")
    print(cursor.fetchall())

    conn.close()

if __name__ == "__main__":
    check_launcher_config()
