# Wisdom (WIS) - Design Document

## Core Philosophy
Wisdom represents intuition, willpower, and connection to the divine. In a Solo Server environment, Wisdom is the **Stat of Warding**. While Stamina protects the body from physical trauma, Wisdom protects the soul from magical annihilation and enhances the power of restoration.

---

## 1. "Divine Barrier" (Spell Mitigation)
*Primary Beneficiaries: Everyone (especially vs. Caster Bosses).*

Just as Armor Class (AGI/STA) mitigates melee swings, Wisdom mitigates magical damage.

### The Mechanic: Spell Shielding
*   **Concept:** Your mental fortitude allows you to "deny" the reality of the fireball hitting you.
*   **Formula:** `SpellDamageReduction% = (50 * Wisdom) / (Wisdom + 500)`.
*   **Impact:**
    *   **100 WIS:** **8.3%** Reduction.
    *   **500 WIS:** **25%** Reduction.
    *   **1000 WIS:** **33%** Reduction.
    *   *Cap:* Hard cap at 50% (requires massive WIS).
    *   *Result:* Essential for surviving AOE-heavy raid encounters solo.

---

## 2. "Potency of Spirit" (Heals & Wards)
*Primary Beneficiaries: Healers, Paladins, Shadowknights, Casters.*

Wisdom directly amplifies the output of any effect that restores health or prevents damage.

### A. Healing Power (The "Heal Mod")
*   **Formula:** `HealMod% = Wisdom / 10`.
*   **Impact:**
    *   **1000 WIS:** **+100% Healing**.
    *   *Scope:* Applies to **Spells**, **Item Clicks**, **Potions**, and **Weapon Procs** (e.g., Lifetaps).
    *   *Melee Synergy:* A Warrior using a "Vampiric Strike" weapon will heal for double the amount with high WIS.

### B. Ward Strength (Runes/Shields)
*   **Formula:** `RuneMod% = Wisdom / 10`.
*   **Impact:**
    *   **1000 WIS:** **+100% Absorb Strength**.
    *   *Scenario:* An Enchanter's "Rune of Protection" (Base 1000 HP) becomes a **2000 HP** shield.
    *   *Solo Reality:* This allows "Squishy" classes to build massive buffer pools to survive burst damage.

---

## 3. "Iron Will" (Crowd Control Resistance)
*Primary Beneficiaries: Everyone.*

In a Solo game, getting Stunned, Feared, or Charmed usually means death. Wisdom is your defense against loss of control.

### The Mechanic: Status Resistance
*   **Formula:** `ResistChance% = (Wisdom * Level) / 1000`.
*   **Impact:**
    *   **Level 70 (1000 WIS):** **70% Chance** to completely ignore a Stun, Fear, Charm, or Silence effect.
    *   *Note:* This is separate from Magic Resistance. Even if the spell lands, your mind might refuse to succumb to the effect.

---

## 4. "The Reservoir" (Mana Pool)
*Primary Beneficiaries: Casters, Healers, Hybrids.*

Wisdom and Intelligence share the burden of expanding the mind's capacity.

### The Formula
*   **Rule:** Mana is calculated using the **Sum of INT + WIS**, but weighted by class.
*   **Formula:** `MaxMana = ((INT + WIS) * Level) * ClassMultiplier`.
*   *Result:*
    *   **Clerics:** Stack WIS for Mana.
    *   **Wizards:** Stack INT for Mana.
    *   **Paladins/Rangers:** Can stack either (or both) to grow their pool.

---

## 5. Class-Specific Benefits

### A. The Healers (Cleric, Druid, Shaman)
*   **The Archon:**
    *   WIS is their "Strength". It makes their heals massive.

#### Class Masteries (WIS)
*   **Cleric - "Divine Retribution":**
    *   **Holy Smite:** Wisdom adds raw **Holy Damage** to all Undead Nukes (making them work on *all* enemies) and adds a "Holy Shock" proc to melee attacks.
    *   **Aegis:** Overhealing (healing a full HP target) converts 50% of the heal into a temporary **Absorb Shield**.
*   **Shaman - "Feral Avatar":**
    *   **Battle Shaman:** Wisdom grants a massive bonus to **Melee Damage** and **Proc Rate**, allowing them to fight alongside their pet.
    *   **Venomous Spirit:** Wisdom increases the damage of Poison/Disease DoTs specifically.
*   **Druid - "Nature's Wrath":**
    *   **Thorns:** Wisdom scales the damage of **Damage Shields** exponentially.

### B. The Hybrids (Paladin, Shadowknight, Ranger, Beastlord)
*   **The Battle-Sage:**
    *   **Paladins:** Massive heals (Lay on Hands scales with WIS) and **Holy Damage** on melee swings.
    *   **Shadowknights:** Massive Lifetaps.
    *   **Beastlords:** Stronger Pet Heals and Slows (Resist check).

### C. The Casters (Enchanter, Necro, Magician, Wizard)
*   **The Warded Mage:**
    *   They rely on **Runes/Shields** to survive. WIS doubles the effectiveness of these shields.
    *   *Trade-off:* Do I stack INT for Damage, or WIS for Survival (Shields + CC Resist)?

### D. The Pure Melee (Warrior, Rogue, Monk, Berserker)
*   **The Zen Warrior:**
    *   **Survival:** Magic Mitigation is their only defense against spells (since AC doesn't help).
    *   **Sustain:** Boosts the effectiveness of **Healing Potions** and **Lifetap Procs**.
    *   **Discipline:** Reduces duration of Stuns (Iron Will), keeping them in the fight.
