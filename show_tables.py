import pymysql, json
cfg = json.load(open('eqemu_config.json'))['server']['database']
conn = pymysql.connect(host=cfg['host'], port=cfg['port'], user=cfg['username'], password=cfg['password'], database=cfg['db'])
cur = conn.cursor()
cur.execute("SHOW TABLES LIKE '%inventory%'")
for t in cur.fetchall():
    print(t[0])
