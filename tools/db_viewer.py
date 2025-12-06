#!/usr/bin/env python3
"""
EQEmu DB Viewer Tool

Usage:
  - List tables
    python tools/db_viewer.py --list-tables

  - Describe table
    python tools/db_viewer.py --describe <TABLE>

  - Show CREATE TABLE
    python tools/db_viewer.py --create-table <TABLE>

  - Show first N rows
    python tools/db_viewer.py --rows <TABLE> --limit 20

  - Count rows
    python tools/db_viewer.py --count <TABLE>

  - Run a query
    python tools/db_viewer.py --query "SELECT * FROM items LIMIT 10"

  - Run a SQL script (multiple statements)
    python tools/db_viewer.py --script utils/sql/custom/cleanup_dynamic_items.sql

  - Dry-run a script (print statements, do not execute)
    python tools/db_viewer.py --script utils/sql/custom/cleanup_dynamic_items.sql --dry-run

  - Run interactive shell
    python tools/db_viewer.py --shell

This tool reads `eqemu_config.json` in the repo root for DB connection settings
if available, otherwise uses CLI arguments.

Dependencies:
    pip install mysql-connector-python

"""

import argparse
import csv
import inspect
import json
import os
import sys
import textwrap

# If mysql.connector not installed, we'll inform the user
try:
    import mysql.connector as mysql
except Exception as e:
    mysql = None


def pretty_print_table(columns, rows, truncate=30, header=True):
    if not rows:
        if header:
            print("No rows found")
        return
    widths = [len(str(c)) for c in columns]
    for r in rows:
        for i, v in enumerate(r):
            widths[i] = max(widths[i], len(str(v)))
    widths = [min(w, truncate) for w in widths]

    def fmt(val, i):
        s = str(val) if val is not None else "NULL"
        if len(s) > truncate:
            s = s[:truncate-3] + '...'
        return s.ljust(widths[i])

    if header:
        print(' | '.join(fmt(c, i) for i, c in enumerate(columns)))
        print('-+-'.join('-' * widths[i] for i in range(len(widths))))
    for r in rows:
        print(' | '.join(fmt(v, i) for i, v in enumerate(r)))


