
import mysql.connector
import json

def get_db_config():
    config_path = 'eqemu_config.json'
    with open(config_path, 'r') as f:
        config = json.load(f)

    db_info = config['server']['database']
    return {
        'host': db_info['host'],
        'port': int(db_info['port']),
        'user': db_info['username'],
        'password': db_info['password'],
        'database': db_info['db']
    }

def check_launcher():
    config = get_db_config()
    conn = mysql.connector.connect(**config)
    cursor = conn.cursor(dictionary=True)

    print("--- Launcher Table ---")
    cursor.execute("SELECT * FROM launcher")
    launchers = cursor.fetchall()
    for l in launchers:
        print(f"Name: {repr(l['name'])}, Dynamics: {l['dynamics']}")

    print("\n--- Launcher Zones Table (tutorialb) ---")
    cursor.execute("SELECT * FROM launcher_zones WHERE zone = 'tutorialb'")
    zones = cursor.fetchall()
    for z in zones:
        print(f"Launcher: {repr(z['launcher'])}, Zone: {z['zone']}, Port: {z['port']}")

    print("\n--- Zone Table (tutorialb) ---")
    cursor.execute("SELECT short_name, zoneidnumber, version, min_status FROM zone WHERE short_name = 'tutorialb'")
    z_info = cursor.fetchall()
    for z in z_info:
        print(z)

    cursor.close()
    conn.close()

if __name__ == "__main__":
    check_launcher()
