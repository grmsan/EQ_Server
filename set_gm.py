import mysql.connector
import json
import os
import sys

def set_gm_status(char_name, status_level=255):
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

        # Find the account ID for the character
        # We need to join character_data and account tables usually, or just update account based on char name
        # But status is on the account table.

        print(f"Looking for character '{char_name}'...")
        cursor.execute("SELECT id, account_id, name FROM character_data WHERE name = %s", (char_name,))
        char_row = cursor.fetchone()

        if not char_row:
            print(f"Character '{char_name}' not found.")
            conn.close()
            return

        char_id, account_id, name = char_row
        print(f"Found character: {name} (ID: {char_id}, Account ID: {account_id})")

        # Update account status
        print(f"Setting status to {status_level} for account ID {account_id}...")
        cursor.execute("UPDATE account SET status = %s WHERE id = %s", (status_level, account_id))
        conn.commit()

        if cursor.rowcount > 0:
            print("Success! GM status updated.")
            print("You may need to relog for changes to take effect.")
        else:
            print("Failed to update account status (Account might not exist?).")

        conn.close()

    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python set_gm.py <character_name> [status_level]")
    else:
        name = sys.argv[1]
        level = int(sys.argv[2]) if len(sys.argv) > 2 else 255
        set_gm_status(name, level)
