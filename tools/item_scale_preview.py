#!/usr/bin/env python3
"""
Item scale preview with simple Tkinter UI
This script provides a GUI to enter base item stats, choose level, and see scaled outcomes.
Features:
- Uses JSON-based curves from `game_design/infinite_progression/item_scaling.json` for more accurate previews (Primary/Weapon/Mod2/Attribute curves, and derived stats)
- Optional 'Load From DB' to fetch base item stats from the server's `items` table using `eqemu_config.json` credentials and `mysql-connector-python`. This is optional.
- Equip-all slots preset and combined totals view

Usage: python tools/item_scale_preview.py
"""

import json
import math
import os
import tkinter as tk
from tkinter import ttk, messagebox
HAS_MATPLOTLIB = True
try:
    import matplotlib
    matplotlib.use('TkAgg')
    from matplotlib.figure import Figure
    from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
except Exception:
    HAS_MATPLOTLIB = False
HAS_MYSQL = True
try:
    import mysql.connector
    from mysql.connector import Error
except Exception:
    HAS_MYSQL = False

DEFAULT_CONFIG_PATH = os.path.join(os.path.dirname(__file__), '..', 'game_design', 'infinite_progression', 'item_scaling.json')


# Basic tiered stat function (similar to CalculateTieredStat)
def calculate_tiered_stat(base_value, level, base_increment=1, tier_bonus=1, tier_size=10):
    if level <= 0:
        return base_value
    total = base_value
    tier = level // tier_size
    for t in range(tier):
        increment = base_increment + (t * tier_bonus)
        total += tier_size * increment
    remaining = level % tier_size
    current_increment = base_increment + (tier * tier_bonus)
    total += remaining * current_increment
    return total


def apply_stat_cap(value):
    base_display = value
    heroic = 0
    if base_display > 127:
        heroic = base_display - 127
        base_display = 127
    return base_display, heroic


# Pool distribution - Option A auto budget
def pool_distribute_auto(base_attrs, scaled_attrs, presence_mults, absence_mults, slot_mult=1.0):
    raw_scaled = {k: scaled_attrs.get(k, 0) for k in base_attrs}
    attr_keys = ['STR', 'STA', 'AGI', 'DEX', 'INT', 'WIS', 'CHA']
    budget = sum(max(0, raw_scaled.get(k, 0) - base_attrs.get(k, 0)) for k in attr_keys)
    budget = int(math.floor(budget * slot_mult))

    weights = {}
    for k in base_attrs:
        if base_attrs.get(k, 0) > 0:
            weights[k] = base_attrs.get(k, 0) * presence_mults.get(k, 1.0)
        else:
            weights[k] = absence_mults.get(k, 0.1)

    total_weight = sum(weights.values())
    if total_weight <= 0 or budget <= 0:
        # even distribution or nothing to distribute
        per = budget // len(base_attrs) if budget > 0 else 0
        return {k: base_attrs.get(k, 0) + per for k in base_attrs}

    result = {}
    for k in base_attrs:
        share = math.floor((weights[k] / total_weight) * budget)
        result[k] = base_attrs.get(k, 0) + share
    return result


def pool_distribute_static(base_attrs, static_budget, presence_mults, absence_mults, slot_mult=1.0):
    weights = {}
    for k in base_attrs:
        if base_attrs.get(k, 0) > 0:
            weights[k] = base_attrs.get(k, 0) * presence_mults.get(k, 1.0)
        else:
            weights[k] = absence_mults.get(k, 0.1)
    total_weight = sum(weights.values())
    if total_weight <= 0 or static_budget <= 0:
        per = static_budget // len(base_attrs) if static_budget > 0 else 0
        return {k: base_attrs.get(k, 0) + per for k in base_attrs}
    result = {}
    static_budget = int(math.floor(static_budget * slot_mult))
    for k in base_attrs:
        share = math.floor((weights[k] / total_weight) * static_budget)
        result[k] = base_attrs.get(k, 0) + share
    return result


def get_default_slot_multipliers(config):
    # Return defaults if no JSON present
    defaults = {
        'Chest': {'Attributes': 1.5, 'HP': 1.2, 'AC': 1.2},
        'Head': {'Attributes': 1.25, 'HP': 1.1, 'AC': 1.1},
        'Legs': {'Attributes': 1.1, 'HP': 1.05},
        'Arms': {'Attributes': 1.0},
        'Hands': {'Attributes': 1.0},
        'Feet': {'Attributes': 1.0},
        'Primary': {'Attributes': 1.25, 'Damage': 1.15, 'Attack': 1.1},
        'Secondary': {'Attributes': 1.0, 'Damage': 1.05},
        'Range': {'Attributes': 1.0, 'Damage': 1.1},
        'Wrist': {'Attributes': 0.75},
        'Back': {'Attributes': 1.0},
        'Neck': {'Attributes': 1.0},
        'Ear': {'Attributes': 0.5},
        'Ring': {'Attributes': 0.5},
        'Face': {'Attributes': 0.5}
    }
    if not config:
        return defaults
    return config.get('SlotMultipliers', defaults)


def get_db_config_from_eqemu_config(eqemu_path=None):
    path = eqemu_path or os.path.join(os.path.dirname(__file__), '..', 'eqemu_config.json')
    if not os.path.exists(path):
        raise RuntimeError('eqemu_config.json not found')
    with open(path, 'r') as f:
        cfg = json.load(f)
    dbcfg = cfg.get('server', {}).get('database', {})
    if not dbcfg:
        raise RuntimeError('Database config not found')
    return dbcfg


def evaluate_curve(points, level):
    """
    Evaluate a piecewise linear curve. 'points' is a list of [level, value]
    Return interpolated value for 'level'.
    """
    if not points:
        return 1.0
    pts = sorted(points, key=lambda x: x[0])
    if level <= pts[0][0]:
        return pts[0][1]
    if level >= pts[-1][0]:
        return pts[-1][1]
    for i in range(1, len(pts)):
        a_lvl, a_val = pts[i - 1]
        b_lvl, b_val = pts[i]
        if level >= a_lvl and level <= b_lvl:
            # linear interpolation
            t = (level - a_lvl) / float(b_lvl - a_lvl)
            return a_val + (b_val - a_val) * t
    return pts[-1][1]


