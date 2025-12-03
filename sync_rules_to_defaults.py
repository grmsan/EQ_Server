"""
Sync specific rules in the database to match code defaults from ruletypes.h
This will DELETE rules from the database so they use code defaults.
"""
import mysql.connector

# Database connection from eqemu_config.json
conn = mysql.connector.connect(
    host='127.0.0.1',
    user='root',
    password='110987',
    database='peq',
    port=3308
)

cursor = conn.cursor()

# Rules to reset to code defaults (will DELETE from database)
rules_to_reset = [
    'Combat:UseNewStaminaFormula',
    'Combat:UseNewStrDamageFormula',
    # Add more rules here as needed
]

print("Resetting rules to code defaults...")
for rule_name in rules_to_reset:
    cursor.execute("DELETE FROM rule_values WHERE rule_name = %s", (rule_name,))
    print(f"  Deleted: {rule_name} (will now use ruletypes.h default)")

conn.commit()
print(f"\n{cursor.rowcount} rule(s) reset to code defaults.")
print("Run '/reload rules' in-game or restart the server to apply changes.")

cursor.close()
conn.close()