class DBViewer:
    def __init__(self, host=None, port=None, user=None, password=None, db=None):
        self.host = host
        self.port = port or 3308
        self.user = user
        self.password = password
        self.db = db
        self.conn = None

    def _load_config(self):
        config_path = os.path.join(os.getcwd(), 'eqemu_config.json')
        if os.path.exists(config_path):
            try:
                with open(config_path, 'r') as f:
                    config = json.load(f)
                dbcfg = config.get('server', {}).get('database', {})
                return {
                    'host': dbcfg.get('host', '127.0.0.1'),
                    'port': dbcfg.get('port', 3306),
                    'user': dbcfg.get('username', 'root'),
                    'password': dbcfg.get('password', ''),
                    'db': dbcfg.get('db', '')
                }
            except Exception:
                pass
        return None

    def connect(self):
        if not mysql:
            raise RuntimeError('Missing mysql-connector-python. Install with: pip install mysql-connector-python')

        if not (self.host and self.user and self.db):
            cfg = self._load_config()
            if cfg:
                self.host = self.host or cfg['host']
                self.port = self.port or cfg['port']
                self.user = self.user or cfg['user']
                self.password = self.password or cfg['password']
                self.db = self.db or cfg['db']

        if not (self.host and self.user and self.db):
            raise RuntimeError('Missing DB configuration; specify via eqemu_config.json or CLI args')

        self.conn = mysql.connect(
            host=self.host,
            port=int(self.port),
            user=self.user,
            password=self.password,
            database=self.db,
            autocommit=True
        )

    def close(self):
        if self.conn:
            self.conn.close()
            self.conn = None

    def list_tables(self):
        cur = self.conn.cursor()
        cur.execute("SHOW TABLES")
        rows = cur.fetchall()
        cur.close()
        print('\n'.join(r[0] for r in rows))

    def list_databases(self):
        cur = self.conn.cursor()
        cur.execute("SHOW DATABASES")
        rows = cur.fetchall()
        cur.close()
        print('\n'.join(r[0] for r in rows))

    def describe_table(self, table):
        cur = self.conn.cursor()
        cur.execute(f"DESCRIBE `{table}`")
        rows = cur.fetchall()
        cur.close()
        pretty_print_table(['Field', 'Type', 'Null', 'Key', 'Default', 'Extra'], rows)

    def create_table(self, table):
        cur = self.conn.cursor()
        cur.execute(f"SHOW CREATE TABLE `{table}`")
        rows = cur.fetchall()
        cur.close()
        if rows:
            print(rows[0][1])

    def rows(self, table, limit=20):
        cur = self.conn.cursor()
        cur.execute(f"SELECT * FROM `{table}` LIMIT {int(limit)}")
        rows = cur.fetchall()
        cols = [d[0] for d in cur.description]
        cur.close()
        pretty_print_table(cols, rows)

    def count(self, table):
        cur = self.conn.cursor()
        cur.execute(f"SELECT COUNT(*) FROM `{table}`")
        rows = cur.fetchall()
        cur.close()
        if rows:
            print(rows[0][0])

    def sample(self, table, limit=5):
        cur = self.conn.cursor()
        cur.execute(f"SELECT * FROM `{table}` ORDER BY RAND() LIMIT {int(limit)}")
        rows = cur.fetchall()
        cols = [d[0] for d in cur.description]
        cur.close()
        pretty_print_table(cols, rows)

    def query(self, sql, limit=None, csvout=None):
        cur = self.conn.cursor()
        cur.execute(sql)
        rows = cur.fetchall()
        if cur.description:
            cols = [d[0] for d in cur.description]
            if limit:
                rows = rows[:int(limit)]
            pretty_print_table(cols, rows)
            if csvout:
                with open(csvout, 'w', newline='', encoding='utf-8') as f:
                    writer = csv.writer(f)
                    writer.writerow(cols)
                    for r in rows:
                        writer.writerow(r)
                print(f'Wrote {len(rows)} row(s) to {csvout}')
        else:
            print("Query executed (no results)" )
        cur.close()

    def _strip_comments_and_split(self, sql_text):
        """
        Remove line/block comments and split into individual statements on semicolons.
        Not a full SQL parser; avoid using with custom DELIMITER blocks.
        """
        lines = []
        in_block = False
        for raw in sql_text.splitlines():
            line = raw
            stripped = line.strip()
            if in_block:
                if '*/' in stripped:
                    in_block = False
                continue
            if stripped.startswith('/*'):
                if '*/' not in stripped:
                    in_block = True
                continue
            if stripped.startswith('--') or stripped.startswith('#') or stripped == '':
                continue
            # crude inline comment removal
            for token in ('--', '#'):
                if token in line:
                    line = line.split(token, 1)[0]
            if line.strip():
                lines.append(line)
        cleaned = '\n'.join(lines)
        return [s.strip() for s in cleaned.split(';') if s.strip()]

    def run_script(self, path, stop_on_error=False, dry_run=False):
        if not os.path.exists(path):
            raise FileNotFoundError(f"Script not found: {path}")
        with open(path, 'r', encoding='utf-8') as f:
            sql_text = f.read()
        if not sql_text.strip():
            print(f"{path}: empty script; nothing to run")
            return

        statements = self._strip_comments_and_split(sql_text)

        if dry_run:
            print(f"[DRY RUN] Would execute statements from: {path}")
            if not statements:
                print("(No statements detected)")
                return
            for i, stmt in enumerate(statements, 1):
                print(f"[{i}] {stmt};")
            print(f"Total statements: {len(statements)}")
            return

        cur = self.conn.cursor()
        print(f"Running script: {path}")
        idx = 0
        supports_multi = 'multi' in inspect.signature(cur.execute).parameters
        try:
            if supports_multi:
                for idx, stmt in enumerate(cur.execute(sql_text, multi=True), start=1):
                    if stmt.with_rows:
                        rows = stmt.fetchall()
                        cols = [d[0] for d in stmt.description]
                        print(f"[{idx}] Result set ({len(rows)} rows)")
                        pretty_print_table(cols, rows)
                    else:
                        print(f"[{idx}] OK ({stmt.rowcount} rows affected)")
            else:
                # Fallback: run pre-split statements (comments stripped)
                for idx, statement in enumerate(statements, start=1):
                    cur.execute(statement)
                    if cur.with_rows:
                        rows = cur.fetchall()
                        cols = [d[0] for d in cur.description]
                        print(f"[{idx}] Result set ({len(rows)} rows)")
                        pretty_print_table(cols, rows)
                    else:
                        print(f"[{idx}] OK ({cur.rowcount} rows affected)")
        except Exception as e:
            print(f"Error in script at statement {idx}: {e}")
            if stop_on_error:
                cur.close()
                raise
        cur.close()

    def shell(self):
        print('DB Viewer Shell; enter SQL or `help` or `.exit`')
        while True:
            try:
                s = input('sql> ').strip()
            except (EOFError, KeyboardInterrupt):
                print('\nExiting')
                break
            if not s:
                continue
            if s in ('.exit', 'exit', 'quit'):
                break
            if s == 'help':
                print('Commands:\n  list-tables, list-dbs, desc <table>, create <table>, rows <table> <limit>, count <table>, sample <table> <limit>\n  sql <SQL>\n  run <path.sql>  (execute script)\n  run-dry <path.sql>  (print statements, do not execute)')
                continue
            if s.startswith('list-dbs'):
                self.list_databases(); continue
            if s.startswith('list-tables'):
                self.list_tables(); continue
            if s.startswith('desc '):
                parts = s.split(None, 1); self.describe_table(parts[1]); continue
            if s.startswith('create '):
                parts = s.split(None, 1); self.create_table(parts[1]); continue
            if s.startswith('rows '):
                parts = s.split(); self.rows(parts[1], int(parts[2]) if len(parts) > 2 else 20); continue
            if s.startswith('count '):
                parts = s.split(); self.count(parts[1]); continue
            if s.startswith('sample '):
                parts = s.split(); self.sample(parts[1], int(parts[2]) if len(parts) > 2 else 5); continue
            if s.startswith('sql '):
                parts = s.split(None, 1); self.query(parts[1]); continue
            if s.startswith('run '):
                parts = s.split(None, 1); self.run_script(parts[1]); continue
            if s.startswith('run-dry '):
                parts = s.split(None, 1); self.run_script(parts[1], dry_run=True); continue
            # otherwise run raw SQL
            self.query(s)


