# Stamina (STA) - Design Document

> This document defines STA's gameplay role and mechanic shapes. Exact HP, regen, mitigation, class multiplier, environmental resistance, ramp, and cap values should be hotfixable through `zone/combat_balance.ini`; compiled constants are fallback defaults.

## Core Philosophy
Stamina represents the body's energy reserves, resilience, and capacity to endure hardship. In a Solo Server environment, Stamina is the **Stat of Sustainability**. It determines not just how much damage you can take (HP), but how long you can keep fighting (Regen/Endurance) before collapsing.

---

## 1. The "Iron Constitution" (Health Pool)
*Primary Beneficiaries: Tanks, but essential for everyone.*

Classic EQ HP calculation is often conservative. For a solo player facing raid bosses, we need a larger buffer.

### The Formula
```cpp
// Classic-ish but scaled up
MaxHP = BaseHP + (Stamina * LevelMultiplier * ClassMultiplier);
```
*   **Class Multiplier:**
    static constexpr float WARRIOR_MULTIPLIER = 1.6f;
    static constexpr float CLERIC_MULTIPLIER = 1.1f;
    static constexpr float PALADIN_MULTIPLIER = 1.5f;
    static constexpr float RANGER_MULTIPLIER = 1.2f;
    static constexpr float SHADOWKNIGHT_MULTIPLIER = 1.4f;
    static constexpr float DRUID_MULTIPLIER = 1.0f;
    static constexpr float MONK_MULTIPLIER = 1.1f;
    static constexpr float BARD_MULTIPLIER = 1.1f;
    static constexpr float ROGUE_MULTIPLIER = 1.0f;
    static constexpr float SHAMAN_MULTIPLIER = 1.1f;
    static constexpr float NECROMANCER_MULTIPLIER = 1.3f;    // Higher HP for caster
    static constexpr float WIZARD_MULTIPLIER = 0.8f;
    static constexpr float MAGICIAN_MULTIPLIER = 0.8f;
    static constexpr float ENCHANTER_MULTIPLIER = 0.9f;
    static constexpr float BEASTLORD_MULTIPLIER = 1.2f;
    static constexpr float BERSERKER_MULTIPLIER = 1.0f;

### Progression Goal
*   **Level 1:** ~100 HP (Survivable start).
*   **Level 70 (Tank):** ~30,000 HP
*   **Level 70 (Caster):** ~15,000 HP.
*   *Why:* Bosses will hit hard. You need the pool to survive the spike damage while your regen kicks in.

---

## 2. "Undying Vitality" (Regeneration)
*Primary Beneficiaries: Everyone.*

In a Solo Server environment where you have no healer and no enchanter for mana, **Self-Sufficiency is King**. The regeneration rates must scale to match the massive HP/Mana pools of the end-game (100k+ HP).

### A. Health Regeneration (The Wolverine Factor)
*   **Concept:** High Stamina closes wounds rapidly. Since you are taking hits constantly (Solo), you need to heal constantly.
*   **Formula:** `HP_Regen_Bonus = (Stamina * Level) / 10`.
*   **Impact:**
    *   **Level 1 (100 STA):** `(100 * 1) / 10` = **+10 HP/tick**. (10% of ~100 HP). *Great start.*
    *   **Level 70 (1000 STA):** `(1000 * 70) / 10` = **+7,000 HP/tick**. (7% of ~100k HP).
    *   *Result:* You recover your full health bar in ~15 ticks (90 seconds) of combat. This replaces the need for a dedicated healer for general content.

### B. Mana Regeneration (The Arcane Well)
*   **Concept:** Physical endurance fuels mental focus.
*   **Formula:** `Mana_Regen_Bonus = (Stamina * Level) / 50`.
*   **Impact:**
    *   **Level 1 (100 STA):** `(100 * 1) / 50` = **+2 Mana/tick**.
    *   **Level 70 (1000 STA):** `(1000 * 70) / 50` = **+1,400 Mana/tick**.
    *   *Result:* A high-STA caster generates enough mana to sustain a rotation, effectively replacing "Clarity" buffs.

