# EveCraft: Contextual Logic Examples & Flow

## How Your Examples Work in the System

Here's exactly how the three examples you mentioned flow through the Contextual Combination Engine:

---

## Example 1: Fire + Armor = Themed Armor

```
USER COMBINES:
  Pure Flame of Fennin Ro (Weapon)
  + Chestplate of the Order of Fennin Ro (Armor)

┌─ ATTRIBUTE EXTRACTION ─────────────────────────────────────────┐
│                                                                 │
│ Item 1: Pure Flame of Fennin Ro                               │
│  ├─ Category: WEAPON                                          │
│  ├─ Theme.keywords: ["fire", "flame", "fennin"]              │
│  ├─ Theme.rarity_tier: 2 (RARE)                              │
│  ├─ Attack: 25                                               │
│  └─ FR (Fire Resistance): 15                                 │
│                                                                 │
│ Item 2: Chestplate of the Order of Fennin Ro                 │
│  ├─ Category: ARMOR                                          │
│  ├─ Slot: CHEST                                              │
│  ├─ Theme.keywords: ["fire", "fennin"]                       │
│  ├─ Theme.rarity_tier: 2 (RARE)                              │
│  ├─ AC: -15                                                  │
│  └─ FR (Fire Resistance): 10                                 │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘

┌─ TEMPLATE MATCHING (Priority Order) ────────────────────────────┐
│                                                                 │
│ Template 100: same_theme_armor                                │
│  ├─ Check: Both ARMOR? NO (one is WEAPON) ✗                  │
│  │                                                              │
│ Template 95: rare_fusion                                      │
│  ├─ Check: Both rarity >= 2? YES ✓                           │
│  ├─ Check: Do they have common theme? "fire" = "fire"? YES ✓│
│  ├─ MATCH FOUND! Use rare_fusion template                    │
│  │                                                              │
│ (stop checking further templates)                             │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘

┌─ STAT BLENDING (rare_fusion template) ──────────────────────────┐
│                                                                 │
│ Result Stats Computed:                                         │
│  ├─ AC = (-15 + 0) / 2 * 1.2 = -9 (25% armor bonus)         │
│  ├─ Attack = (25 + 0) / 2 * 1.15 = 14 (15% weapon bonus)    │
│  ├─ HP = (0 + 50) / 2 * 1.25 = 31 (25% rare fusion bonus)   │
│  └─ FR = MAX(15, 10) = 15 (use highest fire resist)         │
│                                                                 │
│ Tier 2 (Enhanced) Multiplier Applied:                         │
│  ├─ AC: -9 * 1.5 = -13                                       │
│  ├─ Attack: 14 * 1.5 = 21                                    │
│  └─ HP: 31 * 1.5 = 47                                        │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘

┌─ NAME GENERATION (rare_fusion template) ────────────────────────┐
│                                                                 │
│ Logic:                                                          │
│  ├─ Extract common theme: "fire"                             │
│  ├─ Extract armor slot: CHEST → "Chestplate"                │
│  ├─ Extract associated NPC: "Fennin Ro"                      │
│  ├─ Combine with tier prefix: "Enhanced"                     │
│  │                                                              │
│ Result Name:                                                   │
│  "Enhanced Fire Chestplate of Fennin Ro"                     │
│                                                                 │
│ (User sees the thematic connection: Fire + Fire = Stronger   │
│  fire armor, not random output)                               │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘

FINAL RESULT:
  ✓ "Enhanced Fire Chestplate of Fennin Ro"
    Stats: -13 AC, +21 attack (from weapon), +47 HP, +15 FR
    Why it makes sense: Fire weapon fused into fire armor =
                        stronger fire-themed armor
```

---

## Example 2: Fire + Water = Neutralized Result

