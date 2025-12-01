import mysql.connector

def run_sql_file(filename):
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

            with open(filename, 'r') as f:
                sql_file = f.read()

            commands = sql_file.split(';')
            for command in commands:
                if command.strip():
                    try:
                        cursor.execute(command)
                        print(f"Executed: {command[:50]}...")
                    except mysql.connector.Error as err:
                        print(f"Error executing command: {err}")
                        print(f"Command: {command}")

            connection.commit()
            print("SQL script executed successfully.")

    except Exception as e:
        print(f"Error: {e}")
    finally:
        if connection.is_connected():
            cursor.close()
            connection.close()

if __name__ == "__main__":
    run_sql_file('create_quest.sql')
