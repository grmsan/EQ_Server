
import mysql.connector
import json

def get_db_config():
    with open('eqemu_config.json', 'r') as f:
        config = json.load(f)
    return config['server']['database']

def check_bazaar():
    db_config = get_db_config()
    conn = mysql.connector.connect(
        host=db_config['host'],
        port=int(db_config['port']),
        user=db_config['username'],
        password=db_config['password'],
        database=db_config['db']
    )
    cursor = conn.cursor(dictionary=True)

    print("--- launcher_zones ---")
    cursor.execute("SELECT * FROM launcher_zones WHERE zone = 'bazaar'")
    print(cursor.fetchall())

    print("\n--- zone table ---")
    cursor.execute("SELECT short_name, zoneidnumber, version, insttype, type FROM zone WHERE short_name = 'bazaar'")
    print(cursor.fetchall())

    conn.close()

if __name__ == "__main__":
    check_bazaar()
