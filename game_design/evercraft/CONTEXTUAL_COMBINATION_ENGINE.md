# EveCraft: Contextual Combination Engine

## The Problem

Combining any two items is easy. Combining them in a way that **makes sense** is hard.

**Bad result:** Fire + Water → "Fiery Water" (doesn't make thematic sense)
**Good result:** Fire + Water → "Burning Ashes" (thematic + intuitive)

**Bad result:** Rare fire chest + fire spell item → "Flame Chestplate" (identical stats, redundant)
**Good result:** Rare fire chest + fire spell item → "Blazing Chestplate of Ro" (combined theme, enhanced quality)

This document explains the **Contextual Combination Engine** — how EveCraft determines what items should produce based on what they are, what themes they embody, and how those themes interact.

---

## Architecture Overview

```
Player combines: Item A + Item B
    ↓
Extract Item Attributes
    ├─ Type (Weapon, Armor, Material, Container, etc.)
    ├─ Theme (Fire, Water, Dragon, Undead, etc.)
    ├─ Slot (Chest, Head, Legs, etc.)
    ├─ Rarity (Common, Rare, Artifact, etc.)
    ├─ Resistances/Affinities (Cold/Fire/Magic)
    └─ Associated NPC/Faction (Fennin Ro, Dragons, etc.)
    ↓
Match Combination Template
    ├─ Same Theme + Armor + Different Slot
    │   → Enhanced Armor with theme
    ├─ Opposing Themes (Fire + Water)
    │   → Ashes/Neutralized result
    ├─ Theme + Consumable
    │   → Thematic consumable (ashes, water)
    ├─ Rare Item + Rare Item
    │   → Hybrid combining both themes
    └─ Fallback: Average generic combination
    ↓
Generate Context-Aware Name
    └─ Use template based on combo type
    ↓
Compute Context-Aware Stats
    └─ Blend stats based on template logic
```

---

## Step 1: Item Attribute Extraction

Before combining, the system **categorizes and tags** each item.

### Item Type Classification

```cpp
enum class ItemCategory {
    WEAPON,           // Sword, axe, bow, etc.
    ARMOR,            // Chest, legs, head, gloves, boots, shoulders
    MATERIAL,         // Leather, metal, cloth, wood, stone
    CONSUMABLE,       // Food, drink, potion
    CONTAINER,        // Bags, backpacks (cannot combine)
    COMPONENT,        // Gem, ore, herb (crafting material)
    QUEST_ITEM,       // Quest flag set (cannot combine)
    CURRENCY,         // Platinum, alternate currency (cannot combine)
    MAGICAL_FOCUS,    // Wand, staff, focus item
    TOOL,             // Hammer, wrench, tinkering tool
    UNKNOWN           // Fallback
};

// Example classification logic:
ItemCategory ClassifyItem(const EQ::ItemData* item) {
    if (item->QuestItemFlag) return ItemCategory::QUEST_ITEM;
    if (item->ItemType == ItemType::Armor) return ItemCategory::ARMOR;
    if (item->Attack > 0 && item->Damage > 0) return ItemCategory::WEAPON;
    if (item->ItemClass == ItemClass::Container) return ItemCategory::CONTAINER;
    // ... etc ...
    return ItemCategory::UNKNOWN;
}
```

### Theme/Affinity Detection

**Themes** are extracted from item names, resistances, and properties.

```cpp
struct ItemTheme {
    std::vector<std::string> keywords;  // ["fire", "flame", "burning"]
    int32_t primary_resistance;          // CR, FR, MR, etc.
    std::string associated_npc;          // "Fennin Ro", "Dragons", etc.
    std::string material_type;           // "Dragon", "Undead", "Elemental"
    uint8_t rarity_tier;                 // 0=trash, 1=common, 2=rare, 3=epic, 4=artifact
};

// Theme extraction from name
ItemTheme ExtractTheme(const EQ::ItemData* item) {
    ItemTheme theme;
    std::string name_lower = ToLower(item->Name);

    // Extract keywords from name
    static const std::map<std::string, std::string> theme_keywords = {
        {"flame", "fire"},
        {"burning", "fire"},
        {"inferno", "fire"},
        {"frost", "cold"},
        {"frozen", "cold"},
        {"icy", "cold"},
        {"water", "water"},
        {"aqua", "water"},
        {"dragon", "dragon"},
        {"dragonscale", "dragon"},
        {"undead", "undead"},
        {"skeletal", "undead"},
        {"rot", "undead"},
        {"radiant", "holy"},
        {"shadow", "shadow"},
        {"shadow", "shadow"},
        {"darkness", "shadow"},
        {"necro", "shadow"},
    };

    for (const auto& [keyword, theme_name] : theme_keywords) {
        if (name_lower.find(keyword) != std::string::npos) {
            theme.keywords.push_back(theme_name);
        }
    }

    // Extract from resistances (if high resist = affinity)
    if (item->FR > 10) {
        theme.primary_resistance = item->FR;
        if (theme.keywords.empty()) {
            theme.keywords.push_back("fire");
        }
    }
    if (item->CR > 10) {
        theme.primary_resistance = item->CR;
        if (theme.keywords.empty()) {
            theme.keywords.push_back("cold");
        }
    }
    // ... etc for other resists ...

    // Extract rarity (from name patterns)
    if (name_lower.find("legendary") != std::string::npos) {
        theme.rarity_tier = 4;
    } else if (name_lower.find("artifact") != std::string::npos) {
        theme.rarity_tier = 4;
    } else if (name_lower.find("epic") != std::string::npos) {
        theme.rarity_tier = 3;
    } else if (item->LoreFlag) {  // Lore = typically rare/unique
        theme.rarity_tier = 2;
    } else {
        theme.rarity_tier = 1;
    }

    // Extract associated NPC/faction
    if (name_lower.find("fennin") != std::string::npos) {
        theme.associated_npc = "Fennin Ro";
    } else if (name_lower.find("dragon") != std::string::npos) {
        theme.associated_npc = "Dragons";
    }
    // ... etc ...

    return theme;
}
```

### Armor Slot Detection

```cpp
enum class ArmorSlot {
    CHEST,
    HEAD,
    LEGS,
    HANDS,
    FEET,
    SHOULDERS,
    BACK,
    WAIST,
    WRIST,
    FINGER,
    EAR,
    OTHER
};

ArmorSlot GetArmorSlot(const EQ::ItemData* item) {
    // Check item slots bitfield
    if (item->Slots & (1 << EQ::invslot::SLOT_CHEST)) return ArmorSlot::CHEST;
    if (item->Slots & (1 << EQ::invslot::SLOT_HEAD)) return ArmorSlot::HEAD;
    if (item->Slots & (1 << EQ::invslot::SLOT_LEGS)) return ArmorSlot::LEGS;
    if (item->Slots & (1 << EQ::invslot::SLOT_HANDS)) return ArmorSlot::HANDS;
    if (item->Slots & (1 << EQ::invslot::SLOT_FEET)) return ArmorSlot::FEET;
    if (item->Slots & (1 << EQ::invslot::SLOT_SHOULDERS)) return ArmorSlot::SHOULDERS;
    if (item->Slots & (1 << EQ::invslot::SLOT_BACK)) return ArmorSlot::BACK;
    if (item->Slots & (1 << EQ::invslot::SLOT_WAIST)) return ArmorSlot::WAIST;
    if (item->Slots & (1 << EQ::invslot::SLOT_WRIST)) return ArmorSlot::WRIST;
    // ... etc ...
    return ArmorSlot::OTHER;
}
```

---

## Step 2: Combination Template Matching

After extracting attributes, match the combination against **template rules**.

Each template defines:
- **Input criteria** (what items it applies to)
- **Output name template** (how to name the result)
- **Stat blending formula** (how to compute stats)
- **Priority** (higher = checked first)

### Template System

```cpp
struct CombinationTemplate {
    std::string id;                      // "fire_armor_fusion"
    uint8_t priority;                    // 0-255 (higher = checked first)

    // Input matching
    std::function<bool(const ItemContext&, const ItemContext&)> matches;

    // Output generation
    std::function<std::string(const ItemContext&, const ItemContext&)> generate_name;
    std::function<StatBlend(const ItemContext&, const ItemContext&, uint8_t)> blend_stats;

    // Validation
    std::function<bool(const ItemContext&, const ItemContext&)> validate;
};

struct ItemContext {
    const EQ::ItemData* item;
    ItemCategory category;
    ItemTheme theme;
    ArmorSlot slot;
    uint16_t skill;
};
```

### Template Examples

**Template 1: Same Theme + Armor → Themed Armor**

```cpp
CombinationTemplate same_theme_armor = {
    .id = "same_theme_armor",
    .priority = 100,

    .matches = [](const ItemContext& a, const ItemContext& b) {
        // Both are armor
        if (a.category != ItemCategory::ARMOR || b.category != ItemCategory::ARMOR) {
            return false;
        }

        // Both have a common theme
        for (const auto& theme_a : a.theme.keywords) {
            for (const auto& theme_b : b.theme.keywords) {
                if (theme_a == theme_b) {
                    return true;  // Match found
                }
            }
        }
        return false;
    },

    .generate_name = [](const ItemContext& a, const ItemContext& b, uint8_t tier) {
        // Find common theme
        std::string common_theme;
        for (const auto& theme_a : a.theme.keywords) {
            for (const auto& theme_b : b.theme.keywords) {
                if (theme_a == theme_b) {
                    common_theme = theme_a;
                    break;
                }
            }
            if (!common_theme.empty()) break;
        }

        // Determine dominant armor piece
        const auto* dominant = (a.item->AC < b.item->AC) ? a.item : b.item;
        std::string slot_name = GetArmorSlotName(GetArmorSlot(dominant));

        // Generate name based on tier
        static const std::vector<std::string> tier_prefixes = {
            "Crude", "Standard", "Enhanced", "Masterwork"
        };

        std::string theme_adjective = CapitalizeFirst(common_theme);
        return fmt::format("{} {} {}",
            tier_prefixes[std::min(tier, (uint8_t)3)],
            theme_adjective,
            slot_name
        );
        // Result: "Enhanced Fire Chestplate" or "Masterwork Ice Gauntlets"
    },

    .blend_stats = [](const ItemContext& a, const ItemContext& b, uint8_t tier) {
        StatBlend result;
        result.ac = (a.item->AC + b.item->AC) / 2;
        result.hp = (a.item->HP + b.item->HP) / 2;

        // BONUS: Enhanced resistance from common theme
        if (std::find(a.theme.keywords.begin(), a.theme.keywords.end(), "fire")
            != a.theme.keywords.end()) {
            result.resistances[RES_FIRE] = (a.item->FR + b.item->FR) / 2 + 5;
        }

        // Apply tier multiplier
        result = ApplyTierMultiplier(result, tier);
        return result;
    },

    .validate = [](const ItemContext& a, const ItemContext& b) {
        // Can't combine two of the exact same armor
        return a.item->ID != b.item->ID;
    }
};
```

**Template 2: Opposing Elements → Neutralization**

```cpp
CombinationTemplate opposing_elements = {
    .id = "opposing_elements",
    .priority = 90,

    .matches = [](const ItemContext& a, const ItemContext& b) {
        // Check for opposing theme pairs
        static const std::set<std::pair<std::string, std::string>> opposites = {
            {"fire", "water"},
            {"water", "fire"},
            {"hot", "cold"},
            {"cold", "hot"},
            {"light", "shadow"},
            {"shadow", "light"},
        };

        for (const auto& theme_a : a.theme.keywords) {
            for (const auto& theme_b : b.theme.keywords) {
                auto pair = std::make_pair(theme_a, theme_b);
                if (opposites.find(pair) != opposites.end()) {
                    return true;
                }
            }
        }
        return false;
    },

    .generate_name = [](const ItemContext& a, const ItemContext& b, uint8_t tier) {
        // Determine opposing pair
        std::string theme_a_str, theme_b_str;
        for (const auto& theme_a : a.theme.keywords) {
            for (const auto& theme_b : b.theme.keywords) {
                if (IsOpposite(theme_a, theme_b)) {
                    theme_a_str = theme_a;
                    theme_b_str = theme_b;
                    break;
                }
            }
        }

        // Result is neutralized version
        // Fire + Water = Ashes/Steam
        if ((theme_a_str == "fire" && theme_b_str == "water") ||
            (theme_a_str == "water" && theme_b_str == "fire")) {
            return tier == 0 ? "Scorched Remains" :
                   tier == 1 ? "Steam Cloud" :
                   tier == 2 ? "Crystallized Vapor" :
                   "Pure Steam Essence";
        }

        // Light + Shadow = Void/Twilight
        if ((theme_a_str == "light" && theme_b_str == "shadow") ||
            (theme_a_str == "shadow" && theme_b_str == "light")) {
            return tier == 0 ? "Murky Twilight" :
                   tier == 1 ? "Twilight Essence" :
                   tier == 2 ? "Balanced Void" :
                   "Pure Void Matter";
        }

        return "Neutralized Matter";
    },

    .blend_stats = [](const ItemContext& a, const ItemContext& b, uint8_t tier) {
        StatBlend result;

        // Neutralized stats are DIFFERENT from combining same theme
        // More focus on resistances, less on damage
        result.hp = (a.item->HP + b.item->HP) / 2;
        result.ac = (a.item->AC + b.item->AC) / 2;

        // REDUCED attack from neutralization
        result.attack = ((a.item->Attack + b.item->Attack) / 2) * 0.75;

        // HIGH universal resistance from neutralization
        for (int i = 0; i < 6; i++) {
            result.resistances[i] = 10 + tier * 3;
        }

        return result;
    },

    .validate = [](const ItemContext& a, const ItemContext& b) {
        return true;  // Always valid
    }
};
```

**Template 3: Rare + Rare → Hybrid Artifact**

```cpp
CombinationTemplate rare_fusion = {
    .id = "rare_fusion",
    .priority = 95,

    .matches = [](const ItemContext& a, const ItemContext& b) {
        // Both must be rare or higher rarity
        return a.theme.rarity_tier >= 2 && b.theme.rarity_tier >= 2;
    },

    .generate_name = [](const ItemContext& a, const ItemContext& b, uint8_t tier) {
        // Combine theme keywords
        std::vector<std::string> all_themes;
        for (const auto& t : a.theme.keywords) all_themes.push_back(t);
        for (const auto& t : b.theme.keywords) {
            if (std::find(all_themes.begin(), all_themes.end(), t) == all_themes.end()) {
                all_themes.push_back(t);
            }
        }

        // If associated with NPC, use that
        std::string npc_name;
        if (!a.theme.associated_npc.empty()) npc_name = a.theme.associated_npc;
        else if (!b.theme.associated_npc.empty()) npc_name = b.theme.associated_npc;

        if (!npc_name.empty()) {
            // "Blazing Chestplate of Fennin Ro"
            return fmt::format("{} {} of {}",
                CapitalizeFirst(all_themes[0]),
                GetArmorSlotName(GetArmorSlot(a.item)),
                npc_name
            );
        }

        // Generic rare fusion
        std::string theme_str = all_themes.empty() ? "Hybrid" :
                                CapitalizeFirst(all_themes[0]);
        return fmt::format("Fused {} Item", theme_str);
    },

    .blend_stats = [](const ItemContext& a, const ItemContext& b, uint8_t tier) {
        StatBlend result;

        // ENHANCED stats from rare fusion
        result.hp = (a.item->HP + b.item->HP) / 2 * 1.25;  // 25% bonus
        result.ac = (a.item->AC + b.item->AC) / 2 * 1.2;
        result.attack = (a.item->Attack + b.item->Attack) / 2 * 1.15;

        // Bonus: Both themes' resistances
        for (int i = 0; i < 6; i++) {
            result.resistances[i] = std::max(a.item->resistances[i], b.item->resistances[i]);
        }

        // Apply tier multiplier (on top of bonuses)
        result = ApplyTierMultiplier(result, tier);

        return result;
    },

    .validate = [](const ItemContext& a, const ItemContext& b) {
        // Can't combine exact same item (lore conflict)
        return a.item->ID != b.item->ID;
    }
};
```

**Template 4: Theme + Material → Themed Material Version**

```cpp
CombinationTemplate theme_material = {
    .id = "theme_material",
    .priority = 70,

    .matches = [](const ItemContext& a, const ItemContext& b) {
        bool has_theme = (!a.theme.keywords.empty() || !b.theme.keywords.empty());
        bool has_material = (a.category == ItemCategory::MATERIAL ||
                            b.category == ItemCategory::MATERIAL);
        return has_theme && has_material;
    },

    .generate_name = [](const ItemContext& a, const ItemContext& b, uint8_t tier) {
        // Determine which is material, which is themed
        const auto* material_item = (a.category == ItemCategory::MATERIAL) ? a.item : b.item;
        const auto& themed = (a.category == ItemCategory::MATERIAL) ? b : a;

        std::string material_name = material_item->Name;
        std::string theme_name = themed.theme.keywords.empty() ?
            "Enhanced" : CapitalizeFirst(themed.theme.keywords[0]);

        return fmt::format("{} {}",
            theme_name,
            material_name
        );
        // Result: "Burning Leather", "Icy Ore"
    },

    .blend_stats = [](const ItemContext& a, const ItemContext& b, uint8_t tier) {
        // Materials don't have many stats, so this is mostly about
        // the themed item's properties
        const auto* themed = (a.category == ItemCategory::MATERIAL) ? b.item : a.item;

        StatBlend result;
        result.hp = themed->HP;
        result.ac = themed->AC;
        result.attack = themed->Attack;

        // Add material resistance bonus
        for (int i = 0; i < 6; i++) {
            result.resistances[i] = themed->resistances[i] + 2;
        }

        return result;
    },

    .validate = [](const ItemContext& a, const ItemContext& b) {
        return true;
    }
};
```

---

## Step 3: Template Priority and Fallback

Templates are checked in **priority order**. First match wins.

```cpp
std::vector<CombinationTemplate> templates = {
    same_theme_armor,      // Priority 100
    rare_fusion,           // Priority 95
    opposing_elements,     // Priority 90
    theme_material,        // Priority 70
    // ... more templates ...
    generic_fallback,      // Priority 1 (always matches)
};

CombinationTemplate* FindMatchingTemplate(
    const ItemContext& item1,
    const ItemContext& item2
) {
    // Sort by priority descending
    std::sort(templates.begin(), templates.end(),
        [](const auto& a, const auto& b) {
            return a.priority > b.priority;
        }
    );

    // First matching template wins
    for (auto& template_def : templates) {
        if (template_def.matches(item1, item2)) {
            return &template_def;
        }
    }

    return nullptr;  // Never happens (generic_fallback always matches)
}
```

### Generic Fallback Template

```cpp
CombinationTemplate generic_fallback = {
    .id = "generic_fallback",
    .priority = 1,

    .matches = [](const ItemContext& a, const ItemContext& b) {
        return true;  // Always matches
    },

    .generate_name = [](const ItemContext& a, const ItemContext& b, uint8_t tier) {
        // Last resort: bland generic name
        std::string adj = tier == 0 ? "Crude" :
                         tier == 1 ? "Standard" :
                         tier == 2 ? "Enhanced" :
                         "Masterwork";

        return fmt::format("{} Combined Item", adj);
    },

    .blend_stats = [](const ItemContext& a, const ItemContext& b, uint8_t tier) {
        // Average both items' stats
        StatBlend result;
        result.hp = (a.item->HP + b.item->HP) / 2;
        result.ac = (a.item->AC + b.item->AC) / 2;
        result.attack = (a.item->Attack + b.item->Attack) / 2;

        for (int i = 0; i < 6; i++) {
            result.resistances[i] = (a.item->resistances[i] + b.item->resistances[i]) / 2;
        }

        return ApplyTierMultiplier(result, tier);
    },

    .validate = [](const ItemContext& a, const ItemContext& b) {
        return true;
    }
};
```

---

## Step 4: Integration into EveCraft::ProcessCombine()

```cpp
namespace EveCraft {

CombineResult DetermineCombination(
    uint32_t item1_id,
    uint32_t item2_id,
    uint16_t skill
)
{
    CombineResult result{};

    auto* item1 = database.GetItem(item1_id);
    auto* item2 = database.GetItem(item2_id);

    if (!item1 || !item2) {
        result.failure_reason = "Item not found in database.";
        return result;
    }

    // === EXTRACT ITEM CONTEXTS ===
    ItemContext ctx1{
        .item = item1,
        .category = ClassifyItem(item1),
        .theme = ExtractTheme(item1),
        .slot = GetArmorSlot(item1),
        .skill = skill
    };

    ItemContext ctx2{
        .item = item2,
        .category = ClassifyItem(item2),
        .theme = ExtractTheme(item2),
        .slot = GetArmorSlot(item2),
        .skill = skill
    };

    // === FIND MATCHING TEMPLATE ===
    auto* template_match = FindMatchingTemplate(ctx1, ctx2);

    if (!template_match) {
        result.failure_reason = "No valid combination template matched.";
        return result;
    }

    // === VALIDATE COMBINATION ===
    if (!template_match->validate(ctx1, ctx2)) {
        result.failure_reason = "Combination violates template rules.";
        return result;
    }

    // === DETERMINE SKILL TIER ===
    uint8_t skill_tier = GetSkillTier(skill);

    // === GENERATE NAME ===
    std::string result_name = template_match->generate_name(ctx1, ctx2, skill_tier);

    // === BLEND STATS ===
    StatBlend result_stats = template_match->blend_stats(ctx1, ctx2, skill_tier);

    // === CREATE ITEM ===
    // Check cache first
    uint32_t cached_id;
    if (TryGetCachedResult(item1_id, item2_id, cached_id)) {
        result.result_item_id = cached_id;
        result.result_name = result_name;
        result.success = true;
        return result;
    }

    // Create new item
    uint32_t new_item_id = InsertDynamicItem(
        item1_id,
        item2_id,
        result_name,
        result_stats,
        template_match->id,
        skill_tier
    );

    // Cache for next time
    CacheResult(item1_id, item2_id, new_item_id, result_name);

    result.result_item_id = new_item_id;
    result.result_name = result_name;
    result.success = true;

    return result;
}

} // namespace EveCraft
```

---

## Real-World Examples

### Example 1: Fire + Armor

```
Player combines:
  Item 1: "Pure Flame of Fennin Ro" (rare, fire theme, 25 attack, 15 FR)
  Item 2: "Chestplate of the Order of Fennin Ro" (armor, fire theme, -15 AC, 10 FR)
  Skill: 150 (tier 2 = Enhanced)

Extract contexts:
  ctx1: {category: WEAPON, theme: {keywords: ["fire", "fennin"], rarity: 2}}
  ctx2: {category: ARMOR, slot: CHEST, theme: {keywords: ["fire", "fennin"], rarity: 2}}

Template matching (priority order):
  1. same_theme_armor? Both have "fire" theme? YES ✓

Template: same_theme_armor
  .generate_name() → "Enhanced Fire Chestplate of Fennin Ro"
  .blend_stats() → {AC: -17, FR: 20, HP: 50, Attack: 15}

Result: New item "Enhanced Fire Chestplate of Fennin Ro"
  Stats: -17 AC, +15 FR, +50 HP
  → Makes sense: Fire armor combined with fire weapon = stronger fire armor
```

### Example 2: Fire + Water

```
Player combines:
  Item 1: "Pure Flame of Fennin Ro" (fire, 25 attack, 15 FR)
  Item 2: "Water Flask" (material, water, no stats)
  Skill: 50 (tier 1 = Standard)

Extract contexts:
  ctx1: {category: WEAPON, theme: {keywords: ["fire", "flame"]}}
  ctx2: {category: MATERIAL, theme: {keywords: ["water"]}}

Template matching (priority order):
  1. same_theme_armor? No (ctx1 is weapon, not armor)
  2. rare_fusion? No (ctx2 has low rarity)
  3. opposing_elements? Fire vs Water? YES ✓

Template: opposing_elements
  .generate_name() → "Steam Cloud"
  .blend_stats() → {AC: 0, resistances: [10, 10, 10, 10, 10, 10], attack: 12}

Result: New item "Steam Cloud"
  Stats: No AC, +10 all resistances, reduced attack
  → Makes sense: Fire + Water = neutralized to steam with universal resistance
```

### Example 3: Leather + Dragon Helm

```
Player combines:
  Item 1: "Dragon Leather" (material, dragon theme, rarity: 1)
  Item 2: "Helm of Dragons" (armor, dragon theme, rarity: 2)
  Skill: 200 (tier 2 = Enhanced)

Extract contexts:
  ctx1: {category: MATERIAL, theme: {keywords: ["dragon"], rarity: 1}}
  ctx2: {category: ARMOR, slot: HEAD, theme: {keywords: ["dragon"], rarity: 2}}

Template matching:
  1. same_theme_armor? No (ctx1 is material, not armor)
  2. rare_fusion? rarity >= 2 for both? Only ctx2 qualifies, No
  3. opposing_elements? No
  4. theme_material? Has theme AND material? YES ✓

Template: theme_material
  .generate_name() → "Dragon Helm"
  .blend_stats() → {AC: -18, attack: 5, resistances: [5, 5, 5, 5, 5, 5]}

Result: New item "Dragon Helm"
  Stats: -18 AC, +5 all resistances (from dragon theme)
  → Makes sense: Dragon material infused into dragon helm = stronger dragon helm
```

---

## Template Extensibility

New templates can be added easily:

```cpp
templates.push_back(CombinationTemplate{
    .id = "custom_fusion",
    .priority = 75,
    .matches = [](const ItemContext& a, const ItemContext& b) {
        // Custom logic
        return /* your condition */;
    },
    .generate_name = [](const ItemContext& a, const ItemContext& b, uint8_t tier) {
        // Custom naming
        return "Custom Result";
    },
    .blend_stats = [](const ItemContext& a, const ItemContext& b, uint8_t tier) {
        // Custom stat formula
        return StatBlend{};
    },
    .validate = [](const ItemContext& a, const ItemContext& b) {
        return true;
    }
});
```

This makes it easy to add new combination types as the system evolves.

---

## Summary

**EveCraft ensures combinations "make sense" through:**

1. **Attribute Extraction** — Classify items by type, theme, rarity, slot
2. **Template Matching** — Match combinations against contextual rules (100+ possible)
3. **Priority System** — Check templates in priority order (specific → generic)
4. **Theme-Based Naming** — Generate names reflecting the combination type
5. **Theme-Based Stats** — Adjust stats based on themes (bonuses, penalties, neutralization)
6. **Fallback Logic** — Generic template ensures no combination fails

**Result:** Fire + Fire = Enhanced Fire thing. Fire + Water = Neutralized steam. Rare + Rare = Hybrid artifact. Never "Fiery Water" or "Random Junk".
