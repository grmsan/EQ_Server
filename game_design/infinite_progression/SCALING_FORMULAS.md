# Item Scaling Formulas

> Update (Dec 2025): Dynamic items now multiply the tiered baselines by JSON curves (Primary/Attribute/Weapon/Mod2) and use ratio-based weapon damage with a base+1 floor before applying `WeaponCurves` + slot multipliers. Attack also uses `WeaponCurves.Attack`. The tier math below describes the baseline increments; see the preview tool for live numbers with curves applied.

## Base Formula Structure

Scaling uses **tiered linear progression**:
- Increment increases every 10 levels
- Formula-based, easy to tune without database changes

```cpp
// Calculate increment based on level tier
int tier = floor(level / 10);
int increment = base_increment + (tier * tier_bonus);

// Apply scaling
scaled_stat = base_stat + (level * increment)
```

**Example Tiers:**
```
Levels 1-10:   +1 per level (tier 0)
Levels 11-20:  +2 per level (tier 1)
Levels 21-30:  +3 per level (tier 2)
Levels 31-40:  +4 per level (tier 3)
...
Levels 91-100: +10 per level (tier 9)
```

## Stat Cap & Heroic Overflow

**Critical**: EQ client has a **127-point cap** for base stats (STR, STA, AGI, etc.).

When a stat would exceed 127, overflow goes to heroic stat:
```cpp
if (scaled_stat > 127) {
  heroic_overflow = scaled_stat - 127;
  scaled_stat = 127;  // Cap at client limit
  heroic_stat += heroic_overflow;
}

Example:
  Formula calculates +132 STR
  Result: +127 STR, +5 Heroic STR
```

## Primary Stats (AC, HP, Mana)

### Armor Class (AC)
```cpp
// Tiered increment: +1 AC per level per tier
int tier = floor(level / 10);
int total_ac = base_ac;

// Sum all tiers up to current level
for (int i = 1; i <= level; i++) {
  int current_tier = floor((i - 1) / 10);
  total_ac += (current_tier + 1);  // Tier 0 = +1, Tier 1 = +2, etc.
}

// Simplified formula:
for (int t = 0; t < tier; t++) {
  total_ac += 10 * (t + 1);  // Full tier contribution
}
int remaining_levels = level % 10;
total_ac += remaining_levels * (tier + 1);  // Partial tier

Examples:
  Cloth Cap (base 3 AC):
    Level 1:   4 AC (+1)
    Level 10:  13 AC (+1*10 = +10 total)
    Level 20:  33 AC (+10 from tier 0, +20 from tier 1)
    Level 30:  63 AC (+10, +20, +30)
    Level 50:  153 AC (+10, +20, +30, +40, +50)
    Level 100: 553 AC (+10, +20, +30...+100)

  Plate Chestguard (base 25 AC):
    Level 10:  35 AC
    Level 50:  175 AC
    Level 100: 575 AC
```

### Hit Points (HP)
```cpp
// Tiered increment: starts at +2 HP, gains +2 per tier
int tier = floor(level / 10);
int total_hp = base_hp;

for (int t = 0; t < tier; t++) {
  total_hp += 10 * (2 + t * 2);  // Tier 0: +2, Tier 1: +4, Tier 2: +6, etc.
}
int remaining_levels = level % 10;
total_hp += remaining_levels * (2 + tier * 2);

Examples:
  Cloth Cap (base 0 HP):
    Level 1:   2 HP (+2*1)
    Level 10:  20 HP (+2*10)
    Level 20:  60 HP (+20 from tier 0, +40 from tier 1)
    Level 30:  120 HP (+20, +40, +60)
    Level 50:  300 HP (+20, +40, +60, +80, +100)
    Level 100: 1,100 HP (+20, +40, +60...+200)

  Dragon Scale Chestguard (base 50 HP):
    Level 10:  70 HP
    Level 50:  350 HP
    Level 100: 1,150 HP
```