```
USER COMBINES:
  Pure Flame of Fennin Ro (Fire)
  + Water Flask (Water)

┌─ ATTRIBUTE EXTRACTION ─────────────────────────────────────────┐
│                                                                 │
│ Item 1: Pure Flame of Fennin Ro                               │
│  ├─ Category: WEAPON                                          │
│  ├─ Theme.keywords: ["fire", "flame"]                        │
│  ├─ Theme.rarity_tier: 2 (RARE)                              │
│  └─ FR: 15                                                    │
│                                                                 │
│ Item 2: Water Flask                                           │
│  ├─ Category: MATERIAL                                        │
│  ├─ Theme.keywords: ["water", "liquid"]                      │
│  ├─ Theme.rarity_tier: 0 (COMMON)                            │
│  └─ CR: 0 (no special properties)                            │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘

┌─ TEMPLATE MATCHING (Priority Order) ────────────────────────────┐
│                                                                 │
│ Template 100: same_theme_armor                                │
│  ├─ Check: Both ARMOR? NO ✗                                  │
│  │                                                              │
│ Template 95: rare_fusion                                      │
│  ├─ Check: Both rarity >= 2? NO (water is common) ✗          │
│  │                                                              │
│ Template 90: opposing_elements ⭐                              │
│  ├─ Check: Opposing themes?                                  │
│  │   Is ("fire", "water") in opposites set? YES ✓            │
│  ├─ MATCH FOUND! Use opposing_elements template              │
│  │                                                              │
│ (stop checking further templates)                             │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘

┌─ STAT BLENDING (opposing_elements template) ────────────────────┐
│                                                                 │
│ Result Stats Computed:                                         │
│  ├─ Attack = (25 + 0) / 2 * 0.75 = 9 (REDUCED by 25%)       │
│  │  (neutralization reduces damage)                           │
│  ├─ AC = (-15 + 0) / 2 = -8                                  │
│  ├─ HP = (0 + 0) / 2 = 0                                     │
│  └─ ALL Resistances = 10 (neutral resist to everything)      │
│     ├─ CR: 10                                                │
│     ├─ DR: 10                                                │
│     ├─ PR: 10                                                │
│     ├─ MR: 10                                                │
│     ├─ FR: 10                                                │
│     └─ SVCorrup: 10                                          │
│                                                                 │
│ Tier 1 (Standard) Multiplier Applied:                         │
│  ├─ Attack: 9 * 1.25 = 11                                    │
│  └─ Resistances: +tier*3 = +3 bonus → 13 each               │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘

┌─ NAME GENERATION (opposing_elements template) ──────────────────┐
│                                                                 │
│ Logic:                                                          │
│  ├─ Detect opposing pair: ("fire", "water")                 │
│  ├─ Fire + Water = Ashes/Steam                              │
│  ├─ Tier 1 (Standard) uses second-tier name                 │
│  │                                                              │
│ Result Name:                                                   │
│  "Steam Cloud"                                                │
│                                                                 │
│ Other possible names by tier:                                │
│  ├─ Tier 0: "Scorched Remains"                              │
│  ├─ Tier 1: "Steam Cloud" ← current                         │
│  ├─ Tier 2: "Crystallized Vapor"                            │
│  └─ Tier 3: "Pure Steam Essence"                            │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘

FINAL RESULT:
  ✓ "Steam Cloud"
    Stats: +11 attack, -8 AC, +13 all resistances
    Why it makes sense: Fire + Water = Neutralized steam with
                        universal protection, not "Fiery Water"
```

---

## Example 3: Dragon Helm + Dragon Leather = Enhanced Dragon Helm

```
USER COMBINES:
  Helm of Dragons (Dragon Armor)
  + Dragon Leather (Dragon Material)

┌─ ATTRIBUTE EXTRACTION ─────────────────────────────────────────┐
│                                                                 │
│ Item 1: Helm of Dragons                                       │
│  ├─ Category: ARMOR                                          │
│  ├─ Slot: HEAD                                               │
│  ├─ Theme.keywords: ["dragon"]                               │
│  ├─ Theme.associated_npc: "Dragons"                          │
│  ├─ Theme.rarity_tier: 2 (RARE)                              │
│  └─ AC: -18                                                  │
│                                                                 │
│ Item 2: Dragon Leather                                        │
│  ├─ Category: MATERIAL                                        │
│  ├─ Theme.keywords: ["dragon"]                               │
│  ├─ Theme.rarity_tier: 1 (COMMON)                            │
│  └─ No inherent stats                                         │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘

┌─ TEMPLATE MATCHING (Priority Order) ────────────────────────────┐
│                                                                 │
│ Template 100: same_theme_armor                                │
│  ├─ Check: Both ARMOR? NO (one is MATERIAL) ✗               │
│  │                                                              │
│ Template 95: rare_fusion                                      │
│  ├─ Check: Both rarity >= 2? NO (leather is common) ✗       │
│  │                                                              │
│ Template 90: opposing_elements                                │
│  ├─ Check: Opposing themes? NO ("dragon" ≠ opposite) ✗     │
│  │                                                              │
│ Template 70: theme_material ⭐                                 │
│  ├─ Check: Has theme AND material?                          │
│  │   Has theme: YES ("dragon")                              │
│  │   Has material: YES (Dragon Leather)                     │
│  ├─ MATCH FOUND! Use theme_material template                │
│  │                                                              │
│ (stop checking further templates)                             │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘

┌─ STAT BLENDING (theme_material template) ──────────────────────┐
│                                                                 │
│ Result Stats Computed:                                         │
│  ├─ Inherit from themed item (Helm of Dragons):             │
│  │   ├─ AC: -18                                             │
│  │   └─ All Resistances from helm                          │
│  │                                                              │
│  ├─ Add material affinity bonus: +2 to all resistances      │
│  │   ├─ CR: +2                                              │
│  │   ├─ DR: +2                                              │
│  │   ├─ PR: +2                                              │
│  │   ├─ MR: +2                                              │
│  │   ├─ FR: +2                                              │
│  │   └─ SVCorrup: +2                                        │
│  │                                                              │
│ Tier 2 (Enhanced) Multiplier Applied:                         │
│  ├─ AC: -18 * 1.5 = -27 (greatly enhanced armor)            │
│  └─ Resistances: each +2 * 1.5 = +3 bonus                  │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘

┌─ NAME GENERATION (theme_material template) ────────────────────┐
│                                                                 │
│ Logic:                                                          │
│  ├─ Determine which is material: Dragon Leather             │
│  ├─ Determine themed item: Helm of Dragons                  │
│  ├─ Extract theme: "dragon"                                 │
│  ├─ Extract material: "Leather"                             │
│  ├─ Format: "{theme} {material}"                            │
│  │                                                              │
│ Result Name:                                                   │
│  "Dragon Helm"                                                │
│                                                                 │
│ (Simple and effective: Material themed into armor piece)     │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘

FINAL RESULT:
  ✓ "Dragon Helm"
    Stats: -27 AC (greatly improved), +5 all resistances (from dragon material infusion)
    Why it makes sense: Dragon leather infused into dragon helm =
                        stronger dragon armor with dragon affinity
```

