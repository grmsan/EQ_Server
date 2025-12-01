import mysql.connector

def get_max_ids():
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

            tables = ['items', 'npc_types', 'spawn2', 'spawngroup', 'loottable', 'lootdrop']
            for table in tables:
                col = 'id'
                if table == 'spawn_group': col = 'id' # actually it is id

                cursor.execute(f"SELECT MAX({col}) FROM {table}")
                max_id = cursor.fetchone()[0]
                print(f"Max ID for {table}: {max_id}")

    except Exception as e:
        print(f"Error: {e}")
    finally:
        if connection.is_connected():
            cursor.close()
            connection.close()

if __name__ == "__main__":
    get_max_ids()