### Mana
```cpp
// Tiered increment: starts at +1 Mana, gains +1 per tier
int tier = floor(level / 10);
int total_mana = base_mana;

for (int t = 0; t < tier; t++) {
  total_mana += 10 * (1 + t);  // Tier 0: +1, Tier 1: +2, Tier 2: +3, etc.
}
int remaining_levels = level % 10;
total_mana += remaining_levels * (1 + tier);

Examples:
  Cloth Cap (base 0 Mana):
    Level 1:   1 Mana
    Level 10:  10 Mana
    Level 20:  30 Mana (+10, +20)
    Level 50:  150 Mana (+10, +20, +30, +40, +50)
    Level 100: 550 Mana (+10, +20, +30...+100)
```

## Attribute Stats (STR, STA, etc.)

### ✅ IMPLEMENTED: Universal Stat Scaling
**ALL items gain ALL stats**, regardless of base values:
```cpp
// Tiered increment: starts at +1, gains +1 per tier
// NOTE: No longer checks if (base_stat > 0)
int tier = floor(level / 10);
int raw_stat = base_stat;  // Can be 0!

for (int t = 0; t < tier; t++) {
  raw_stat += 10 * (1 + t);  // Tier 0: +1, Tier 1: +2, etc.
}
int remaining_levels = level % 10;
raw_stat += remaining_levels * (1 + tier);

// Apply 127 cap with heroic overflow
if (raw_stat > 127) {
  base_stat_display = 127;
  heroic_stat = raw_stat - 127;
} else {
  base_stat_display = raw_stat;
  heroic_stat = 0;
}

Example - Banded Mail (base +5 STR):
  Level 1:   +6 STR
  Level 10:  +15 STR (+5 base + 10 from tier 0)
  Level 20:  +35 STR (+5 + 10 tier 0 + 20 tier 1)
  Level 50:  +127 STR, +28 Heroic STR (155 total)
  Level 100: +127 STR, +428 Heroic STR (555 total)
  Level 200: +127 STR, +2,028 Heroic STR (2,155 total)

Example - Short Sword (base 0 STR):
  Level 1:   +1 STR   (0 base + 1 from tier 0)
  Level 10:  +10 STR  (0 + 10 from tier 0)
  Level 20:  +30 STR  (0 + 10 tier 0 + 20 tier 1)
  Level 50:  +127 STR, +23 Heroic STR (150 total)
  Level 100: +127 STR, +423 Heroic STR (550 total)
```
**Current tuning (live JSON, Dec 2025):**
- `AttributeCurve` is gentle: 1.0 @1, ~1.2 @50, 1.3 @100, 1.45 @150, 1.55 @200, 1.7 @300.
- Presence/absence weighting: 0.97 if the stat exists on the base item, 0.90 if it does not.
- Base emphasis: attributes get a bonus factor of `1 + (base_stat / 22.0)`, so items that already have a stat stay meaningfully ahead.
- Slot multiplier still applies to the overall pool (e.g., Head 1.25×, Chest 1.5×).
- Distribution uses the weighted pool directly (no base value added afterward), mirroring server code.

### ⏳ TODO: Random Stats (Framework Exists)
Planned for milestone-based stat additions:
```cpp
// Added at level milestones (every 5 levels)
// If item has < 8 different stats:
if (level % 5 == 0 && num_stats < 8) {
  if (rand() < 0.5) {  // 50% chance
    // Start with tiered value at current level
    int tier = floor(level / 10);
    new_stat = floor(level / 5) + tier;
  }
}

// NOTE: Currently disabled - all items get all stats instead
```

## Heroic Stats

Heroic stats come from **two sources**:

### 1. Overflow from Base Stat Cap (127)
When base stats exceed 127, overflow automatically goes to heroic:
```cpp
if (base_stat > 127) {
  heroic_stat += (base_stat - 127)
  base_stat = 127
}
```

### 2. Milestone Unlocks (Level 50+)
Additional heroic stats added at milestones:
```cpp
// These are BONUS heroics, separate from overflow
if (level >= 50) {
  milestone_heroic = floor((level - 50) / 5)
}

Examples:
  Level 50:  +0 Milestone Heroic (just unlocked)
  Level 60:  +2 Milestone Heroic
  Level 100: +10 Milestone Heroic
  Level 200: +30 Milestone Heroic
```