---

## The Key Insight: Why These Make Sense

Each template encodes **domain knowledge** about what combinations should produce:

| Template | Rule | Output Type | Stat Treatment |
|----------|------|-------------|-----------------|
| **same_theme_armor** | Fire + Fire armor | Enhanced fire armor | Stats combined + theme bonus |
| **rare_fusion** | Rare + Rare with shared theme | Hybrid artifact | Enhanced stats + all bonuses |
| **opposing_elements** | Fire + Water | Neutralized matter | Reduced damage + universal resist |
| **theme_material** | Theme + Material | Themed material result | Material properties + theme resist bonus |
| **generic_fallback** | Anything else | Generic blend | Simple average |

The system **never produces nonsensical results** because:

1. **Explicit rules** handle common/expected combinations
2. **Priority order** ensures more specific rules win over generic ones
3. **Fallback logic** ensures every combination succeeds (worst case: generic average)
4. **Theme extraction** ensures similar items combine recognizably
5. **Stat formulas** differ per template, so fire+fire ≠ fire+water (different results)

---

## How to Add New Templates

Want fire + material to create "flaming" versions?

```cpp
CombinationTemplate theme_weapon_material = {
    .id = "theme_weapon_material",
    .priority = 72,  // Between theme_material and others

    .matches = [](const ItemContext& a, const ItemContext& b) {
        bool has_weapon = (a.category == ItemCategory::WEAPON ||
                          b.category == ItemCategory::WEAPON);
        bool has_material = (a.category == ItemCategory::MATERIAL ||
                            b.category == ItemCategory::MATERIAL);
        bool has_fire_theme = (
            std::find(a.theme.keywords.begin(), a.theme.keywords.end(), "fire") != a.theme.keywords.end() ||
            std::find(b.theme.keywords.begin(), b.theme.keywords.end(), "fire") != b.theme.keywords.end()
        );
        return has_weapon && has_material && has_fire_theme;
    },

    .generate_name = [](const ItemContext& a, const ItemContext& b, uint8_t tier) {
        return "Flaming " + GetItemTypeName(weapon_item);
    },

    .blend_stats = [](const ItemContext& a, const ItemContext& b, uint8_t tier) {
        // Fire weapon + material = enhanced fire damage on weapon
        StatBlend result;
        result.attack = weapon->Attack * 1.2;  // 20% fire bonus
        result.damage = weapon->Damage * 1.15;  // 15% fire bonus
        return ApplyTierMultiplier(result, tier);
    },

    .validate = [](const ItemContext& a, const ItemContext& b) {
        return true;
    }
};

// Add to templates list:
templates.push_back(theme_weapon_material);
```

**Result:** Fire Flask + Sword = "Flaming Sword" with enhanced fire damage. Added in 10 lines of code.

---

## Summary

The Contextual Combination Engine uses:

1. **Attribute Extraction** → What is each item?
2. **Template Matching** → What combination type is this?
3. **Priority Rules** → Which template best matches?
4. **Context-Aware Naming** → Name based on combination type
5. **Context-Aware Stats** → Stat formula differs by type
6. **Fallback Logic** → Generic option if nothing matches

**Result:** Every combination produces something that makes thematic sense, with your three examples demonstrating the three most common template types in action.
