#!/usr/bin/env python3
import json
import os
import pymysql

# Load DB config
def get_db_config():
    config_path = os.path.join(os.getcwd(), 'eqemu_config.json')
    if os.path.exists(config_path):
        with open(config_path, 'r') as f:
            config = json.load(f)
        dbcfg = config.get('server', {}).get('database', {})
        return {
            'host': dbcfg.get('host', '127.0.0.1'),
            'port': dbcfg.get('port', 3306),
            'user': dbcfg.get('username', 'root'),
            'password': dbcfg.get('password', ''),
            'db': dbcfg.get('db', 'peq')
        }
    return None

def score_item(item):
    """
    Very basic scoring logic based on ITEM_SCORING_SYSTEM.md
    """
    weights = {
        'hp': 1.0, 'mana': 1.0, 'ac': 4.0, 'astr': 2.5, 'asta': 2.5,
        'adex': 2.5, 'aagi': 2.5, 'aint': 2.5, 'awis': 2.5, 'acha': 2.5,
        'haste': 10.0, 'damage': 15.0, 'attack': 2.0
    }

    score = 0
    for stat, weight in weights.items():
        val = item.get(stat, 0)
        if val:
            score += float(val) * weight

    return score

def main():
    cfg = get_db_config()
    if not cfg:
        print("Could not find eqemu_config.json")
        return

    conn = pymysql.connect(
        host=cfg['host'], port=cfg['port'],
        user=cfg['user'], password=cfg['password'],
        database=cfg['db'], cursorclass=pymysql.cursors.DictCursor
    )

    try:
        with conn.cursor() as cur:
            # Query items and their best expansion source
            # This query samples 10 items and finds their earliest expansion via loot
            sql = """
                SELECT i.id, i.Name, i.ac, i.hp, i.mana, i.haste, i.damage, i.astr, i.asta,
                       (SELECT MIN(z.expansion)
                        FROM lootdrop_entries le
                        JOIN loottable_entries lte ON le.lootdrop_id = lte.lootdrop_id
                        JOIN npc_types n ON n.loottable_id = lte.loottable_id
                        JOIN spawnentry se ON se.npcID = n.id
                        JOIN spawngroup sg ON se.spawngroupID = sg.id
                        JOIN spawn2 s2 ON s2.spawngroupID = sg.id
                        JOIN zone z ON s2.zone = z.short_name
                        WHERE le.item_id = i.id
                       ) as expansion
                FROM items i
                ORDER BY i.id DESC
                LIMIT 10
            """
            cur.execute(sql)
            items = cur.fetchall()

            print(f"{'ID':<10} | {'Name':<30} | {'Exp':<3} | {'Score':<10}")
            print("-" * 65)
            for it in items:
                ps = score_item(it)
                exp = it['expansion'] if it['expansion'] is not None else -1
                print(f"{it['id']:<10} | {it['Name'][:30]:<30} | {exp:<3} | {ps:<10.2f}")

    finally:
        conn.close()

if __name__ == "__main__":
    main()
