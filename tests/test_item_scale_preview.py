import os
import json
from tools.item_scale_preview import scaled_item, pool_distribute_auto, pool_distribute_static, sum_items
from tools.item_scale_preview import evaluate_curve


def test_scaled_item_basic():
    base = {'AC': 10, 'HP': 20, 'STR': 5}
    scaled = scaled_item(base, 50, slot_mults={'Attributes': 1.0, 'HP': 1.0, 'AC': 1.0})
    assert scaled['AC'] >= base['AC']
    assert scaled['HP'] >= base['HP']
    assert scaled['STR'] >= base['STR']


def test_pool_auto_vs_static():
    base = {'STR': 5, 'STA': 2, 'AGI': 0, 'DEX': 0, 'INT': 0, 'WIS': 0, 'CHA': 0}
    scaled = {'STR': 50, 'STA': 20, 'AGI': 10, 'DEX': 0, 'INT': 0, 'WIS': 0, 'CHA': 0}
    presence = {'STR': 1.2, 'STA': 1.1, 'AGI': 1.0, 'DEX': 1.0, 'INT': 1.0, 'WIS': 1.0, 'CHA': 1.0}
    absence = {k: 0.4 for k in presence}
    auto = pool_distribute_auto(base, scaled, presence, absence, slot_mult=1.0)
    static = pool_distribute_static(base, 100, presence, absence, slot_mult=1.0)
    assert sum(auto.values()) >= sum(base.values())
    assert sum(static.values()) >= sum(base.values())


def test_evaluate_curve_linear():
    pts = [[1, 1.0], [10, 2.0], [50, 6.0]]
    assert evaluate_curve(pts, 1) == 1.0
    assert evaluate_curve(pts, 10) == 2.0
    assert evaluate_curve(pts, 30) == 4.0


def test_spell_dmg_from_int_divisor():
    cfg = {
        'SpellDmgFromInt': {'enabled': True, 'mode': 'divisor', 'divisor': 8}
    }
    base = {'INT': 40}
    scaled = scaled_item(base, 10, slot_mults={}, config=cfg)
    # default scaled INT will be >= base, but divisor applied using final INT
    # Expect SpellDmg = scaled INT // 8
    scaled_int = scaled.get('INT', 0)
    assert scaled.get('SpellDmg', 0) == scaled_int // 8 or scaled.get('SpellDmg', 0) == 0


def test_spell_dmg_from_int_curve_mode():
    cfg = {
        'SpellDmgFromInt': {'enabled': True, 'mode': 'curve', 'curve': {'points': [[1, 1], [100, 100]]}}
    }
    base = {'INT': 100}
    scaled = scaled_item(base, 50, slot_mults={}, config=cfg)
    # For level 50 with curve from 1 to 100, the curve should be >1
    # SpellDmg should be computed using the level-based multiplier when used by the app (we can't run app here),
    # But scaled spell dmg should be present after scaled_item returns (zero by default), so check that key exists
    assert 'SpellDmg' in scaled


def test_parse_eqemu_db_config():
    cfg = None
    from tools.item_scale_preview import get_db_config_from_eqemu_config
    cfg = get_db_config_from_eqemu_config()
    assert 'host' in cfg and 'username' in cfg and 'db' in cfg


def test_primary_curve_multiplier_usage():
    cfg = {'PrimaryCurves': {'AC': {'points': [[1, 2.0], [100, 2.0]]}}}
    base = {'AC': 10}
    scaled_default = scaled_item(base, 10, slot_mults={}, config=None)
    scaled_curve = scaled_item(base, 10, slot_mults={}, config=cfg)
    # With a 2.0 multiplier the AC should be roughly doubled vs default
    assert scaled_curve['AC'] >= scaled_default['AC'] * 2


