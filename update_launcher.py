
import mysql.connector
from mysql.connector import Error

def update_launcher_name():
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
            # The new launcher name (relative path to executable)
            new_name = r'build\bin\RelWithDebInfo\zone'

            # 1. Ensure the launcher entry exists
            cursor.execute("SELECT name FROM launcher WHERE name=%s", (new_name,))
            if not cursor.fetchone():
                print(f"Creating launcher entry for '{new_name}'")
                cursor.execute("INSERT INTO launcher (name, dynamics) VALUES (%s, 0)", (new_name,))

            # 2. Update ALL zones in launcher_zones to use this launcher
            print("Updating launcher_zones...")
            cursor.execute("UPDATE launcher_zones SET launcher=%s WHERE launcher IN ('zone', 'peq')", (new_name,))

            connection.commit()
            print(f"Updated launcher_zones to use '{new_name}'")

    except Error as e:
        print(f"Error: {e}")
    finally:
        if connection.is_connected():
            cursor.close()
            connection.close()

if __name__ == "__main__":
    update_launcher_name()
