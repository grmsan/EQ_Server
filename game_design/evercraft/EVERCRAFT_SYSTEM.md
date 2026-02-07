# EveCraft: Dynamic Infinite Crafting System

## Overview

**EveCraft** is a revolutionary crafting system inspired by "Infinite Craft" that allows players to combine **any item with any item** to generate new, contextually appropriate results. Unlike the traditional EQEmu recipe system where combinations are pre-defined in a database table, EveCraft dynamically generates outcomes based on:

- **Item type combinations** (e.g., weapon + armor = reinforced item)
- **Rarity and stats** (item quality influences result quality)
- **Character crafting skill level** (higher skill = better results, rare variants)
- **Contextual logic** (a knife + fur logically creates leather wrappings, not a helmet)

This creates **infinite crafting possibilities** where players discover new combinations, with results that *feel* correct despite never being pre-defined.

---

## Core Design Principles

### 1. **Deterministic Generation**

Every combination of two items produces the **same result** given the same crafting skill level. This ensures:
- Players can replicate successful combinations
- Results are predictable and repeatable
- No "random" feel that breaks immersion

**Implementation:** Hash the two item IDs + skill level to seed a deterministic outcome engine.

### 2. **Skill-Based Quality Scaling**

Crafting skill (0–300) directly affects:
- **Success rate** (low skill = more failures)
- **Result quality** (low skill = trash variants; high skill = prized items)
- **Stat variance** (low skill = lower stats; high skill = stat bonuses)
- **Unique variants** (at skill 200+, chance for "Mastercrafted" variant names)

Example:
```
Knife + Wolf Skin (at skill 1):
  "Roughly Wrapped Wolf Fur"  (50% success, 50% attack, 50% AC)

Knife + Wolf Skin (at skill 100):
  "Leather Wrapped Blade"  (90% success, 75% attack, 75% AC)

Knife + Wolf Skin (at skill 250):
  "Masterwork Wolf-Skin Blade"  (99% success, 110% attack, 110% AC)
```

### 3. **Contextual Combination Logic**

Results must "make sense." The system uses item **categories** and **types** to determine valid combinations:

**Valid combinations:**
- Weapon + Armor → Reinforced/Enhanced variant
- Armor + Armor (same slot) → Upgraded armor
- Two different rare items → Hybrid item with combined effects
- Weapon + Material → Enhanced weapon
- Food + Food → Soup/Stew/Meal

**Invalid combinations:**
- Currency + Anything → Rejected
- Quest item + Most things → Rejected
- Lore items with conflicting lore → Rejected

### 4. **First-Time Discovery**

When a player first combines two items that haven't been combined before:

1. **Item lookup** occurs - the system searches the database for an existing item that matches the combination profile
2. If found → Use that existing item as the result
3. If not found → **Create a dynamic item** with generated stats based on both inputs
4. **Cache the result** (via data bucket) so future attempts produce the same item

Example:
```
First combine:  Helm of Dragonslaying + Gloves of a Slain God
  → Lookup for "Helm of Dragon God" (not found)
  → Generate new dynamic item with stats averaging both inputs
  → Cache mapping: (helm_id=2345 + gloves_id=5678) → new_result_id

Next combine: Helm of Dragonslaying + Gloves of a Slain God
  → Lookup cache, find cached result
  → Return same item ID (deterministic)
```

### 5. **Dynamic Item Generation**

When no existing item matches, generate one with:

- **Name:** Contextual blend of input items (e.g., "Gloves of Dragon Protection")
- **Stats:** Average or combine relevant stats from inputs
  - Attack from weapon → carried to result if weapon-like
  - AC from armor → carried to result if armor-like
  - Resists from both → averaged
  - HP/Mana from both → averaged
- **Restrictions:** Inherit stricter class/race restrictions from inputs
- **Durability:** Lore group stays consistent (prevent lore conflicts)
- **Weight:** Average of input weights
- **Slot:** Determined by dominant item type

### 6. **Failure and Experimentation**

Low skill levels result in **failures:**

- **Salvage system:** Some recipes salvage components (get one item back)
- **Failure variants:** At low skill, results may be "flawed," "cracked," or "crude" (lower stats)
- **Destruction:** At very low skill, combine can fail completely (lose items)

