"""
List all rules currently overridden in the database
This shows what will be lost if you delete rule_values
"""
import mysql.connector

conn = mysql.connector.connect(
    host='127.0.0.1',
    user='root',
    password='110987',
    database='peq',
    port=3308
)

cursor = conn.cursor()
cursor.execute("SELECT rule_name, rule_value FROM rule_values ORDER BY rule_name")

print("Current Database Rule Overrides:")
print("=" * 60)
count = 0
for rule_name, rule_value in cursor:
    print(f"{rule_name} = {rule_value}")
    count += 1

print("=" * 60)
print(f"Total overrides: {count}")
print("\nIf you delete rule_values, all of these will use ruletypes.h defaults.")

cursor.close()
conn.close()
