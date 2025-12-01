import mysql.connector
import json
import os

def check_launcher():
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

        tables_to_check = ["launcher", "launcher_zones", "variables"]

        for table in tables_to_check:
            try:
                print(f"\n--- {table} ---")
                cursor.execute(f"SELECT * FROM {table}")
                rows = cursor.fetchall()
                if not rows:
                    print("Empty")
                else:
                    # Get column names
                    column_names = [i[0] for i in cursor.description]
                    print(f"Columns: {column_names}")
                    for row in rows:
                        print(row)
            except Exception as e:
                print(f"Error reading {table}: {e}")

        conn.close()

    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    check_launcher()