### Combined Example
```
Banded Mail +100 (base +5 STR):
  Tiered Formula: 5 + (10*1) + (10*2) + ... + (10*10) = 555 STR

  Base STR: 127 (capped)
  Heroic STR from overflow: 428
  Heroic STR from milestone: +10 (level 100 milestone)
  Total Heroic STR: 438
```

## Secondary Stats

### Weapon Damage
```cpp
if (item_is_weapon && base_damage > 0) {
  // ✅ IMPLEMENTED - Tiered increment: starts at +4, gains +4 per tier
  int tier = floor(level / 10);
  int increment = 4 + (tier * 4);  // Base: +4, Tier bonus: +4

  int total_damage = base_damage + (level * increment);
}

Example - Short Sword (base 4 damage):
  Level 1:   8 damage   (+4*1 = +4)
  Level 10:  44 damage  (+4*10 = +40)
  Level 50:  404 damage (+4 tier 0-4 = +400)
  Level 100: 1,604 damage
```

### Attack Rating
```cpp
if (item_is_weapon) {
  // ✅ IMPLEMENTED - Tiered increment: starts at +2, gains +2 per tier
  int tier = floor(level / 10);
  int total_attack = base_attack;

  for (int t = 0; t < tier; t++) {
    total_attack += 10 * (2 + t * 2);  // Tier 0: +2, Tier 1: +4, etc.
  }
  int remaining_levels = level % 10;
  total_attack += remaining_levels * (2 + tier * 2);
}

Example - Short Sword (base 10 attack):
  Level 1:   12 attack
  Level 10:  30 attack (+20 from tier 0)
  Level 20:  70 attack (+20, +40)
  Level 50:  310 attack (+20, +40, +60, +80, +100)
  Level 100: 1,110 attack (sum of tiers)
```

### ⏳ TODO: Haste (Formula Exists, Commented Out)
```cpp
// ⏳ TODO: Enable haste scaling (lines 305-327 in dynamic_item_manager.cpp)
// Haste unlocks at 25, scales with tiers, caps at 100%
if (level >= 25) {
  if (level <= 50) {
    // Slow growth to 50
    haste = floor((level - 25) / 5)  // +1% per 5 levels
  } else {
    // Faster growth after 50
    haste = 5 + floor((level - 50) / 2)  // +1% per 2 levels
  }

  haste = min(haste, 100)  // Cap at 100%
}

Planned Examples:
  Level 25:  0% haste (just unlocked)
  Level 30:  1% haste
  Level 50:  5% haste
  Level 100: 30% haste
  Level 200: 80% haste
  Level 250+: 100% haste (capped)

// NOTE: Code exists but is commented out - needs client testing
```

### Regeneration (HP/Mana/Endurance)
```cpp
if (level >= 50) {
  // Linear growth
  regen = floor((level - 50) / 10)  // +1 per 10 levels
}

Examples:
  Level 50:  0 regen
  Level 60:  +1 regen
  Level 100: +5 regen
  Level 200: +15 regen
  Level 500: +45 regen
```

## Resistances