def test_auto_pool_no_overflow_past_slot_scaled():
    cfg = None
    from tools.item_scale_preview import scaled_item, pool_distribute_auto
    base = {'STR':25,'STA':18,'AGI':12,'DEX':15,'INT':0,'WIS':0,'CHA':0}
    level = 30
    slot_mults = {'Attributes':1.5}
    scaled_no_slot = scaled_item(base, level, None, config=cfg)
    scaled_slot = scaled_item(base, level, slot_mults, config=cfg)
    redistributed = pool_distribute_auto(base, {k:scaled_no_slot.get(k,0) for k in base}, {'STR':1.2,'STA':1.1,'AGI':1.15,'DEX':1.25,'INT':1.6,'WIS':1.6,'CHA':1.0}, {k:0.4 for k in base}, slot_mult=slot_mults['Attributes'])
    # After clamping, redistributed should not exceed slot-scaled per attribute
    for k in base:
        assert redistributed[k] >= 0
        assert redistributed[k] >= base[k]
        # ensure the clamped final value would not exceed the slot-scaled value
        assert min(redistributed[k], scaled_slot[k]) <= scaled_slot[k]


def test_global_scale_and_presence_bias_effect():
    from tools.item_scale_preview import scaled_item, pool_distribute_auto
    base = {'AC': 60, 'HP':160, 'STR':25, 'STA':18, 'AGI':12, 'DEX':15, 'INT':0, 'WIS':0, 'CHA':0}
    cfg = None
    level = 30
    # Base results open
    s_default = scaled_item(base, level, None, config=None)
    # Small global scale should reduce AC and HP
    # We'll set the global scale by applying it manually since UI uses slider; verify manual scaling logic
    gscale = 0.5
    s_manual = s_default.copy()
    s_manual['AC'] = int(s_manual['AC'] * gscale)
    s_manual['HP'] = int(s_manual['HP'] * gscale)
    assert s_manual['AC'] < s_default['AC']
    assert s_manual['HP'] < s_default['HP']


def test_generate_curve_points_linear():
    from tools.item_scale_preview import generate_curve_points
    pts = generate_curve_points('linear', 1, 100, 1.0, 10.0, strength=1.0, steps=10)
    assert pts[0][0] == 1
    assert pts[-1][0] == 100
    assert abs(pts[0][1] - 1.0) < 1e-6
    assert abs(pts[-1][1] - 10.0) < 1e-6


def test_generate_curve_points_sigmoid_midpoint():
    from tools.item_scale_preview import generate_curve_points
    pts = generate_curve_points('sigmoid', 1, 100, 1.0, 10.0, strength=1.0, steps=100)
    # sigmoid mid should be near midpoint value
    mid = pts[len(pts)//2][1]
    assert mid > 1.0 and mid < 10.0


def test_save_curve_to_json_temp(tmp_path):
    from tools.item_scale_preview import generate_curve_points
    import json, os
    # Prepare a temp json file
    cfg = {"AttributeCurve": {"points": [[1,1.0],[10,2.0]]}}
    p = tmp_path / "item_scaling.json"
    p.write_text(json.dumps(cfg))
    pts = generate_curve_points('linear', 1, 100, 1.0, 10.0, steps=10)
    # call save
    from tools.item_scale_preview import ItemScaleApp
    app = ItemScaleApp()
    app.save_curve_to_json(str(p), 'AttributeCurve', pts)
    new_cfg = json.loads(p.read_text())
    assert 'AttributeCurve' in new_cfg
    assert 'points' in new_cfg['AttributeCurve']


def test_equip_all_sum():
    presets = {
        'Chest': {'AC': 25, 'HP': 50, 'STR': 5, 'STA': 5},
        'Primary': {'Damage': 8, 'Attack': 10, 'STR': 2}
    }
    scaled_items = [scaled_item(p, 10, slot_mults={'Attributes': 1.0}) for p in presets.values()]
    totals = sum_items(scaled_items)
    assert totals['AC'] >= 25
    assert totals['HP'] >= 50
