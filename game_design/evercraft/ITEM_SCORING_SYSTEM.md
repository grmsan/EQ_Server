# EveCraft: Item Scoring & Tiering System

## Overview

To make combination results contextually appropriate and historically accurate ("Classic BiS" vs "PoP BiS"), EveCraft uses a **Power Score (PS)** and a **Tier (T)** system.

This ensures that:
1. Combining two high-quality items from a lower expansion results in a high-quality item for that expansion.
2. High-end raid items have a "weight" that allows them to push combinations into higher tiers better than trash drops.
3. Expansion power creep is natively handled.

---

## 1. The Power Score (PS) Formula

The Power Score is a raw numerical value representing the total "stat energy" in an item. It is calculated during the first discovery of an item and cached.

### Base Stat Weights ($W$)
Items are scored based on the following weights (Relative to 1 HP = 1 Point):

| Stat | Weight | Notes |
|------|--------|-------|
| **HP** | 1.0 | Base unit |
| **Mana** | 1.0 | Base unit |
| **Endurance** | 1.0 | Base unit |
| **AC** | 4.0 | Mitigation is high value |
| **Attributes** (STR/DEX/etc.) | 2.5 | Core stats |
| **Haste** | 10.0 | Per 1% (e.g., 36% = 360 pts) |
| **Damage** (Weapon) | 15.0 | High weight for primary source |
| **Attack** | 2.0 | Flat attack bonus |
| **Resistances** | 1.5 | MR, FR, CR, etc. |
| **Heroic Stats** | 20.0 | High value for endgame items |
| **Strike Through** | 8.0 | Performance mod |
| **Spell Shielding** | 12.0 | Performance mod |
| **Combat Effects** | 5.0 | Performance mod |

**Formula:**
$$PS = \sum (Stat\_Value_i \cdot W_i)$$

### Source Multipliers ($M_{source}$)
Items obtained from legendary or rare sources receive a bonus to their Power Score to reflect their prestige:

| Drop Source | Multiplier | Detection Logic |
|-------------|------------|-----------------|
| **Trash Mob** | 1.0x | Loot shared by > 10 NPCs |
| **Named / Rare** | 1.2x | Loot shared by 2-5 NPCs |
| **Raid Boss** | 1.5x | Unique Loottable (1 NPC) + Level > 50 |
| **God / Dragon** | 2.0x | Unique Loottable + Zone Expansion > 2 + HP > 2M |

---

## 2. The Tier System (T)

Tiers (1–20) normalize Power Scores across different expansions.

### Expansion Mapping

| Expansion ID | Name | Tier Range | Base PS Range |
|--------------|------|------------|---------------|
| 0 | Classic | 1–3 | 0–500 |
| 1 | Kunark | 4–6 | 501–1200 |
| 2 | Velious | 7–9 | 1201–2500 |
| 3 | Luclin | 10–12 | 2501–5000 |
| 4 | Planes of Power | 13–15 | 5001–10000 |
| 5+ | God & Beyond | 16–20 | 10001+ |

**Programmatic Tier Determination:**
1. Determine the earliest **Expansion** the item is available in (via Loot sources).
2. Calculate **Power Score**.
3. Map $PS$ to the sub-tier within that expansion.

---

## 3. Combination Tier Logic

When two items are combined, the resulting Item Tier ($T_{result}$) is determined by the inputs ($T_A, T_B$).

### The "Ugrade Chance" Algorithm

1. **Calculate Average Tier**: $T_{avg} = (T_A + T_B) / 2$
2. **Calculate Gap**: $\Delta = |T_A - T_B|$
3. **Roll for Upgrade**:
   - If $\Delta \le 1$: 20% chance result is $MAX(T_A, T_B) + 1$.
   - If $\Delta > 5$: 5% chance result is Tier $MAX(T_A, T_B) + 1$ (The lower item acts as a "catalyst").
   - Otherwise: Result is Tier $T_{avg}$ (rounded up).

### Example 1: Classic BiS + Classic BiS
- Item A: Cloak of Flames (T3, PS 600)
- Item B: Short Shield (T1, PS 50)
- $T_{avg} = 2$.
- Result is likely a Tier 2-3 item.

### Example 2: PoP Raid + PoP Trash
- Item A: Quarm Hammer (T15, PS 12,000)
- Item B: Ethereal Parchment (T13, PS 4,500)
- Mid-range chance result is Tier 15.
- Small chance result is Tier 16 (Breakthrough Craft).

---

## 4. Item Classification "Flags"

Items in the dynamic range (ID >= 1B) will store their origin information in their `custom_data` (JSON string in the DB):

```json
{
  "evercraft": {
    "source_tier": 14,
    "ps": 8450,
    "source_flag": "RAID_BOSS",
    "discovery_char": 102,
    "version": 1
  }
}
```

This allows the system to verify the pedigree of an item at any point in the future.
