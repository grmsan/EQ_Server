# Strength (STR) - Design Document

> **⚙️ BALANCE CONFIGURATION**: All tunable values for this system are centralized in `zone/combat_balance_config.h`.  
> **📊 IMPLEMENTATION**: Technical details in `game_design/STR_IMPLEMENTATION_PLAN.md`.

## Core Philosophy
Strength is the measure of physical force and stability. In our Solo Server environment, it serves two distinct purposes:
1.  **The Engine of Damage:** For martial classes, Strength is the primary multiplier that drives DPS from "Classic" levels to "God Tier" (100k+ DPS).
2.  **The Anchor of Stability:** For casters, Strength represents physical resilience—the ability to plant one's feet and cast a spell while a dragon is chewing on them.

---

## 1. The "Heroic Force" Formula (Damage Scaling)

To achieve the target DPS curve (5 DPS at Level 1 -> 100,000 DPS at Level 70), a linear "1 STR = 1 Damage" is insufficient. We implement a **Level-Scaled Multiplier**.

### The Formula
```cpp
StrengthDamageBonus = Strength * (Level / STR_LEVEL_DIVISOR);
// Where STR_LEVEL_DIVISOR = 10.0f (from combat_balance_config.h)
// Minimum Multiplier: STR_MIN_LEVEL_MULTIPLIER = 0.1f
```

**Tunable Constants** (see `zone/combat_balance_config.h`):
- `STR_LEVEL_DIVISOR = 10.0f` - Controls level-based scaling (higher = less damage at low levels)
- `STR_MIN_LEVEL_MULTIPLIER = 0.1f` - Minimum effectiveness (prevents zero damage)
- `OFFHAND_STR_PENALTY = 0.5f` - Offhand gets 50% STR damage (balances dual-wield)

### Weapon Delay Bonuses

**In addition to STR scaling, slower weapons get a delay-based bonus** (controlled by `ENABLE_WEAPON_DELAY_BONUS = true`):

- **1H Weapons**: `(delay - 40) / 3` bonus damage (for delay > 40)
- **2H Weapons**: `(delay - 30) / 2.5` bonus damage (for delay > 30)

**Rationale**: Slow weapons should hit harder to balance their lower attack speed. This makes weapon choice meaningful beyond just DPS calculations.

**Tunable Constants**:
- `DELAY_BONUS_DIVISOR_1H = 3.0f` - Adjusts 1H scaling strength
- `DELAY_THRESHOLD_1H = 40` - Minimum delay for 1H bonus
- `DELAY_BONUS_DIVISOR_2H = 2.5f` - Adjusts 2H scaling (slightly better than 1H)
- `DELAY_THRESHOLD_2H = 30` - Minimum delay for 2H bonus

### DPS Progression Analysis (Melee/Martial Focus)
*Note: This progression targets martial classes (Warrior, Rogue, Monk, Berserker, Ranger). Casters and Summoners achieve similar DPS targets via Spells and Pets (INT/WIS/CHA scaling), with Strength providing supplemental physical damage or utility.*

*Assumptions: Standard Weapon (Dly 30), Standard Haste/Double Attack progression.*

#### Level 1: The Rat Slayer
*   **Target:** ~5 DPS
*   **Stats:** 75 STR, Level 1.
*   **Math:** `75 * 0.1 = 7.5 Bonus Damage`.
*   **Total Hit:** 5 (Weapon) + 7.5 (STR) = **12.5 Base**.
*   **Result:** Swings for ~12-15 damage every 3 seconds. **~4-5 DPS.**
*   *Status: Perfect.*

#### Level 10: The Orc Hunter
*   **Target:** ~50 DPS
*   **Stats:** 100 STR, Level 10.
*   **Math:** `100 * 1.0 = 100 Bonus Damage`.
*   **Total Hit:** 10 (Weapon) + 100 (STR) = **110 Base**.
*   **Result:** Swings for ~110 damage. With slight haste/skills: **~40-50 DPS.**
*   *Status: On Target.*

#### Level 50: The Dragon Slayer
*   **Target:** ~5,000 DPS
*   **Stats:** 250 STR, Level 50.
*   **Math:** `250 * 5.0 = 1250 Bonus Damage`.
*   **Total Hit:** 30 (Weapon) + 1250 (STR) = **1280 Base**.
*   **Multipliers:** Damage Table (x2.5) -> ~3,200 Hit.
*   **Speed:** Haste + Double Attack (~1.5 hits/sec).
*   **Result:** **~4,800 DPS.**
*   *Status: Solid Mid-Game Power.*

#### Level 70: The God of War
*   **Target:** ~100,000 DPS
*   **Stats:** 1000 STR, Level 70.
*   **Math:** `1000 * 7.0 = 7000 Bonus Damage`.
*   **Total Hit:** 100 (Weapon) + 7000 (STR) = **7100 Base**.
*   **Multipliers:** Damage Table (x3.5) -> ~25,000 Hit. Crit (x2.0) -> **50,000 Hit**.
*   **Speed:** Max Haste + Flurry + Triple Attack (~2.5 hits/sec).
*   **Result:** **~125,000 DPS.**
*   *Status: God Tier Achieved.*

---

## 2. Class-Specific Benefits

