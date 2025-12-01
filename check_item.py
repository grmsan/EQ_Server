import mysql.connector

def check_item():
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

            item_id = 200000
            cursor.execute(f"SELECT * FROM items WHERE id = {item_id}")
            item = cursor.fetchone()

            if item:
                print(f"Item found: {item['Name']} (ID: {item['id']})")
                print(f"Lore: {item['lore']}")
            else:
                print(f"Item with ID {item_id} not found.")

    except Exception as e:
        print(f"Error: {e}")
    finally:
        if connection.is_connected():
            cursor.close()
            connection.close()

if __name__ == "__main__":
    check_item()