### C. Endurance Regeneration (The Infinite Warrior)
*   **Concept:** Fuel for the 100k DPS engine.
*   **Formula:** `Endurance_Regen_Bonus = (Stamina * Level) / 20`.
*   **Impact:**
    *   **Level 70 (1000 STA):** **+3,500 End/tick**.
    *   *Result:* Allows for near-permanent uptime on Disciplines.

---

## 3. "Thick Skin" (Damage Mitigation)
*Primary Beneficiaries: Everyone (Since everyone tanks).*

Since every class is a tank in Solo play, Stamina provides a flat damage reduction to ensure "squishy" classes don't get one-shot before their regen kicks in.

### The Mechanic
**Flat Damage Reduction:** A portion of incoming melee damage is simply absorbed.
*   **Formula:** `Damage_Reduction = (Stamina * Level) / 100`.
*   **Constraint:** Damage cannot be reduced below **25%** of the original hit. (You always feel *something*).
*   **Impact:**
    *   **Level 70 (1000 STA):** **-700 Damage per hit** (Potential).
    *   *Scenario A (Big Hit):* A boss hits for 2000. Reduction is 700. You take **1300 damage**.
    *   *Scenario B (Small Hit):* A swarm mob hits for 500. Reduction *would* be 700, but is capped at 75% of 500 (375). You take **125 damage**.

---

## 4. Class-Specific Benefits (Solo Context)

### A. The Tanks (Warrior, Shadowknight, Paladin)
*   **The Juggernaut:**
    *   **Role:** Face-tanking Raid Bosses.
    *   **Benefit:** With 1.5x HP scaling and massive Regen, they can survive sustained DPS checks that would flatten other classes.
    *   **Solo Reality:** They don't kill as fast as Rogues, but they *cannot die* as long as they aren't one-shot.

### B. The Melee DPS (Rogue, Berserker, Monk, Ranger, Bard)
*   **The Drain Tank:**
    *   **Role:** Kill before being killed.
    *   **Benefit:** They take full damage (lower mitigation/HP than tanks), but the **Endurance Regen** allows them to spam stuns, trips, and massive attacks to mitigate incoming DPS via crowd control and murder.
    *   **Solo Reality:** Stamina is their "Time Limit" extender.

### C. The Healers (Cleric, Druid, Shaman)
*   **The Battle Medic:**
    *   **Role:** Attrition Warfare.
    *   **Benefit:** Since they must tank while casting, the **HP Regen** + **Mitigation** acts as a passive "HoT" (Heal over Time), allowing them to focus mana on nukes or debuffs instead of spamming heals on themselves.
    *   **Solo Reality:** High STA turns a Cleric into a Paladin who casts better spells.

### D. The Casters (Wizard, Magician, Enchanter, Necromancer)
*   **The Kiting Tank:**
    *   **Role:** Control and Blast.
    *   **Benefit:**
        *   **Pets:** If the pet dies, the Caster is next. High HP/Mitigation buys time to resummon.
        *   **Mana:** The massive Mana Regen from STA is the only way to sustain the 100k DPS burn needed to kill bosses solo.
    *   **Solo Reality:** You aren't "squishy" anymore. You are a "Light Tank" with nuclear weapons.

---

## 5. Environmental Resistance (The "Hardy" Trait)
*   **Poison/Disease Resist:** `Resist_Bonus = Stamina / 5`.
*   **Drowning:** High STA increases breath timer significantly.
*   **Food/Drink:** High STA reduces consumption rate (Quality of Life).

## Summary
Stamina transforms from a passive "HP Bar" stat into an active **Resource Engine**. It fuels the fight, whether that fuel is Blood (HP), Sweat (Endurance), or Focus (Mana).
