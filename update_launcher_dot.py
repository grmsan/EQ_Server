
import mysql.connector
from mysql.connector import Error

def update_launcher_name_dot():
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
            # Current name
            old_name = r'build\bin\RelWithDebInfo\zone.exe'
            # New name with dot
            new_name = r'build\bin\RelWithDebInfo\zone.exe.'

            # Check if new name already exists
            cursor.execute("SELECT name FROM launcher WHERE name=%s", (new_name,))
            if cursor.fetchone():
                print(f"Launcher '{new_name}' already exists. Updating zones...")
                cursor.execute("UPDATE launcher_zones SET launcher=%s WHERE launcher=%s", (new_name, old_name))
                cursor.execute("DELETE FROM launcher WHERE name=%s", (old_name,))
            else:
                print(f"Renaming launcher '{old_name}' to '{new_name}'")
                cursor.execute("UPDATE launcher SET name=%s WHERE name=%s", (new_name, old_name))
                cursor.execute("UPDATE launcher_zones SET launcher=%s WHERE launcher=%s", (new_name, old_name))

            connection.commit()
            print(f"Updated launcher configuration to '{new_name}'")

    except Error as e:
        print(f"Error: {e}")
    finally:
        if connection.is_connected():
            cursor.close()
            connection.close()

if __name__ == "__main__":
    update_launcher_name_dot()