Added as random stats at milestones, then scale with tiered increments:
```cpp
// 20% chance each to add a resist at level 10, 20, 30, etc.
if (level % 10 == 0 && rand() < 0.2) {
  resist_type = random(FR, CR, MR, DR, PR)

  // Use tiered formula (same as stats)
  int tier = floor(level / 10);
  int resist_value = 0;
  for (int t = 0; t < tier; t++) {
    resist_value += 10 * (1 + t);
  }

  resist_value = min(resist_value, 127);  // Cap at 127
}

Example progression:
  Level 10: +10 Fire Resist (random roll)
  Level 20: +30 Fire Resist, +30 Magic Resist (new resist)
  Level 50: +127 Fire, +127 Magic (capped)
  Level 100: +127 Fire, +127 Magic, +127 Cold (all capped)

Note: Resists cap at 127 in the client

### Item scaling JSON configuration
Configured curves for attribute-derived stats (SpellDmg/Heal) can be controlled via `game_design/infinite_progression/item_scaling.json`.
This JSON allows selecting a simple divisor approach (e.g., 1 per 10 attribute points) or a piecewise linear curve mapping a level to a per-10-attribute derived value.
By default the system preserves current behavior (divisor=10) but supports custom curves for early rapid growth followed by tapering.

**JSON keys of interest:**
- `SpellDmgFromInt`: Controls how SpellDamage is computed from item INT; fields: `enabled`, `mode` (`divisor` or `curve`), `divisor` (int), and `curve` (points array).
- `HealFromWis`: Similar configuration for HealAmt derived from WIS.
- `AttributeCurve`: Global per-level multiplier applied to raw attribute growth.
- `AttributePreferences`: `presence_mult` and `absence_mult` for each attribute; optional per-attribute `curve`.
- `AttributePool`: `mode` (`auto`, `static`, `none`) and `static_budget`.
 - `SlotMultipliers`: object mapping slot category names to multipliers (e.g., `Chest:1.5`, `Wrist:0.75`). These multipliers are applied to the attribute pool for items that can be equipped in the given slot, increasing or decreasing total attribute budget at generation time.
- `WeaponCurves`, `Mod2Curves`, `Mod2CurvesExtras`, and `PrimaryCurves` for weapon damage, attack, mod2 and primary stat per-level modifiers.

Example: If `SpellDmgFromInt.mode` is set to `divisor` with `divisor = 8`, then each 8 INT generates `+1 SpellDmg`.
If instead `mode = curve` with points `[[1,0.8], [100, 8.0]]`, EvaluateCurve(level) multiplies per-10 attribute points by that value.

### Attribute Preferences and Class Multipliers
The JSON supports `AttributePreferences` which defines two multipliers per attribute: `presence_mult` and `absence_mult`.
When an item starts with a base value for the attribute, `presence_mult` is applied to the scaled value (so base-focused items grow stronger in their primary stats).
If the base attribute is absent, `absence_mult` is applied to the scaled value to keep a modest amount of growth on filler stats.

`ClassMultipliers` supports optional per-class adjustments. For example, a class-specific multiplier can increase `SpellDmg` for `Wizard` while leaving it lower for melee classes.
These multipliers are applied when the Mob equips the item (not in the base item data) so the item remains a single canonical definition for all players.

Examples can be configured in `item_scaling.json` with section `AttributePreferences` and `AttributeCurve`.
Note: Equip-time class multipliers have been removed; class-specific power should be implemented via AAs if desired. The `ClassMultipliers` section is deprecated.
### Pool Distribution vs Static Budget (Examples)
Pool distribution spreads attribute growth across attributes based on base stat weights (presence vs absence multipliers) and a total budget derived from scaled attribute gains (Option A - Auto) or a static budget provided by the administrator (Option B - Static).

Example input items at Level 100

- Helm1: AC=2 (no other stats)
- Helm2: AC=90, HP=300, STR=30, STA=30, AGI=30, DEX=30, INT=30, WIS=30, CHA=30, resists=20 each, Shielding=5, StrikeThrough=20

We used a small preview script `tools/item_scale_preview.py` to simulate both methods with default presence/absence multipliers.

Results (script output):

Helm1 [base AC=2]
- Option A (Auto):    minimal attribute budget — AC grows, but attributes remain unchanged
- Option B (Static=100): attributes get distributed from static pool: DEX/STR/STA/AGI/INT/WIS/CHA ~ 7 each; AC=37

Helm2 [base AC=90, HP=300, STR=30...]
- Option A (Auto): larger attribute budget computed from scaled values: attributes distributed proportionally to presence: INT/WIS favored due to higher multipliers — final values roughly STR=49, STA=47, AGI=48, DEX=50, INT=WIS=~55, CHA=46, AC=138, HP=461, Shielding=7, StrikeThrough=30
- Option B (Static=100): static budget allocation produces smaller attribute increases across the board, lower AC/HP increases compared to auto approach

The script `tools/item_scale_preview.py` reproduces these values and demonstrates how weights shift items toward attributes they already contain.

### JSON Configuration: AttributePool
Add an `AttributePool` object to `item_scaling.json` to switch modes and set a static budget:
```json
"AttributePool": {
  "mode": "auto", // auto | static | none
  "static_budget": 10
}
```

- `mode`:
  - `auto`: total pool is the sum of per-attribute scaled increases (default). Weights derived from AttributePreferences determine distribution.
  - `static`: total pool is the `static_budget` integer value; this budget is distributed using AttributePreferences as weights.
  - `none`: legacy behavior; each attribute is multiplied by presence/absence multipliers.
### Design Considerations & Pacing Guidance

This system contains many moving parts. The following design tips help make tuning consistent and give players a satisfying growth rate:

- **Control when stats hit cap:** Because base formulas and `AttributeCurve` multiply aggressively, attributes can reach the client cap of 127 quickly. To avoid capping too early, lower the `AttributeCurve` early (levels 1-40) and let it grow later. Example: at level 40 keep curve near 2.0, and at level 100 near 10.0 (default sample). You can also use per-attribute curves to keep some stats lower or higher.
- **Set a pacing target:** Define expected average dynamic item level by player level for each content tier (zone). e.g. Recommended target: average dynamic item level of ~60 by player level 40 for mid-game; this can be achieved by tuning `AttributeCurve` and `WeaponCurves` or using `static` budget set to `floor(player_level * 1.5)` in drop logic.
  **How to implement the target:**
  1. Adjust `AttributeCurve` such that early-level values are lower and ramp up later.
  2. Alternatively use `AttributePool.mode=static` and set `static_budget` dynamically in your drop logic or zone spawn script (for instance `static_budget = floor(player_level * 1.5)` or `= level * 2`) to meet a desired per-level increase.
  3. Finally, tune the `WeaponCurves` and `PrimaryCurves` to adjust weapon and core stat growth separately (this helps keep weapons from scaling too fast compared with primary attribute growth).
- **Use `static` budgets to normalize gear:** If items with large base values unintentionally generate large `auto` pools (making them dominate progression), use a static budget to provide parity between item types (e.g., jewelry vs armor).
- **Bias growth with `AttributePreferences`:** Use presence vs absence multipliers to keep items aimed toward their intended role; ARMs and weapons should favor relevant stats and avoid turning a sword into a spellcaster weapon via raw attribute allocation.
- **Derived caster stat tuning:** Choose `divisor` or `curve` modes for `SpellDmgFromInt` / `HealFromWis` to achieve consistent caster power progression and avoid rapid inflation in spell damage for mixed-stat items.
- **Avoid equip-time class multipliers:** They were removed. If class flavoring is desired, implement AAs or per-class bonuses applied in player-specific code rather than changing item canonical base definitions.

These guardrails help keep player growth balanced while still allowing players to experience 'power spikes' through well-designed content (e.g., dragon helm fusion).

### Example default tuning (Quick start)
Below are starting values to use as a baseline while you tune content and observe telemetry:

- `AttributePool.mode`: `auto` — begin with auto so items with more base stats have larger budgets.
- `AttributePool.static_budget`: `10` — only used if you switch to `static` mode.
- `AttributeCurve` sample: `[[1,1.0],[20,1.2],[40,2.0],[80,6.0],[100,10.0]]` — gentle early growth, faster later.
- `WeaponCurves.Damage`: `[[1,1.0],[50,4.0],[100,8.0]]` — use weapon curves to adjust damage vs attributes.
- `SpellDmgFromInt.mode`: `divisor` with `divisor=10` for simple parity, or `mode=curve` for more control.
- `AttributePreferences` default `presence_mult=1.25` and `absence_mult=0.5` — encourages growth in existing attributes and lesser growth on filler stats.

Use these sample values as a baseline and test with the `#tune itemscale preview` command and the Python preview tool. Iterate on `AttributeCurve` and `WeaponCurves` to reach a pacing target you like (e.g., average item level ≈ 60 by player level 40).