### A. The Tanks (Warrior, Shadowknight, Paladin)
*   **Primary Benefit: Aggro Generation.**
    *   In this era, Aggro is often capped by weapon damage. By scaling Base Damage with STR, a Tank's aggro generation scales infinitely. A 1000 STR Warrior holds aggro against 100k DPS Wizards easily.
*   **Secondary Benefit: The "Battle Caster" (Pal/SK).**
    *   **Interrupt Immunity:** `Channeling + (STR / 4)`.
    *   Tanking multiple mobs means constant hit checks. High STR ensures your lifetaps and heals *never* fizzle due to melee hits.

### B. The DPS Melee (Rogue, Berserker, Monk, Ranger)
*   **Rogue:**
    *   **Backstab Synergy:** Backstab applies a multiplier (e.g., x5) to Base Damage.
    *   *Math:* (Weapon + STR_Bonus) * BS_Mult.
    *   A 1000 STR Rogue doesn't just hit for 7000; they Backstab for **35,000 base** (Crit -> 100k+ single hits).
*   **Berserker:**
    *   **Frenzy Scaling:** As implemented, Frenzy uses Weapon + STR scaling.
    *   High STR turns Frenzy into a tactical nuke.
*   **Monk:**
    *   **Weight Limit:** Monks lose AC if over weight. `MaxWeight = STR * 10`. High STR allows Monks to loot freely without losing their defense.
    *   **Skill Damage:** Flying Kick scales directly with the STR-boosted weapon damage.

### C. The Summoners (Magician, Necromancer, Beastlord)
*   **Feature: "Sympathetic Might"**
    *   **Mechanic:** Pets inherit owner's Strength at `PET_STR_INHERITANCE` rate (currently **50%**).
    *   **Scenario:** You find a Ring of the Giant (+50 STR).
    *   **Benefit:** Your Pet gains +25 STR (50% of 50). At Level 70 (x7 multiplier), that is **+175 Base Damage** per swing for the pet.
    *   *Result:* STR is a top-tier DPS stat for Summoners.
    *   **Tunable**: Adjust `PET_STR_INHERITANCE` in `combat_balance_config.h` if pets are too strong/weak.

### D. The Priests (Cleric, Druid, Shaman)
*   **Feature: "The Battle Priest"**
    *   **Mana Conservation:** When OOM, a Priest with high STR can switch to melee.
    *   *Scenario:* A Level 70 Cleric with 800 STR hits for ~40,000 damage. This is enough to finish off bosses or grind trash without spending a drop of mana.
*   **Feature: "Unshakable Faith"**
    *   **Interrupt Resist:** Critical for healers. If you are being beaten on by 5 mobs, you need to get that Complete Heal off. High STR makes you immovable.

### E. The Pure Casters (Wizard, Enchanter)
*   **Feature: "Iron Focus"**
    *   **Stun Resistance:** `Chance to Resist Stun = STR / 50 %`.
    *   **Scenario:** A Wizard with 1000 STR has a **20% passive chance** to ignore melee stuns (Bash/Kick stuns). Combined with AA, this makes them incredibly durable in solo kiting/tanking situations.
    *   **Trade-off:** Do you take +INT for more mana pool, or +STR to ensure your Gate/Nuke never gets interrupted?

### F. The Bard (Jack of All Trades)
*   **Melee Damage:** Bards are melee combatants. Strength scales their damage just like Rangers or Rogues, allowing them to contribute significant DPS between songs.
*   **"Unbroken Melody":**
    *   **Interrupt Resist:** Bards are constantly singing while being hit (pulling, kiting, AOEing).
    *   **Mechanic:** Strength adds to the check to avoid "You miss a note" when hit.
    *   *Scenario:* A Bard with high STR can swarm kite or tank mobs without their songs dropping, maintaining CC and buffs.


---

## 3. Gear Progression (Logarithmic/Exponential)

To support this curve, gear must provide STR in increasing density:

*   **Level 1-10:** Items have +1 to +5 STR. (Total ~100).
*   **Level 40-50:** Items have +10 to +20 STR. (Total ~300).
*   **Level 60:** Items have +20 to +30 STR. (Total ~500).
*   **Level 70:** Items have +50 to +100 STR. (Total ~1000+).

This gear curve aligns perfectly with the `STR * (Level/10)` formula to produce the desired DPS explosion at the end game.

---

## 4. System Applicability

**Who Gets STR Scaling?** (Controlled by `combat_balance_config.h`)

✅ **Players**: All player characters get full STR scaling (`IsClient()`)  
✅ **Player Pets**: Inherit owner STR at `PET_STR_INHERITANCE` rate (currently 50%)  
❌ **NPCs**: Do NOT get STR scaling (`NPCS_USE_STR_SCALING = false`)  
❌ **Charmed NPCs**: Do NOT get STR scaling (`CHARMED_NPCS_USE_STR_SCALING = false`)  

**Rationale**:
- Player power fantasy: Players scale infinitely, NPCs have fixed power
- Balance: Prevents NPCs from one-shotting players with high STR
- Charm: Already powerful, doesn't need STR scaling on top

**Damage Mitigation**:
- STR damage **IS affected by armor class** (`STR_DAMAGE_AFFECTED_BY_AC = true`)
- This is raw physical damage, not armor-penetrating
- Future: DEX may get AC penetration for "skilled attacks"
