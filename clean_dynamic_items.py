import pymysql.cursors

# Connect
conn = pymysql.connect(
    host='127.0.0.1',
    port=3308,
    user='eqemu',
    password='eqemu',
    database='peq',
    auth_plugin='mysql_native_password'
)

cursor = conn.cursor()

# Clean up bad custom_data
cursor.execute("UPDATE inventory SET custom_data = '' WHERE custom_data LIKE 'dynamic_level^%'")
print(f"Cleared {cursor.rowcount} bad custom_data entries")

# Delete dynamic items from items table
cursor.execute("DELETE FROM items WHERE id >= 1000000000")
print(f"Deleted {cursor.rowcount} dynamic item rows")

conn.commit()
conn.close()

print("Database cleaned successfully!")