- `static_budget`: integer budget to allocate across primary attributes when `mode` is `static`.

### Gaps & Proposed Enhancements
The following items are proposed changes or gaps to address during tuning and future development:

- **Zone-based drop scaling & pacing telemetry**: Establish configuration in zone spawn scripts so average item level tracks player progression reliably (e.g., ensure mid-tier zones reward item levels appropriate to player level). Add analytics logging (fusion_log, level progression metrics) to measure actual player averages and tune curves accordingly.
- **Per-slot budgets and caps**: Consider `static_budget` per-slot (weapons vs armor vs rings) to better balance pocket items vs main-hand weapons.
- **Normalization for 'auto' budgets**: Provide clamping to prevent an item with high base stats from producing runaway `auto` pools. A `max_pool_multiplier` or `slot-based clamping` could keep it fair.
- **SpellDmg/Heal parity**: Ensure derived SpellDmg/Heal curves don't allow mixed-stat melee items to overshadow hybrid or pure caster designs; per-class adjustments via AAs or per-class curves may be better than introducing class multipliers.
- **Default pacing benchmark**: Add recommended default curves in the sample JSON and record this in analytics until validated with player testing.




```

## ⏳ TODO: Focus Effects (Milestones Defined)

### Minor Focus (Level 100+) - NOT YET IMPLEMENTED
```cpp
// ⏳ TODO: Implement AddFocusEffect() method
if (level >= 100 && level < 200) {
  add_focus_effect(MINOR_TIER)  // Needs implementation
}

