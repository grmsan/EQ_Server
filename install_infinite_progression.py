#!/usr/bin/env python3
"""
Install Infinite Progression System Database Schema
Executes the SQL schema file for the infinite progression system.
"""

import json
import sys
from pathlib import Path

try:
    import pymysql
except ImportError:
    print("❌ Error: pymysql module not found")
    print("Installing pymysql...")
    import subprocess
    subprocess.check_call([sys.executable, "-m", "pip", "install", "pymysql"])
    import pymysql

def load_database_config():
    """Load database credentials from eqemu_config.json"""
    config_path = Path(__file__).parent / "eqemu_config.json"

    if not config_path.exists():
        print(f"Error: {config_path} not found")
        sys.exit(1)

    with open(config_path, 'r') as f:
        config = json.load(f)

    db_config = config.get('server', {}).get('database', {})

    return {
        'host': db_config.get('host', 'localhost'),
        'port': db_config.get('port', 3306),
        'username': db_config.get('username', 'root'),
        'password': db_config.get('password', ''),
        'db': db_config.get('db', 'peq')
    }

def execute_sql_file(sql_file: Path, db_config: dict):
    """Execute SQL file using pymysql"""

    if not sql_file.exists():
        print(f"Error: SQL file not found: {sql_file}")
        sys.exit(1)

    print(f"Installing Infinite Progression Database Schema...")
    print(f"Database: {db_config['db']} on {db_config['host']}:{db_config['port']}")
    print(f"SQL File: {sql_file}")
    print()

    try:
        # Connect to database
        connection = pymysql.connect(
            host=db_config['host'],
            port=db_config['port'],
            user=db_config['username'],
            password=db_config['password'],
            database=db_config['db'],
            autocommit=False
        )

        cursor = connection.cursor()

        # Read and execute SQL file
        with open(sql_file, 'r', encoding='utf-8') as f:
            sql_content = f.read()

        # Split by semicolons and execute each statement
        statements = [s.strip() for s in sql_content.split(';') if s.strip()]

        print(f"Executing {len(statements)} SQL statements...")

        for i, statement in enumerate(statements, 1):
            if statement:
                try:
                    cursor.execute(statement)
                    print(f"  [{i}/{len(statements)}] ✓", end='\r')
                except pymysql.Error as e:
                    print(f"\n⚠️  Warning on statement {i}: {e}")

        connection.commit()
        print(f"\n✅ Schema installed successfully!")
        print()

        # Verify installation by checking tables
        cursor.execute("SHOW TABLES LIKE 'dynamic_items%'")
        tables = cursor.fetchall()

        if tables:
            print("Installed tables:")
            for table in tables:
                cursor.execute(f"SELECT COUNT(*) FROM {table[0]}")
                count = cursor.fetchone()[0]
                print(f"  - {table[0]} ({count} rows)")
            print()

        # Show initial config
        try:
            cursor.execute("SELECT * FROM progression_config")
            configs = cursor.fetchall()

            if configs:
                print("Default Configuration:")
                cursor.execute("SHOW COLUMNS FROM progression_config")
                columns = [col[0] for col in cursor.fetchall()]

                for config in configs:
                    print(f"  {config[0]}: {config[1]} = {config[2]}")
                print()
        except pymysql.Error:
            pass

        cursor.close()
        connection.close()

        print("🎉 Infinite Progression System is ready!")
        print("\nNext steps:")
        print("1. Boot the server (shared_memory, world, zone)")
        print("2. Log in and test: #createscaled 1001 50")
        print("3. Check logs/inf/ for detailed event logs")

    except pymysql.Error as e:
        print(f"❌ Database error: {e}")
        sys.exit(1)
    except Exception as e:
        print(f"❌ Error: {e}")
        sys.exit(1)

def main():
    script_dir = Path(__file__).parent
    sql_file = script_dir / "utils" / "sql" / "infinite_progression_schema.sql"

    print("=" * 60)
    print("Infinite Progression System - Database Installer")
    print("=" * 60)
    print()

    # Load database config
    db_config = load_database_config()

    # Execute SQL
    execute_sql_file(sql_file, db_config)

if __name__ == "__main__":
    main()
