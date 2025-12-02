# Item Scaling Formulas

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

### Existing Stats (base > 0)
```cpp
// Tiered increment: starts at +1, gains +1 per tier
int tier = floor(level / 10);
int raw_stat = base_stat;

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
```

### New Stats (added during progression)
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

Example - Cloth Cap (no base STR):
  Level 5:  +1 STR (random roll succeeded)
  Level 10: +3 STR (+1, +1, +1, +1, +1 from levels 6-10)
  Level 15: +8 STR, +2 WIS (new random stat added)
  Level 20: +13 STR, +4 WIS
  Level 50: +78 STR, +43 WIS
  Level 100: +127 STR, +127 WIS, +226 H.STR, +71 H.WIS
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

### Attack
```cpp
if (item_is_weapon) {
  // Tiered increment: starts at +2, gains +2 per tier
  int tier = floor(level / 10);
  int total_attack = base_attack;

  for (int t = 0; t < tier; t++) {
    total_attack += 10 * (2 + t * 2);  // Tier 0: +2, Tier 1: +4, etc.
  }
  int remaining_levels = level % 10;
  total_attack += remaining_levels * (2 + tier * 2);
}

Example - Rusty Dagger (base 10 attack):
  Level 1:   12 attack
  Level 10:  30 attack (+20 from tier 0)
  Level 20:  70 attack (+20, +40)
  Level 50:  310 attack (+20, +40, +60, +80, +100)
  Level 100: 1,110 attack (sum of tiers)
```

### Haste
```cpp
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

Examples:
  Level 25:  0% haste (just unlocked)
  Level 30:  1% haste
  Level 50:  5% haste
  Level 100: 30% haste
  Level 200: 80% haste
  Level 250+: 100% haste (capped)
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
```

## Focus Effects

### Minor Focus (Level 100+)
```cpp
if (level >= 100 && level < 200) {
  add_focus_effect(MINOR_TIER)
}

Examples:
  - Improved Damage I (+5% spell damage)
  - Improved Healing I (+5% heal amount)
  - Spell Haste I (-5% cast time)
```

### Major Focus (Level 200+)
```cpp
if (level >= 200 && level < 500) {
  upgrade_focus_effect(MAJOR_TIER)
}

Examples:
  - Improved Damage III (+15% spell damage)
  - Improved Healing III (+15% heal amount)
  - Spell Haste III (-15% cast time)
```

### Epic Focus (Level 500+)
```cpp
if (level >= 500) {
  add_second_focus_effect(EPIC_TIER)
}

Examples:
  - Improved Damage V (+25% spell damage)
  - Mana Preservation IV (25% chance no mana cost)
  - Multiple simultaneous focus effects
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

  // Base increments (tier 0, levels 1-10)
  int ac_base_increment = 1;         // +1 AC per level
  int hp_base_increment = 2;         // +2 HP per level
  int mana_base_increment = 1;       // +1 Mana per level
  int stat_base_increment = 1;       // +1 stat per level
  int attack_base_increment = 2;     // +2 attack per level

  // Tier bonuses (added per tier)
  int ac_tier_bonus = 1;             // Each tier adds +1 to increment
  int hp_tier_bonus = 2;             // Each tier adds +2 to increment
  int mana_tier_bonus = 1;           // Each tier adds +1 to increment
  int stat_tier_bonus = 1;           // Each tier adds +1 to increment
  int attack_tier_bonus = 2;         // Each tier adds +2 to increment

  // Client limits
  int base_stat_cap = 127;           // EQ client hard cap
  int resist_cap = 127;              // Resist cap
  int haste_cap = 100;               // Haste cap (%)

  // Heroic milestones
  int heroic_start_level = 50;
  int heroic_per_levels = 5;         // +1 heroic per 5 levels

  // Haste progression
  int haste_start_level = 25;
  int haste_slow_divisor = 5;        // Levels 25-50
  int haste_fast_divisor = 2;        // Levels 50+

  // Focus effects
  int focus_minor_level = 100;
  int focus_major_level = 200;
  int focus_epic_level = 500;

  // Random stats
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
