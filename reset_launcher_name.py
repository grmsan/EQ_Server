
import mysql.connector

def reset_launcher():
    try:
        connection = mysql.connector.connect(
            host='localhost',
            database='peq',
            user='root',
            password='110987',
            port=3308
        )

        if connection.is_connected():
            cursor = connection.cursor()

            # 1. Check counts
            cursor.execute("SELECT launcher, COUNT(*) FROM launcher_zones GROUP BY launcher")
            print(f"Current distribution: {cursor.fetchall()}")

            # 2. Delete 'peq' assignments (assuming they are old/duplicates)
            cursor.execute("DELETE FROM launcher_zones WHERE launcher = 'peq'")
            print("Deleted 'peq' assignments.")

            # 3. Delete 'default' assignments if any
            cursor.execute("DELETE FROM launcher_zones WHERE launcher = 'default'")

            # 4. Repopulate launcher_zones from zone table
            # First, clear the table completely to remove any stale entries (like 'zone.', 'default', etc.)
            cursor.execute("TRUNCATE TABLE launcher_zones")
            print("Truncated launcher_zones table.")

            # Insert tutorialb, bazaar, and poknowledge as static zones.
            cursor.execute("""
                INSERT INTO launcher_zones (launcher, zone, port)
                VALUES
                ('node_1', 'tutorialb', 0),
                ('node_1', 'bazaar', 0),
                ('node_1', 'poknowledge', 0)
            """)
            print(f"Added 'tutorialb', 'bazaar', 'poknowledge' to launcher_zones.")
            print(f"Repopulated launcher_zones from zone table to 'node_1'.")

            # 5. Fix launcher table
            cursor.execute("DELETE FROM launcher")
            cursor.execute("INSERT INTO launcher (name, dynamics) VALUES ('node_1', 10)")

            connection.commit()
            print("Launcher reset to 'node_1' and zones updated.")

    except Exception as e:
        print(f"Error: {e}")
    finally:
        if connection.is_connected():
            cursor.close()
            connection.close()

if __name__ == "__main__":
    reset_launcher()
