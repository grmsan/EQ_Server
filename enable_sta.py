import mysql.connector

# Connect to database
conn = mysql.connector.connect(
    host='127.0.0.1',
    user='root',
    password='110987',
    database='peq',
    port=3308
)

cursor = conn.cursor()

# Check if rule exists
cursor.execute("SELECT COUNT(*) FROM rule_values WHERE rule_name='Combat:UseNewStaminaFormula'")
count = cursor.fetchone()[0]

if count == 0:
    # Insert new rule
    cursor.execute("INSERT INTO rule_values (ruleset_id, rule_name, rule_value, notes) VALUES (1, 'Combat:UseNewStaminaFormula', 'true', 'Enable new STA-based sustainability system')")
    conn.commit()
    print(f"STA Formula rule inserted. Rows affected: {cursor.rowcount}")
else:
    # Update existing rule
    cursor.execute("UPDATE rule_values SET rule_value='true' WHERE rule_name='Combat:UseNewStaminaFormula'")
    conn.commit()
    print(f"STA Formula enabled. Rows affected: {cursor.rowcount}")

# Verify
cursor.execute("SELECT rule_name, rule_value FROM rule_values WHERE rule_name='Combat:UseNewStaminaFormula'")
result = cursor.fetchone()
if result:
    print(f"Current setting: {result[0]} = {result[1]}")
else:
    print("Rule still not found - check database connection")

cursor.close()
conn.close()
