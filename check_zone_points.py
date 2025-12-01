
import mysql.connector
import json

def get_db_config():
    with open('eqemu_config.json', 'r') as f:
        config = json.load(f)
    return config['server']['database']

def check_zone_points():
    db_config = get_db_config()
    conn = mysql.connector.connect(
        host=db_config['host'],
        port=int(db_config['port']),
        user=db_config['username'],
        password=db_config['password'],
        database=db_config['db']
    )
    cursor = conn.cursor(dictionary=True)

    print("--- Zone Points from TutorialB (189) ---")
    cursor.execute("SELECT * FROM zone_points WHERE zone = 'tutorialb'")
    points = cursor.fetchall()
    for p in points:
        print(p)

    print("\n--- Doors in TutorialB (189) leading to Bazaar (151) ---")
    cursor.execute("SELECT * FROM doors WHERE zone = 'tutorialb' AND dest_zone = 'bazaar'")
    doors = cursor.fetchall()
    for d in doors:
        print(d)

    conn.close()

if __name__ == "__main__":
    check_zone_points()
