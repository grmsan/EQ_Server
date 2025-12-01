import mysql.connector
import json
import os

def fix_launcher():
    config_path = "eqemu_config.json"
    if not os.path.exists(config_path):
        print("eqemu_config.json not found.")
        return

    with open(config_path, "r") as f:
        config = json.load(f)

    db_config = config["server"]["database"]

    try:
        conn = mysql.connector.connect(
            host=db_config["host"],
            port=db_config["port"],
            user=db_config["username"],
            password=db_config["password"],
            database=db_config["db"]
        )
        cursor = conn.cursor()

        # 1. Add bazaar to zone launcher as static
        print("Adding bazaar to zone launcher...")
        cursor.execute("INSERT IGNORE INTO launcher_zones (launcher, zone, port) VALUES ('zone', 'bazaar', 0)")

        # 2. Increase dynamic zones for zone launcher
        print("Increasing dynamic zones for zone launcher...")
        cursor.execute("UPDATE launcher SET dynamics = 10 WHERE name = 'zone'")

        conn.commit()
        print("Database updated.")
        conn.close()

    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    fix_launcher()