Planned Examples:
  - Improved Damage I (+5% spell damage)
  - Improved Healing I (+5% heal amount)
  - Spell Haste I (-5% cast time)
```

### Major Focus (Level 200+) - NOT YET IMPLEMENTED
```cpp
if (level >= 200 && level < 500) {
  upgrade_focus_effect(MAJOR_TIER)  // Needs implementation
}

Planned Examples:
  - Improved Damage III (+15% spell damage)
  - Improved Healing III (+15% heal amount)
  - Spell Haste III (-15% cast time)
```

### Epic Focus (Level 500+) - NOT YET IMPLEMENTED
```cpp
if (level >= 500) {
  add_second_focus_effect(EPIC_TIER)  // Needs implementation
}

Planned Examples:
  - Improved Damage V (+25% spell damage)
  - Mana Preservation IV (25% chance no mana cost)
  - Multiple simultaneous focus effects

// NOTE: Milestone thresholds defined in config, needs spell database entries
```

## Procs and Clicks

### Procs (Level 500+)
```cpp
if (item_is_weapon && level >= 500) {
  proc_damage = 50 + floor((level - 500) / 50)
  proc_rate = min(floor((level - 500) / 100), 100)  // 1% per 100 levels
}

Example - Level 500 weapon:
  Fire Strike: 50 damage, 1% proc rate

Example - Level 1000 weapon:
  Fire Strike: 60 damage, 5% proc rate
```

### Click Effects (Level 1000+)
```cpp
if (level >= 1000) {
  add_click_effect(utility_type)
}

Examples:
  - Gate (instant teleport to bind)
  - Levitate
  - Invisibility
  - Damage Shield
```

## Milestone Summary Table

| Level | Milestone Bonus |
|-------|----------------|
| 5     | 50% chance new random stat |
| 10    | Guaranteed new stat OR boost existing |
| 15    | 50% chance new random stat |
| 20    | Guaranteed new stat + 20% chance resist |
| 25    | Unlock Haste (0%) |
| 30    | 20% chance resist |
| 50    | Unlock Heroic stats |
| 75    | +1 Regen |
| 100   | Add Minor Focus Effect |
| 200   | Upgrade to Major Focus |
| 500   | Add Proc (weapons) |
| 1000  | Add Click Effect |
| 2000  | Double all bonuses |

## Formula Tuning Variables

These can be adjusted for balance:

```cpp
// In dynamic_item_manager.h
struct ScalingConfig {
  // Tiered scaling - increment increases per tier
  int tier_size = 10;                // Levels per tier

