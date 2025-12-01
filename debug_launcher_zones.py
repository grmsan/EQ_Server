
import mysql.connector

def check_launcher_zones():
    try:
        db = mysql.connector.connect(
            host="127.0.0.1",
            user="root",
            password="password",
            database="peq",
            port=3308
        )
        cursor = db.cursor()

        print("--- Launcher Table ---")
        cursor.execute("SELECT * FROM launcher")
        for row in cursor.fetchall():
            print(row)

        print("\n--- Launcher Zones Table (tutorialb) ---")
        cursor.execute("SELECT * FROM launcher_zones WHERE zone = 'tutorialb'")
        for row in cursor.fetchall():
            print(row)

        print("\n--- Launcher Zones Table (All) ---")
        cursor.execute("SELECT * FROM launcher_zones LIMIT 20")
        for row in cursor.fetchall():
            print(row)

    except mysql.connector.Error as err:
        print(f"Error: {err}")
    finally:
        if 'db' in locals() and db.is_connected():
            db.close()

if __name__ == "__main__":
    check_launcher_zones()