def generate_curve_points(preset, start_lvl, end_lvl, start_val, end_val, strength=1.0, steps=100):
    """
    Generate piecewise points for a curve between start_lvl and end_lvl using preset:
    - preset: 'linear', 'exponential', 'sigmoid', 'asymptotic'
    - strength: curve strength parameter
    Returns list of [level, value]
    """
    if start_lvl >= end_lvl:
        # single point
        return [[start_lvl, start_val]]

    points = []
    import math
    for i in range(steps + 1):
        t = i / float(steps)
        level = start_lvl + (end_lvl - start_lvl) * t
        if preset == 'linear':
            val = start_val + (end_val - start_val) * t
        elif preset == 'exponential':
            # exponential interpolation with strength controlling curvature. strength >0, default 1
            if start_val == 0:
                base = 1e-6
            else:
                base = start_val
            # compute geometric interpolation using pow
            if start_val <= 0 or end_val <= 0:
                # fallback linear if invalid for geometric interpolation
                val = start_val + (end_val - start_val) * (t ** (1.0 + strength))
            else:
                ratio = (end_val / base) ** (t ** (1.0 / max(0.0001, strength)))
                val = base * ratio
        elif preset == 'sigmoid':
            # logistic curve between start_val and end_val; strength controls steepness
            k = max(0.1, strength * 6.0)
            x0 = 0.5
            logistic = 1.0 / (1.0 + math.exp(-k * (t - x0)))
            # normalize logistic to 0..1
            minL = 1.0 / (1.0 + math.exp(-k * (0 - x0)))
            maxL = 1.0 / (1.0 + math.exp(-k * (1 - x0)))
            norm = (logistic - minL) / (maxL - minL)
            val = start_val + (end_val - start_val) * norm
        elif preset == 'asymptotic':
            # approach end_val asymptotically; strength controls rate
            k = max(0.01, strength * 0.1)
            val = end_val - (end_val - start_val) * math.exp(-k * (t * (end_lvl - start_lvl)))
        else:
            # default to linear
            val = start_val + (end_val - start_val) * t
        points.append([int(round(level)), float(val)])
    # compress points by unique levels
    compressed = []
    seen = set()
    for lvl, v in points:
        if lvl not in seen:
            compressed.append([lvl, v])
            seen.add(lvl)
    return compressed


def scaled_item(base, level, slot_mults=None, config=None):
    # Use calculate_tiered_stat for attributes and weapon/damage/attack
    out = base.copy()
    # primary stats
    # If JSON curves exist, use them; otherwise fall back to tiered defaults
    if config and 'PrimaryCurves' in config:
        pc = config['PrimaryCurves']
        ac_curve = pc.get('AC', {}).get('points') if 'AC' in pc else None
        hp_curve = pc.get('HP', {}).get('points') if 'HP' in pc else None
        ac_mult = evaluate_curve(ac_curve, level) if ac_curve else 1.0
        hp_mult = evaluate_curve(hp_curve, level) if hp_curve else 1.0
        out['AC'] = int(math.floor(calculate_tiered_stat(base.get('AC', 0), level, base_increment=1, tier_bonus=1) * ac_mult))
        out['HP'] = int(math.floor(calculate_tiered_stat(base.get('HP', 0), level, base_increment=4, tier_bonus=4) * hp_mult))
    else:
        out['AC'] = calculate_tiered_stat(base.get('AC', 0), level, base_increment=1, tier_bonus=1)
        out['HP'] = calculate_tiered_stat(base.get('HP', 0), level, base_increment=4, tier_bonus=4)
    # For the preview we use attribute base increment 1
    for k in ['STR', 'STA', 'AGI', 'DEX', 'INT', 'WIS', 'CHA']:
        # AttributeCurve multiplier from JSON
        attr_curve_points = None
        if config:
            # prefer curve_override from config's AttributeCurve if present, else use config default
            attr_curve_points = config.get('AttributeCurve', {}).get('points') if 'AttributeCurve' in config and config.get('AttributeCurve') else None
        attr_mult = evaluate_curve(attr_curve_points, level) if attr_curve_points else 1.0
        out[k] = int(math.floor(calculate_tiered_stat(base.get(k, 0), level, base_increment=1, tier_bonus=1) * attr_mult))
    # Mod2-ish
    mod_points = config.get('Mod2Curves', {}) if config else {}
    shielding_curve = mod_points.get('Shielding', {}).get('points')
    strikethrough_curve = mod_points.get('StrikeThrough', {}).get('points')
    shielding_mult = evaluate_curve(shielding_curve, level) if shielding_curve else 1.0
    strk_mult = evaluate_curve(strikethrough_curve, level) if strikethrough_curve else 1.0
    out['Shielding'] = int(math.floor(calculate_tiered_stat(base.get('Shielding', 0), level, base_increment=1, tier_bonus=0) * shielding_mult))
    out['StrikeThrough'] = int(math.floor(calculate_tiered_stat(base.get('StrikeThrough', 0), level, base_increment=2, tier_bonus=1) * strk_mult))
    # weapon fields
    if base.get('Damage', 0) > 0:
        weapon_curves = config.get('WeaponCurves', {}) if config else {}
        dmg_curve_points = weapon_curves.get('Damage', {}).get('points')
        atk_curve_points = weapon_curves.get('Attack', {}).get('points')
        dmg_mult = evaluate_curve(dmg_curve_points, level) if dmg_curve_points else 1.0
        atk_mult = evaluate_curve(atk_curve_points, level) if atk_curve_points else 1.0
        out['Damage'] = int(math.floor(calculate_tiered_stat(base.get('Damage', 0), level, base_increment=4, tier_bonus=4) * dmg_mult))
        out['Attack'] = int(math.floor(calculate_tiered_stat(base.get('Attack', 0), level, base_increment=2, tier_bonus=2) * atk_mult))

    # SpellDmg and HealAmt will be computed after pool redistribution in the UI flow
    out['SpellDmg'] = base.get('SpellDmg', 0)
    out['HealAmt'] = base.get('HealAmt', 0)

    # Apply slot multipliers
    if slot_mults:
        if 'HP' in slot_mults and out.get('HP', 0) > 0:
            out['HP'] = int(math.floor(out['HP'] * slot_mults['HP']))
        if 'AC' in slot_mults and out.get('AC', 0) > 0:
            out['AC'] = int(math.floor(out['AC'] * slot_mults['AC']))
        if 'Damage' in slot_mults and out.get('Damage', 0) > 0:
            out['Damage'] = int(math.floor(out['Damage'] * slot_mults['Damage']))
        if 'Attack' in slot_mults and out.get('Attack', 0) > 0:
            out['Attack'] = int(math.floor(out['Attack'] * slot_mults['Attack']))
        if 'Attributes' in slot_mults:
            for k in ['STR', 'STA', 'AGI', 'DEX', 'INT', 'WIS', 'CHA']:
                if out.get(k, 0) > 0:
                    out[k] = int(math.floor(out[k] * slot_mults['Attributes']))

    return out


