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
            cursor = connection.cursor(dictionary=True)
            cursor.execute("SELECT short_name, min_status, file_name FROM zone WHERE short_name IN ('bazaar', 'poknowledge')")
            rows = cursor.fetchall()
            print("Zone Info:")
            for row in rows:
                print(row)

    except Exception as e:
        print(f"Error: {e}")
    finally:
        if connection.is_connected():
            cursor.close()
            connection.close()

if __name__ == "__main__":
    check_zone()