Skill increase follows the existing EQEmu system:
- Small chance to increase skill per successful combine
- Modifier based on trivial skill level (skill/trivial ratio)
- Stat bonuses (INT/WIS) improve skill gain chance

---

## Game Balance Considerations

### Progression

**Early Game (Skill 0-50):**
- 40% success rate
- Failure: lose 1 item, salvage other
- Results: basic variants only
- Use case: leveling crafting from newbie level

**Mid Game (Skill 50-150):**
- 70% success rate
- Failure: keep 50% of one item or salvage
- Results: standard variants
- Use case: mainstream progression

**End Game (Skill 150-250):**
- 90% success rate
- Failure: keep items, no salvage
- Results: enhanced variants with stat bonuses
- Use case: min/max character gear

**Master Crafting (Skill 250-300):**
- 98%+ success rate
- Failure: rare, no loss
- Results: rare "Mastercrafted" variants with unique properties
- Use case: aspirational endgame content

### Stat Scaling

**Formula for average result stats:**
```
result_stat = (item1_stat + item2_stat) / 2 * skill_modifier

skill_modifier = 1.0 + (skill - 100) * 0.01
  (at skill 100 = 1x; at skill 200 = 2x multiplier on bonuses)
```

**Caps:**
- Result stats cannot exceed the highest input stat by more than 25%
- Prevents unlimited stat stacking
- Encourages using balanced inputs

### Unique Item Variants (Skill 250+)

At high skill, each successful combine has a **5-10% chance** to generate a unique variant:

```
Regular result:  "Masterwork Wolf-Skin Blade"
Unique variant:  "Legendary Blade of the Moonlit Hunt"
  (Same stats, but unique name; counts as different item for lore purposes)
```

---

## Prohibited and Restricted Combinations

### Never Allow:
- Currency items + anything (would break economy)
- Quest flags + most combinations (prevents quest breaking)
- Conflicting lore items (prevent lore violations)
- No-drop + tradeable items (creates no-drop tradeable item)
- Attuned items + anything (preserve bind-on-pickup semantics)

### Warn Player But Allow:
- Expensive items (warn cost value)
- Limited quantity items (warn scarcity)
- Items with charges (charge loss on combine)

---

## Player Experience

### Discovery and Learning

Players naturally learn:
- **What combinations work** through experimentation
- **Contextual logic** (a weapon + tool = reinforced tool, not a weapon)
- **Skill scaling** (why mastery crafts are better)

### Optional Recipes

Guilds/NPCs might offer "hints":
```
NPC: "I've heard that combining a blade with leather creates something special..."
```

This is purely optional flavor—no recipe database needed.

### Failure Feedback

```
Combine Helm + Gloves (50% success, skill 25):
  ❌ Failed!  Gloves cracked and were destroyed.
  (Helm remains in container)
```

---

## Design Philosophy Summary

| Aspect | Traditional EQ | EveCraft |
|--------|---|---|
| **Recipes** | Hardcoded database | Contextual + dynamic |
| **Discovery** | Look up recipes online | Experimental exploration |
| **Skill Impact** | Binary (skill needed or not) | Continuous scaling (quality improves) |
| **Items** | Pre-existing only | Pre-existing OR dynamically created |
| **Combinations** | Thousands of records | Infinite potential |
| **Player Agency** | Follow recipe | Experimentation driven |

---

## Technical Scope

EveCraft is **server-side only** and does not require:
- External APIs or ML models
- Client modifications
- New opcodes (uses existing combine packets)
- Complex math (integer-based scaling)

It leverages:
- Existing tradeskill system (combine detection, skill tracking)
- Item instance system (custom data storage)
- Data bucket system (result caching)
- Lua/Perl quest hooks (logic scripting)

See [TECHNICAL_IMPLEMENTATION.md](TECHNICAL_IMPLEMENTATION.md) for code architecture.

---

## Future Enhancements (Out of Scope for v1)

- **Group recipes:** 3+ items combined for special results
- **NPC teaching:** NPCs can unlock special recipe types
- **Seasonal events:** Limited-time special combinations
- **PvP crafting:** Player-requested customs
- **Transmog system:** Transform existing items without combining
