import mysql.connector
import json
import os

def check_accounts():
    config_path = "login.json"
    if not os.path.exists(config_path):
        print("login.json not found.")
        return

    with open(config_path, "r") as f:
        config = json.load(f)

    db_config = config["database"]

    try:
        conn = mysql.connector.connect(
            host=db_config["host"],
            port=db_config["port"],
            user=db_config["user"],
            password=db_config["password"],
            database=db_config["db"]
        )
        cursor = conn.cursor()

        # Check for account tables
        cursor.execute("SHOW TABLES LIKE '%account%'")
        tables = cursor.fetchall()
        print("Account related tables:", tables)

        # Try to find the login account table
        # Usually tblLoginServerAccounts or account

        possible_tables = ["tblLoginServerAccounts", "account", "login_accounts"]
        for table in possible_tables:
            try:
                cursor.execute(f"SELECT * FROM {table} LIMIT 5")
                rows = cursor.fetchall()

                # Get column names
                column_names = [i[0] for i in cursor.description]
                print(f"\nColumns of {table}: {column_names}")

                print(f"\nContent of {table}:")
                for row in rows:
                    print(row)
            except Exception as e:
                print(f"Could not read {table}: {e}")

        conn.close()

    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    check_accounts()