  // ✅ IMPLEMENTED - Base increments (tier 0, levels 1-10)
  int ac_base_increment = 1;         // +1 AC per level
  int hp_base_increment = 4;         // +4 HP per level (updated from 2)
  int mana_base_increment = 1;       // +1 Mana per level
  int stat_base_increment = 1;       // +1 stat per level
  int attack_base_increment = 2;     // +2 attack per level
  int damage_base_increment = 4;     // +4 damage per level (NEW)

  // ✅ IMPLEMENTED - Tier bonuses (added per tier)
  int ac_tier_bonus = 1;             // Each tier adds +1 to increment
  int hp_tier_bonus = 4;             // Each tier adds +4 to increment (updated from 2)
  int mana_tier_bonus = 1;           // Each tier adds +1 to increment
  int stat_tier_bonus = 1;           // Each tier adds +1 to increment
  int attack_tier_bonus = 2;         // Each tier adds +2 to increment
  int damage_tier_bonus = 4;         // Each tier adds +4 to increment (NEW)

  // ✅ IMPLEMENTED - Client limits
  int base_stat_cap = 127;           // EQ client hard cap
  int resist_cap = 127;              // Resist cap
  int haste_cap = 100;               // Haste cap (%)

  // ✅ IMPLEMENTED - Heroic milestones
  int heroic_start_level = 50;
  int heroic_per_levels = 5;         // +1 heroic per 5 levels

  // ⏳ TODO - Haste progression (commented out)
  int haste_start_level = 25;
  int haste_slow_divisor = 5;        // Levels 25-50
  int haste_fast_divisor = 2;        // Levels 50+

  // ⏳ TODO - Focus effects (milestones defined, not implemented)
  int focus_minor_level = 100;
  int focus_major_level = 200;
  int focus_epic_level = 500;

  // ⏳ TODO - Random stats (framework exists, disabled for universal scaling)
  float new_stat_chance = 0.5f;      // 50%
  int new_stat_interval = 5;         // Every 5 levels
  int max_random_stats = 8;          // Limit to 8 different stats
};
```

## Example Complete Item Progression

**Cloth Cap (Base: 3 AC, no stats)**

```
Level 0:   3 AC

Level 1:   4 AC, 2 HP

Level 5:   8 AC, 10 HP, +1 STR (random)

Level 10:  13 AC, 20 HP, +3 STR, +10 Fire Resist (milestone)

Level 15:  21 AC, 40 HP, +8 STR, +17 Fire, +2 WIS (random)

Level 20:  33 AC, 60 HP, +13 STR, +30 Fire, +4 WIS, +1 STA (milestone)

Level 30:  63 AC, 120 HP, +28 STR, +60 Fire, +15 WIS, +8 STA, 1% Haste

Level 50:  153 AC, 300 HP, +78 STR, +127 Fire (capped), +43 WIS, +28 STA, 5% Haste

Level 100: 553 AC, 1,100 HP, +127 All Stats (capped), 30% Haste, Minor Focus
           +226 H.STR, +71 H.WIS, +28 H.STA, +10 Milestone Heroics

Level 200: 2,053 AC, 4,100 HP, +127 All Stats, 80% Haste, Major Focus
           +1,826 H.STR, +1,671 H.WIS, +1,628 H.STA, +30 Milestone Heroics

Level 500: 12,553 AC, 25,100 HP, +127 All Stats, 100% Haste, Epic Focus, Fire Proc
           +26,826 H.STR, +26,671 H.WIS, +26,628 H.STA, +90 Milestone Heroics
```

**Key Observations:**
- Much more reasonable scaling than exponential
- Stats hit 127 cap around level 50-100 (depending on base)
- Level 500 items are powerful but not billions of HP
- Tiered system is easy to tune (change tier bonuses)

## See Also
- [OVERVIEW.md](OVERVIEW.md) - System overview
- [FUSION_SYSTEM.md](FUSION_SYSTEM.md) - Item fusion mechanics
- [IMPLEMENTATION.md](IMPLEMENTATION.md) - Code implementation details
