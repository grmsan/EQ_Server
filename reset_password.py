import mysql.connector
import json
import os
import hashlib

def reset_password(username, new_password):
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

        # Hash the password with MD5
        hashed_password = hashlib.md5(new_password.encode()).hexdigest()
        print(f"Resetting password for '{username}' to '{new_password}' (MD5: {hashed_password})")

        # Update login_accounts
        cursor.execute("UPDATE login_accounts SET account_password = %s WHERE account_name = %s", (hashed_password, username))

        if cursor.rowcount > 0:
            print(f"Successfully updated password for '{username}' in login_accounts.")
            conn.commit()
        else:
            print(f"User '{username}' not found in login_accounts.")

        conn.close()

    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    reset_password("Rokze123", "password")
