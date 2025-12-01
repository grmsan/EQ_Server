import mysql.connector

def check_columns():
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

            cursor.execute("DESCRIBE items")
            print("Items columns:")
            for col in cursor.fetchall():
                print(col[0])

            cursor.execute("DESCRIBE npc_types")
            print("\nNPC Types columns:")
            for col in cursor.fetchall():
                print(col[0])

    except Exception as e:
        print(f"Error: {e}")
    finally:
        if connection.is_connected():
            cursor.close()
            connection.close()

if __name__ == "__main__":
    check_columns()