def sum_items(items):
    totals = {}
    for item in items:
        for k, v in item.items():
            totals[k] = totals.get(k, 0) + v
    return totals


class ItemScaleApp(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title('Item Scale Preview')
        self.geometry('950x700')
        self.config = self.load_config()
        self.slot_mults = get_default_slot_multipliers(self.config)
        if self.config and 'AttributePreferences' in self.config:
            self.presence_mults = {}
            for k, v in self.config.get('AttributePreferences', {}).items():
                # keys in JSON are like 'AStr' - map to 'STR'
                if k.startswith('A') and len(k) > 1:
                    mapped = k[1:].upper()
                else:
                    mapped = k.upper()
                self.presence_mults[mapped] = v.get('presence_mult', 1.0)
        else:
            self.presence_mults = {'STR': 1.2, 'STA': 1.1, 'AGI': 1.15, 'DEX': 1.25, 'INT': 1.6, 'WIS': 1.6, 'CHA': 1.0}
        self.absence_mults = {k: 0.4 for k in self.presence_mults}
        # Plot helpers
        self.last_curve_points = None
        self.marker_line = None
        self.marker_point = None
        self.create_widgets()

    def load_config(self):
        try:
            if os.path.exists(DEFAULT_CONFIG_PATH):
                with open(DEFAULT_CONFIG_PATH, 'r') as f:
                    cfg = json.load(f)
                    return cfg
        except Exception:
            pass
        return None

    def plot_curve_from_ui(self):
        if not HAS_MATPLOTLIB:
            messagebox.showerror('Plotting unavailable', "Matplotlib is required to plot curves. Install with 'pip install matplotlib'.")
            return
        try:
            preset = self.curve_preset_var.get()
            s_lvl = int(self.curve_start_level.get())
            e_lvl = int(self.curve_end_level.get())
            s_val = float(self.curve_start_val.get())
            e_val = float(self.curve_end_val.get())
            strength = float(self.curve_strength_var.get())
            points = generate_curve_points(preset, s_lvl, e_lvl, s_val, e_val, strength, steps=200)
            # plot
            self.ax.clear()
            xs = [p[0] for p in points]
            ys = [p[1] for p in points]
            self.ax.plot(xs, ys, label=f'{preset}')
            # optional plots for primary attributes
            try:
                # draw markers for base and scaled attributes if present
                level = int(self.level_var.get()) if hasattr(self, 'level_var') else None
                if level and hasattr(self, 'entries'):
                    # compute attribute base/slot-scaled values and mark them on the plot
                    base = self.get_item_from_inputs()
                    sl = int(self.level_var.get())
                    slot = self.slot_var.get()
                    slot_mult = self.slot_mults.get(slot, {})
                    s_no_slot = scaled_item(base, sl, None, config=self.config)
                    s_slot = scaled_item(base, sl, slot_mult, config=self.config)
                    # pick an attribute to mark (selected in curve attr var)
                    attr = self.curve_attr_var.get() if hasattr(self, 'curve_attr_var') else 'STR'
                    if attr and attr in s_slot:
                        aval = s_slot[attr]
                        # draw a horizontal line where the attribute value sits relative to the multiplier curve
                        self.ax.axhline(aval, color='green', linestyle=':', linewidth=1, zorder=2)
                        # note: attribute value vs curve multiplier is not an exact relation but provides a visual cue
            except Exception:
                pass
            self.ax.set_xlabel('Level')
            self.ax.set_ylabel('Scale Mult')
            self.ax.set_title(f'{preset} curve ({s_lvl}-{e_lvl})')
            self.ax.grid(True)
            self.ax.legend()
            # remember last plotted curve for marker updates
            self.last_curve_points = points
            self.canvas.draw()
            self.update_plot_marker()
        except Exception as e:
            messagebox.showerror('Plot error', str(e))

    def apply_curve_override(self):
        # Create curve points from UI and set into memory override as AttributeCurve
        try:
            preset = self.curve_preset_var.get()
            s_lvl = int(self.curve_start_level.get())
            e_lvl = int(self.curve_end_level.get())
            s_val = float(self.curve_start_val.get())
            e_val = float(self.curve_end_val.get())
            strength = float(self.curve_strength_var.get())
            points = generate_curve_points(preset, s_lvl, e_lvl, s_val, e_val, strength, steps=200)
            self.curve_override = points
            # Keep override in the config memory to use for scaling
            if not self.config:
                self.config = {}
            self.config['AttributeCurve'] = {'points': points}
            messagebox.showinfo('Applied', 'Curve applied for preview (using AttributeCurve override).')
            self.last_curve_points = points
            if self.auto_update.get():
                # re-evaluate preview and plots
                self.on_control_change()
        except Exception as e:
            messagebox.showerror('Error', str(e))

    def save_curve_to_json_ui(self):
        if not os.path.exists(DEFAULT_CONFIG_PATH):
            messagebox.showerror('Error', f'{DEFAULT_CONFIG_PATH} not found')
            return
        target = self.save_target_var.get() if hasattr(self, 'save_target_var') else 'AttributeCurve'
        if not self.curve_override:
            messagebox.showinfo('No override', 'No curve override present — apply or plot a curve to create one before saving')
            return
        try:
            self.save_curve_to_json(DEFAULT_CONFIG_PATH, target, self.curve_override)
            messagebox.showinfo('Saved', f'Saved curve to {DEFAULT_CONFIG_PATH} -> {target}')
            if self.auto_update.get():
                self.on_control_change()
        except Exception as e:
            messagebox.showerror('Save error', str(e))


    def save_curve_to_json(self, path, target, points):
        # Make backup first
        import shutil, datetime
        timestamp = datetime.datetime.now().strftime('%Y%m%dT%H%M%S')
        bak_path = path + f'.bak.{timestamp}'
        shutil.copy2(path, bak_path)
        # Load existing json and set target
        with open(path, 'r', encoding='utf8') as f:
            cfg = json.load(f)
        # target can be 'AttributeCurve' or 'PrimaryCurves.AC' (dot separated)
        parts = target.split('.')
        cur = cfg
        for p in parts[:-1]:
            if p not in cur or not isinstance(cur[p], dict):
                cur[p] = {}
            cur = cur[p]
        # set the points key on last part
        last = parts[-1]
        # If config wants a dict with points, ensure structure
        cur[last] = {'points': points}
        # Write back
        with open(path, 'w', encoding='utf8') as f:
            json.dump(cfg, f, indent=2)

    def create_widgets(self):
        left = tk.Frame(self)
        left.pack(side='left', fill='y', padx=8, pady=8)

        right = tk.Frame(self)
        right.pack(side='right', fill='both', expand=True, padx=8, pady=8)

        # Top controls (level, slot)
        top_ctrl = tk.Frame(left)
        top_ctrl.grid(row=0, column=0, columnspan=2, sticky='we')
        tk.Label(top_ctrl, text='Item Level:').grid(row=0, column=0, sticky='w')
        self.level_var = tk.IntVar(value=100)
        tk.Spinbox(top_ctrl, from_=1, to=10000, textvariable=self.level_var, width=8).grid(row=0, column=1)
        tk.Label(top_ctrl, text='Slot:').grid(row=0, column=2, sticky='w', padx=(12, 0))
        self.slot_var = tk.StringVar(value='Chest')
        slot_names = list(self.slot_mults.keys())
        ttk.Combobox(top_ctrl, values=slot_names, textvariable=self.slot_var, width=12).grid(row=0, column=3)

        # DB Load section
        db_frame = tk.LabelFrame(left, text='DB Load (optional)', padx=4, pady=4)
        db_frame.grid(row=1, column=0, columnspan=2, sticky='we', pady=(6, 8))
        tk.Label(db_frame, text='Item ID load:').grid(row=0, column=0, sticky='w', padx=(0, 4))
        self.item_id_var = tk.IntVar(value=0)
        tk.Entry(db_frame, textvariable=self.item_id_var, width=12).grid(row=0, column=1)
        tk.Button(db_frame, text='Load From DB', command=self.load_item_from_db_gui).grid(row=0, column=2, padx=(6, 0))
        # Search by name
        tk.Label(db_frame, text='Search name:').grid(row=1, column=0, sticky='w', pady=(6, 0))
        self.search_name_var = tk.StringVar(value='')
        tk.Entry(db_frame, textvariable=self.search_name_var, width=18).grid(row=1, column=1, sticky='w')
        tk.Button(db_frame, text='Search DB', command=self.search_db_gui).grid(row=1, column=2, padx=(6, 0))
        # DB status label
        db_status = 'DB Loader: Available' if HAS_MYSQL else "DB Loader: Missing 'mysql-connector-python'"
        tk.Label(db_frame, text=db_status).grid(row=2, column=0, columnspan=3, sticky='w', pady=(6, 0))
        # Search results listbox with scrollbar
        self.search_results_listbox = tk.Listbox(db_frame, height=6, width=40)
        self.search_results_listbox.grid(row=3, column=0, columnspan=2, sticky='w', pady=(6, 0))
        self.search_results_listbox.bind('<Double-Button-1>', self.load_selected_from_search)
        scrollbar = tk.Scrollbar(db_frame, orient='vertical', command=self.search_results_listbox.yview)
        scrollbar.grid(row=3, column=2, sticky='nsw', pady=(6, 0))
        self.search_results_listbox.config(yscrollcommand=scrollbar.set)
        tk.Button(db_frame, text='Load Selected', command=self.load_selected_from_search).grid(row=4, column=0, columnspan=3, pady=(6, 0))

        # Pool mode section
        pool_frame = tk.LabelFrame(left, text='Pool Mode', padx=4, pady=4)
        pool_frame.grid(row=2, column=0, columnspan=2, sticky='we')
        self.pool_mode_var = tk.StringVar(value='auto')
        modes = [('Auto', 'auto'), ('Static', 'static'), ('None', 'none')]
        for i, (t, v) in enumerate(modes):
            tk.Radiobutton(pool_frame, text=t, value=v, variable=self.pool_mode_var).grid(row=0, column=i, sticky='w')
        tk.Label(pool_frame, text='Static Budget:').grid(row=1, column=0, sticky='w', pady=(6, 0))
        self.static_budget_var = tk.IntVar(value=100)
        tk.Entry(pool_frame, textvariable=self.static_budget_var, width=8).grid(row=1, column=1, sticky='w')
        # Global scaling controls
        tk.Label(pool_frame, text='Global Scale:').grid(row=2, column=0, sticky='w', pady=(6, 0))
        self.global_scale_var = tk.DoubleVar(value=1.0)
        tk.Scale(pool_frame, variable=self.global_scale_var, from_=0.1, to=2.0, resolution=0.05, orient='horizontal', length=120).grid(row=2, column=1, sticky='w')
        tk.Label(pool_frame, text='Presence Bias:').grid(row=3, column=0, sticky='w', pady=(6, 0))
        self.presence_bias_var = tk.DoubleVar(value=1.0)
        tk.Scale(pool_frame, variable=self.presence_bias_var, from_=0.0, to=1.0, resolution=0.05, orient='horizontal', length=120).grid(row=3, column=1, sticky='w')
        # Curve generator controls
        curve_frame = tk.LabelFrame(left, text='Attribute Curve Generator', padx=4, pady=4)
        curve_frame.grid(row=4, column=0, columnspan=2, sticky='we', pady=(8, 0))
        tk.Label(curve_frame, text='Attribute:').grid(row=0, column=0, sticky='w')
        self.curve_attr_var = tk.StringVar(value='STR')
        attrs = ['STR', 'STA', 'AGI', 'DEX', 'INT', 'WIS', 'CHA']
        ttk.Combobox(curve_frame, values=attrs, textvariable=self.curve_attr_var, width=6).grid(row=0, column=1, sticky='w')
        tk.Label(curve_frame, text='Preset:').grid(row=0, column=2, sticky='w', padx=(8, 0))
        self.curve_preset_var = tk.StringVar(value='linear')
        ttk.Combobox(curve_frame, values=['linear', 'exponential', 'sigmoid', 'asymptotic'], textvariable=self.curve_preset_var, width=12).grid(row=0, column=3, sticky='w')
        tk.Label(curve_frame, text='Start Level:').grid(row=1, column=0, sticky='w')
        self.curve_start_level = tk.IntVar(value=1)
        tk.Entry(curve_frame, textvariable=self.curve_start_level, width=6).grid(row=1, column=1, sticky='w')
        tk.Label(curve_frame, text='End Level:').grid(row=1, column=2, sticky='w')
        self.curve_end_level = tk.IntVar(value=100)
        tk.Entry(curve_frame, textvariable=self.curve_end_level, width=6).grid(row=1, column=3, sticky='w')
        tk.Label(curve_frame, text='Start Val:').grid(row=2, column=0, sticky='w')
        self.curve_start_val = tk.DoubleVar(value=1.0)
        tk.Entry(curve_frame, textvariable=self.curve_start_val, width=6).grid(row=2, column=1, sticky='w')
        tk.Label(curve_frame, text='End Val:').grid(row=2, column=2, sticky='w')
        self.curve_end_val = tk.DoubleVar(value=10.0)
        tk.Entry(curve_frame, textvariable=self.curve_end_val, width=6).grid(row=2, column=3, sticky='w')
        tk.Label(curve_frame, text='Strength:').grid(row=3, column=0, sticky='w')
        self.curve_strength_var = tk.DoubleVar(value=1.0)
        tk.Scale(curve_frame, variable=self.curve_strength_var, from_=0.1, to=5.0, resolution=0.1, orient='horizontal', length=150).grid(row=3, column=1, columnspan=3, sticky='w')
        tk.Button(curve_frame, text='Plot Curve', command=self.plot_curve_from_ui).grid(row=4, column=0, pady=(4, 0))
        tk.Button(curve_frame, text='Apply to AttributeCurve (Preview Only)', command=self.apply_curve_override).grid(row=4, column=1, columnspan=2, pady=(4, 0))
        tk.Label(curve_frame, text='Save Target:').grid(row=5, column=0, sticky='w', pady=(6, 0))
        self.save_target_var = tk.StringVar(value='AttributeCurve')
        save_targets = [
            'AttributeCurve', 'PrimaryCurves.AC', 'PrimaryCurves.HP', 'WeaponCurves.Damage', 'WeaponCurves.Attack', 'Mod2Curves.Shielding', 'Mod2Curves.StrikeThrough'
        ]
        ttk.Combobox(curve_frame, values=save_targets, textvariable=self.save_target_var, width=20).grid(row=5, column=1, sticky='w')
        tk.Button(curve_frame, text='Save to JSON', command=self.save_curve_to_json_ui).grid(row=5, column=2, columnspan=2, pady=(6, 0))

        # Item inputs
        self.entries = {}
        stat_keys = ['AC', 'HP', 'Damage', 'Attack', 'Shielding', 'StrikeThrough', 'STR', 'STA', 'AGI', 'DEX', 'INT', 'WIS', 'CHA']
        stats_frame = tk.LabelFrame(left, text='Base Stats (enter for the item)', padx=4, pady=4)
        stats_frame.grid(row=3, column=0, columnspan=2, pady=(8, 2), sticky='we')
        for i, k in enumerate(stat_keys):
            tk.Label(stats_frame, text=k + ':').grid(row=i, column=0, sticky='w')
            v = tk.IntVar(value=0)
            e = tk.Entry(stats_frame, textvariable=v, width=8)
            e.grid(row=i, column=1)
            self.entries[k] = v

        # Buttons
        tk.Button(left, text='Scale Item', command=self.scale_item).grid(row=20, column=0, pady=8)
        tk.Button(left, text='Equip All Slots', command=self.equip_all).grid(row=20, column=1, pady=8)
        tk.Button(left, text='Reset', command=self.reset_fields).grid(row=21, column=0)
        tk.Button(left, text='Export to clipboard', command=self.export_clipboard).grid(row=21, column=1)
        self.auto_update = tk.BooleanVar(value=True)
        tk.Checkbutton(left, text='Auto Update', variable=self.auto_update).grid(row=22, column=0, columnspan=2, sticky='w')

        # Output
        # Graph area (optional)
        graph_frame = tk.Frame(right)
        graph_frame.pack(side='top', fill='both', expand=False)
        if HAS_MATPLOTLIB:
            self.fig = Figure(figsize=(5, 2.0), dpi=100)
            self.ax = self.fig.add_subplot(111)
            self.canvas = FigureCanvasTkAgg(self.fig, master=graph_frame)
            self.canvas.get_tk_widget().pack(fill='both', expand=True)
        else:
            tk.Label(graph_frame, text='Matplotlib not installed; graph disabled').pack()

        tk.Label(right, text='Scaled Output:').pack(anchor='nw')
        # Details area: Before / After side-by-side
        details_frame = tk.Frame(right)
        details_frame.pack(side='top', fill='both', expand=True)

        before_box = tk.LabelFrame(details_frame, text='Base / Before', padx=4, pady=4)
        before_box.pack(side='left', fill='both', expand=True)
        self.before_text = tk.Text(before_box, wrap='none', height=18)
        self.before_text.pack(fill='both', expand=True)

        after_box = tk.LabelFrame(details_frame, text='Scaled / After', padx=4, pady=4)
        after_box.pack(side='right', fill='both', expand=True)
        self.output = tk.Text(after_box, wrap='none', height=18)
        self.output.pack(fill='both', expand=True)

        # Defaults for equip-all preset
        self.equip_presets = self.make_equip_presets()
        # Curve override (used by UI curve generator before saving to JSON)
        self.curve_override = None
        # Hook auto update
        self.bind_auto_update_controls()
        self.after(200, self.auto_update_tick)

    def bind_auto_update_controls(self):
        def safe_trace(var):
            try:
                var.trace_add('write', lambda *args: self.on_control_change())
            except Exception:
                try:
                    var.trace('w', lambda *args: self.on_control_change())
                except Exception:
                    pass

        # Vars to watch for changes
        watch_vars = [self.level_var, self.slot_var, self.global_scale_var, self.presence_bias_var, self.curve_attr_var, self.curve_preset_var, self.curve_start_level, self.curve_end_level, self.curve_start_val, self.curve_end_val, self.curve_strength_var]
        for v in watch_vars:
            safe_trace(v)
        for e in self.entries.values():
            try:
                e.trace_add('write', lambda *a: self.on_control_change())
            except Exception:
                try:
                    e.trace('w', lambda *a: self.on_control_change())
                except Exception:
                    pass

    def on_control_change(self):
        # Called when a watched control changes
        if self.auto_update.get():
            try:
                self.scale_item()
            except Exception:
                pass
            try:
                # Update plot if applicable
                if HAS_MATPLOTLIB:
                    self.plot_curve_from_ui()
            except Exception:
                pass

    def auto_update_tick(self):
        # Periodic updater
        try:
            if self.auto_update.get():
                self.scale_item()
                if HAS_MATPLOTLIB:
                    self.update_plot_marker()
        except Exception:
            pass
        # re-schedule
        self.after(200, self.auto_update_tick)

    def update_plot_marker(self):
        if not HAS_MATPLOTLIB or not self.last_curve_points:
            return
        try:
            level = int(self.level_var.get())
            val = evaluate_curve(self.last_curve_points, level)
            # remove old markers
            try:
                if self.marker_line:
                    self.marker_line.remove()
            except Exception:
                pass
            try:
                if self.marker_point:
                    self.marker_point.remove()
            except Exception:
                pass
            # add new
            self.marker_line = self.ax.axvline(level, color='gray', linestyle='--', linewidth=1, zorder=3)
            self.marker_point = self.ax.scatter([level], [val], color='red', zorder=5)
            # Also annotate the current value with a label on the side
            try:
                # Remove old annotation if exists
                if hasattr(self, 'marker_annot') and self.marker_annot:
                    self.marker_annot.remove()
            except Exception:
                pass
            try:
                self.marker_annot = self.ax.annotate(f'{val:.2f}', xy=(level, val), xytext=(level + 1, val + 0.5), fontsize=8, color='red')
            except Exception:
                self.marker_annot = None
            self.canvas.draw_idle()
        except Exception:
            pass

    def make_equip_presets(self):
        # Minimal base set across typical slots (values are example starter items)
        presets = {
            'Chest': {'AC': 25, 'HP': 50, 'STR': 5, 'STA': 5},
            'Head': {'AC': 10, 'HP': 10, 'STR': 2, 'INT': 1},
            'Legs': {'AC': 20, 'HP': 30, 'STA': 3},
            'Arms': {'AC': 8, 'STR': 1, 'STA': 1},
            'Hands': {'AC': 6, 'STR': 1},
            'Feet': {'AC': 6, 'STA': 1},
            'Primary': {'Damage': 8, 'Attack': 10, 'STR': 2},
            'Secondary': {'Damage': 4, 'Attack': 5},
            'Range': {'Damage': 0, 'Attack': 0},
            'Wrist': {'AC': 1, 'STR': 0},
            'Back': {'AC': 2},
            'Neck': {},
            'Ear': {},
            'Ring': {},
            'Face': {}
        }
        return presets

    def get_item_from_inputs(self):
        base = {}
        for k, v in self.entries.items():
            base[k] = int(v.get())
        return base

    def reset_fields(self):
        for k, v in self.entries.items():
            v.set(0)
        self.level_var.set(100)
        self.output.delete('1.0', tk.END)

    def format_item(self, base, scaled):
        lines = []
        lines.append('Base:')
        for k in sorted(base.keys()):
            lines.append(f'  {k}: {base.get(k,0)}')
        lines.append('\nScaled:')
        for k in sorted(scaled.keys()):
            base_val = base.get(k, 0)
            scaled_val = scaled.get(k, 0)
            display, heroic = apply_stat_cap(scaled_val) if k in ['STR', 'STA', 'AGI', 'DEX', 'INT', 'WIS', 'CHA'] else (scaled_val, 0)
            if heroic > 0:
                lines.append(f'  {k}: {display} (+{heroic} heroic)')
            else:
                lines.append(f'  {k}: {display}')
        return '\n'.join(lines)

    def scale_item(self):
        try:
            level = int(self.level_var.get())
            base = self.get_item_from_inputs()
            slot = self.slot_var.get()
            slot_mult = self.slot_mults.get(slot, {})
            # Compute both non-slot and slot-scaled values
            scaled_no_slot = scaled_item(base, level, None, config=self.config)
            scaled = scaled_item(base, level, slot_mult, config=self.config)
            # apply attribute pool distribution
            mode = self.pool_mode_var.get()
            attrs = {k: base.get(k, 0) for k in ['STR', 'STA', 'AGI', 'DEX', 'INT', 'WIS', 'CHA']}
            scaled_attrs_no_slot = {k: scaled_no_slot.get(k, 0) for k in attrs}
            scaled_attrs_with_slot = {k: scaled.get(k, 0) for k in attrs}
            if mode == 'auto':
                # Apply presence bias to presence multipliers: scale the 'additional' portion toward 1.0
                pbias = float(self.presence_bias_var.get()) if hasattr(self, 'presence_bias_var') else 1.0
                eff_presence = {}
                for a, v in self.presence_mults.items():
                    eff_presence[a] = 1.0 + (v - 1.0) * pbias
                redistributed = pool_distribute_auto(attrs, scaled_attrs_no_slot, eff_presence, self.absence_mults, slot_mult=slot_mult.get('Attributes', 1.0))
            elif mode == 'static':
                redistributed = pool_distribute_static(attrs, int(self.static_budget_var.get()), self.presence_mults, self.absence_mults, slot_mult=slot_mult.get('Attributes', 1.0))
            else:
                # 'none' mode: keep attributes as slot-scaled values
                redistributed = scaled_attrs_with_slot

            # update scaled with redistributed attributes
            # Update final scaled dict: preserve non-attribute fields from slot-scaled value, and update attributes from redistributed
            for k in ['STR', 'STA', 'AGI', 'DEX', 'INT', 'WIS', 'CHA']:
                if k in redistributed:
                    # Prevent attributes from exceeding the slot-scaled attribute values
                    scaled[k] = min(redistributed[k], scaled.get(k, 0))
            # Apply global scale factor to final stats
            gscale = float(self.global_scale_var.get()) if hasattr(self, 'global_scale_var') else 1.0
            for s_key in ['AC','HP','Damage','Attack','Shielding','StrikeThrough']:
                if scaled.get(s_key) is not None:
                    scaled[s_key] = int(math.floor(scaled[s_key] * gscale))
            for k in ['STR', 'STA', 'AGI', 'DEX', 'INT', 'WIS', 'CHA']:
                scaled[k] = int(math.floor(scaled.get(k, 0) * gscale))
            # Derived stats (per-item)
            # Compute derived stats from final redistributed attributes
            if self.config:
                s_cfg = self.config.get('SpellDmgFromInt', {})
                if s_cfg.get('enabled'):
                    if s_cfg.get('mode', 'divisor') == 'divisor':
                        d = int(s_cfg.get('divisor', 10))
                        scaled['SpellDmg'] = scaled.get('INT', 0) // d
                    else:
                        points = s_cfg.get('curve', {}).get('points', [])
                        scaled_no_slot = scaled_item(base, level, None, config=self.config)
                        scaled = scaled_item(base, level, slot_mult, config=self.config)
                h_cfg = self.config.get('HealFromWis', {})
                if h_cfg.get('enabled'):
                    if h_cfg.get('mode', 'divisor') == 'divisor':
                        d = int(h_cfg.get('divisor', 10))
                        scaled['HealAmt'] = scaled.get('WIS', 0) // d
                    else:
                        points = h_cfg.get('curve', {}).get('points', [])
                        mult = evaluate_curve(points, level) if points else 1.0
                        scaled['HealAmt'] = int(math.floor((scaled.get('WIS', 0) / 10.0) * mult))

            # Derived stats
            if self.config:
                # SpellDmgFromInt
                s_cfg = self.config.get('SpellDmgFromInt', {})
                if s_cfg.get('enabled'):
                    mode = s_cfg.get('mode', 'divisor')
                    if mode == 'divisor':
                        d = int(s_cfg.get('divisor', 10))
                        scaled['SpellDmg'] = scaled.get('INT', 0) // d
                    else:
                        points = s_cfg.get('curve', {}).get('points', [])
                        mult = evaluate_curve(points, level) if points else 1.0
                        scaled['SpellDmg'] = int(math.floor((scaled.get('INT', 0) / 10.0) * mult))
                # HealFromWis
                h_cfg = self.config.get('HealFromWis', {})
                if h_cfg.get('enabled'):
                    mode = h_cfg.get('mode', 'divisor')
                    if mode == 'divisor':
                        d = int(h_cfg.get('divisor', 10))
                        scaled['HealAmt'] = scaled.get('WIS', 0) // d
                    else:
                        points = h_cfg.get('curve', {}).get('points', [])
                        mult = evaluate_curve(points, level) if points else 1.0
                        scaled['HealAmt'] = int(math.floor((scaled.get('WIS', 0) / 10.0) * mult))

            # Update before and after views
            try:
                self.before_text.delete('1.0', tk.END)
                self.before_text.insert(tk.END, self.format_item(base, scaled_no_slot))
            except Exception:
                pass
            # Update before and after views
            try:
                self.before_text.delete('1.0', tk.END)
                self.before_text.insert(tk.END, self.format_item(base, scaled_no_slot))
            except Exception:
                pass
            self.output.delete('1.0', tk.END)
            self.output.insert(tk.END, self.format_item(base, scaled))
            # Update plot marker based on last plotted curve
            if HAS_MATPLOTLIB:
                self.update_plot_marker()
        except Exception as e:
            messagebox.showerror('Error', str(e))

    def equip_all(self):
        level = int(self.level_var.get())
        items_scaled = []
        details = []
        for slot, base in self.equip_presets.items():
            slot_mult = self.slot_mults.get(slot, {})
            scaled_no_slot = scaled_item(base, level, None, config=self.config)
            scaled = scaled_item(base, level, slot_mult, config=self.config)
            mode = self.pool_mode_var.get()
            attrs = {k: base.get(k, 0) for k in ['STR', 'STA', 'AGI', 'DEX', 'INT', 'WIS', 'CHA']}
            scaled_attrs_no_slot = {k: scaled_no_slot.get(k, 0) for k in attrs}
            scaled_attrs_with_slot = {k: scaled.get(k, 0) for k in attrs}
            if mode == 'auto':
                redistributed = pool_distribute_auto(attrs, scaled_attrs_no_slot, self.presence_mults, self.absence_mults, slot_mult=slot_mult.get('Attributes', 1.0))
            elif mode == 'static':
                redistributed = pool_distribute_static(attrs, int(self.static_budget_var.get()), self.presence_mults, self.absence_mults, slot_mult=slot_mult.get('Attributes', 1.0))
            else:
                redistributed = scaled_attrs_with_slot
            for k in redistributed:
                # Apply non-attribute redistribution values and clamp attributes
                if k not in ['STR', 'STA', 'AGI', 'DEX', 'INT', 'WIS', 'CHA']:
                    scaled[k] = redistributed[k]
            # Clamp attribute values to slot-scaled computed values
            for k in ['STR', 'STA', 'AGI', 'DEX', 'INT', 'WIS', 'CHA']:
                if k in redistributed:
                    scaled[k] = min(redistributed[k], scaled.get(k, 0))
            # Derived stats
            if self.config:
                s_cfg = self.config.get('SpellDmgFromInt', {})
                if s_cfg.get('enabled'):
                    if s_cfg.get('mode', 'divisor') == 'divisor':
                        d = int(s_cfg.get('divisor', 10))
                        scaled['SpellDmg'] = scaled.get('INT', 0) // d
                    else:
                        points = s_cfg.get('curve', {}).get('points', [])
                        mult = evaluate_curve(points, level) if points else 1.0
                        scaled['SpellDmg'] = int(math.floor((scaled.get('INT', 0) / 10.0) * mult))
                h_cfg = self.config.get('HealFromWis', {})
                if h_cfg.get('enabled'):
                    if h_cfg.get('mode', 'divisor') == 'divisor':
                        d = int(h_cfg.get('divisor', 10))
                        scaled['HealAmt'] = scaled.get('WIS', 0) // d
                    else:
                        points = h_cfg.get('curve', {}).get('points', [])
                        mult = evaluate_curve(points, level) if points else 1.0
                        scaled['HealAmt'] = int(math.floor((scaled.get('WIS', 0) / 10.0) * mult))
            items_scaled.append(scaled)
            details.append(f'--- {slot} ---\n' + self.format_item(base, scaled))

        totals = sum_items(items_scaled)
        self.output.delete('1.0', tk.END)
        self.output.insert(tk.END, '\n\n'.join(details))
        # also show base presets in before_text
        try:
            before_lines = []
            for slot, base in self.equip_presets.items():
                s_no_slot = scaled_item(base, level, None, config=self.config)
                before_lines.append(f'--- {slot} Base ---\n' + self.format_item(base, s_no_slot))
            self.before_text.delete('1.0', tk.END)
            self.before_text.insert(tk.END, '\n\n'.join(before_lines))
        except Exception:
            pass
        self.output.insert(tk.END, '\n\n=== Totals ===\n')
        for k in sorted(totals.keys()):
            display, heroic = apply_stat_cap(totals[k]) if k in ['STR', 'STA', 'AGI', 'DEX', 'INT', 'WIS', 'CHA'] else (totals[k], 0)
            if heroic > 0:
                self.output.insert(tk.END, f'{k}: {display} (+{heroic} heroic)\n')
            else:
                self.output.insert(tk.END, f'{k}: {display}\n')

    def load_item_from_db_gui(self):
        try:
            item_id = int(self.item_id_var.get())
            if not HAS_MYSQL:
                messagebox.showerror('Missing dependency', "Please install 'mysql-connector-python' (pip install mysql-connector-python) to enable DB import")
                return
            loaded = self.fetch_item_from_db(item_id)
            if not loaded:
                messagebox.showinfo('Not Found', f'Item id {item_id} not found')
                return
            # fill inputs with loaded base values
            for k, v in loaded.items():
                if k in self.entries:
                    self.entries[k].set(int(v))
            messagebox.showinfo('Loaded', f"Loaded item {item_id} into base fields")
            # refresh preview
            if self.auto_update.get():
                self.scale_item()
        except Exception as e:
            messagebox.showerror('DB Error', str(e))

    def search_db_gui(self):
        try:
            if not HAS_MYSQL:
                messagebox.showerror('Missing dependency', "Please install 'mysql-connector-python' (pip install mysql-connector-python) to enable DB import")
                return
            term = self.search_name_var.get().strip()
            if not term or len(term) < 2:
                messagebox.showinfo('Search term', 'Please enter at least 2 characters to search')
                return
            results = self.search_items_in_db(term, limit=20)
            self.search_results_listbox.delete(0, tk.END)
            self.search_results_map = {}
            for i, r in enumerate(results):
                display = f"{r['id']}: {r['name']}"
                self.search_results_listbox.insert(tk.END, display)
                self.search_results_map[i] = r['id']
            if not results:
                messagebox.showinfo('No results', f'No items found matching "{term}"')
        except Exception as e:
            messagebox.showerror('DB Search Error', str(e))

    def load_selected_from_search(self, event=None):
        try:
            sel = self.search_results_listbox.curselection()
            if not sel:
                return
            idx = sel[0]
            if not hasattr(self, 'search_results_map'):
                return
            item_id = self.search_results_map.get(idx)
            if item_id:
                self.item_id_var.set(item_id)
                self.load_item_from_db_gui()
                if self.auto_update.get():
                    self.scale_item()
        except Exception as e:
            messagebox.showerror('DB Load Error', str(e))

    def search_items_in_db(self, term, limit=20):
        # Returns list of {'id': id, 'name': name}
        cfg_path = os.path.join(os.path.dirname(__file__), '..', 'eqemu_config.json')
        if not os.path.exists(cfg_path):
            raise RuntimeError('eqemu_config.json not found in project root')
        with open(cfg_path, 'r') as f:
            cfg = json.load(f)
        dbcfg = cfg.get('server', {}).get('database', {})
        if not dbcfg:
            raise RuntimeError('Database config not found in eqemu_config.json')
        host = dbcfg.get('host', '127.0.0.1')
        user = dbcfg.get('username', 'root')
        password = dbcfg.get('password', '')
        database = dbcfg.get('db', None)
        port = dbcfg.get('port', 3306)
        conn = None
        try:
            conn = mysql.connector.connect(host=host, user=user, password=password, database=database, port=port)
            cursor = conn.cursor(dictionary=True)
            q = "SELECT id, name FROM items WHERE name LIKE %s LIMIT %s"
            term_like = f"%{term}%"
            cursor.execute(q, (term_like, limit))
            rows = cursor.fetchall()
            cursor.close()
            conn.close()
            if not rows:
                return []
            # Map rows
            return [{'id': r.get('id'), 'name': r.get('name')} for r in rows]
        except Error as e:
            if conn:
                try:
                    conn.close()
                except Exception:
                    pass
            raise RuntimeError(str(e))

    def fetch_item_from_db(self, item_id):
        # Read eqemu_config.json for DB credentials
        cfg_path = os.path.join(os.path.dirname(__file__), '..', 'eqemu_config.json')
        if not os.path.exists(cfg_path):
            raise RuntimeError('eqemu_config.json not found in project root')
        with open(cfg_path, 'r') as f:
            cfg = json.load(f)
        dbcfg = cfg.get('server', {}).get('database', {})
        if not dbcfg:
            raise RuntimeError('Database config not found in eqemu_config.json')
        host = dbcfg.get('host', '127.0.0.1')
        user = dbcfg.get('username', 'root')
        password = dbcfg.get('password', '')
        database = dbcfg.get('db', None)
        port = dbcfg.get('port', 3306)

        try:
            conn = mysql.connector.connect(host=host, user=user, password=password, database=database, port=port)
            cursor = conn.cursor(dictionary=True)
            q = "SELECT id, name, ac, hp, damage, attack, astr, asta, aagi, adex, aint, awis, acha, fr, cr, mr, pr, dr, shielding, strikethrough, healamt, spelldmg FROM items WHERE id = %s"
            cursor.execute(q, (item_id,))
            row = cursor.fetchone()
            # If not found and item_id looks dynamic (>=1,000,000,000), try base id fallback
            if not row and item_id >= 1000000000:
                base_fallback = item_id % 1000000
                cursor.execute(q, (base_fallback,))
                row = cursor.fetchone()
            cursor.close()
            conn.close()
            if not row:
                return None
            # Map DB row to our base keys
            mapped = {
                'AC': row.get('ac', 0),
                'HP': row.get('hp', 0),
                'Damage': row.get('damage', 0),
                'Attack': row.get('attack', 0),
                'STR': row.get('astr', 0),
                'STA': row.get('asta', 0),
                'AGI': row.get('aagi', 0),
                'DEX': row.get('adex', 0),
                'INT': row.get('aint', 0),
                'WIS': row.get('awis', 0),
                'CHA': row.get('acha', 0),
                'FR': row.get('fr', 0),
                'CR': row.get('cr', 0),
                'MR': row.get('mr', 0),
                'PR': row.get('pr', 0),
                'DR': row.get('dr', 0),
                'Shielding': row.get('shielding', 0),
                'StrikeThrough': row.get('strikethrough', 0),
                'HealAmt': row.get('healamt', 0),
                'SpellDmg': row.get('spelldmg', 0)
            }
            return mapped
        except Error as e:
            raise RuntimeError(str(e))

    def export_clipboard(self):
        try:
            text = self.output.get('1.0', tk.END)
            self.clipboard_clear()
            self.clipboard_append(text)
            messagebox.showinfo('Copied', 'Scaled output copied to clipboard')
        except Exception as e:
            messagebox.showerror('Error', str(e))


if __name__ == '__main__':
    app = ItemScaleApp()
    app.mainloop()
