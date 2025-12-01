
import mysql.connector

def check_zone():
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
            cursor.execute("SELECT launcher, port FROM launcher_zones WHERE zone='tutorialb'")
            print(f"tutorialb assignment: {cursor.fetchall()}")

            cursor.execute("SELECT name FROM launcher")
            print(f"Launchers: {cursor.fetchall()}")

    except Exception as e:
        print(f"Error: {e}")
    finally:
        if connection.is_connected():
            cursor.close()
            connection.close()

if __name__ == "__main__":
    check_zone()
