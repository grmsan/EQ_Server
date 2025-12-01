import mysql.connector
import json
import os

def check_db_content():
    config_path = "eqemu_config.json"
    if not os.path.exists(config_path):
        print("Config file not found.")
        return

    with open(config_path, "r") as f:
        config = json.load(f)

    db_config = config["server"]["database"]
    host = db_config["host"]
    port = int(db_config["port"])
    user = db_config["username"]
    password = db_config["password"]
    db_name = db_config["db"]

    try:
        conn = mysql.connector.connect(
            host=host,
            port=port,
            user=user,
            password=password,
            database=db_name
        )
        cursor = conn.cursor()

        tables_to_check = ["items", "spells_new", "zone", "npc_types"]
        for table in tables_to_check:
            try:
                cursor.execute(f"SELECT COUNT(*) FROM {table}")
                count = cursor.fetchone()[0]
                print(f"Table '{table}': {count} rows")
            except mysql.connector.Error as err:
                print(f"Error checking table '{table}': {err}")

        conn.close()

    except mysql.connector.Error as err:
        print(f"Database connection failed: {err}")

if __name__ == "__main__":
    check_db_content()