if __name__ == '__main__':
    ap = argparse.ArgumentParser(description='EQEMu DB Viewer and Diagnostic Tool')
    ap.add_argument('--host', help='DB host override')
    ap.add_argument('--port', help='DB port override', type=int)
    ap.add_argument('--user', help='DB user override')
    ap.add_argument('--password', help='DB password override')
    ap.add_argument('--db', help='DB name override')

    group = ap.add_mutually_exclusive_group()
    group.add_argument('--list-tables', action='store_true')
    group.add_argument('--list-dbs', action='store_true')
    group.add_argument('--describe', help='Describe table')
    group.add_argument('--create-table', help='SHOW CREATE TABLE')
    group.add_argument('--rows', help='Show rows from table')
    group.add_argument('--count', help='Row count for table')
    group.add_argument('--sample', help='Random sample from table')
    group.add_argument('--query', help='Run SQL query')
    group.add_argument('--script', help='Run SQL script file')
    group.add_argument('--shell', action='store_true', help='Interactive shell')
    ap.add_argument('--limit', type=int, default=20, help='Limit for rows')
    ap.add_argument('--csv', help='Write query results to CSV')
    ap.add_argument('--stop-on-error', action='store_true', help='Stop script execution on first error')
    ap.add_argument('--dry-run', action='store_true', help='Print script statements without executing (use with --script)')

    args = ap.parse_args()

    dv = DBViewer(host=args.host, port=args.port, user=args.user, password=args.password, db=args.db)
    try:
        dv.connect()
    except Exception as e:
        print('Failed to connect to database:', e)
        sys.exit(2)

    try:
        if args.list_tables:
            dv.list_tables()
        elif args.list_dbs:
            dv.list_databases()
        elif args.describe:
            dv.describe_table(args.describe)
        elif args.create_table:
            dv.create_table(args.create_table)
        elif args.rows:
            dv.rows(args.rows, args.limit)
        elif args.count:
            dv.count(args.count)
        elif args.sample:
            dv.sample(args.sample, args.limit)
        elif args.query:
            dv.query(args.query, args.limit, args.csv)
        elif args.script:
            dv.run_script(args.script, stop_on_error=args.stop_on_error, dry_run=args.dry_run)
        elif args.shell:
            dv.shell()
        else:
            ap.print_help()
    finally:
        dv.close()
