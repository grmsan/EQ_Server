# Necromancer - Class Design Document

## 1. Class Fantasy: "The Master of Death"
The Necromancer manipulates the forces of life and death. They rot their enemies from the inside, command the undead, and fuel their magic with their own blood. In a Solo Server environment, they are the **Gods of Solo**.

*   **Primary Stat:** **Intelligence (INT)**
*   **Class Mastery:** **"Lich Mastery"**
    *   **Dark Ritual:** Intelligence increases the efficiency of **Lich** (HP to Mana conversion) and **Lifetap** (Damage to HP conversion).
    *   **Plague Bearer:** Intelligence increases the duration and damage of **DoTs**.

---

## 2. Combat Profile

### A. Dealing Damage (The Rot)
Necromancers kill slowly but surely.
*   **Primary Source:** **Damage over Time (DoT)** (Disease, Poison, Fire, Magic).
*   **Secondary Source:** **Undead Pet**.
*   **Mechanic:** **"Fear Kiting"**. The Necromancer snares the target, fears it (making it run away), and lets the DoTs and Pet kill it while it flees helplessly.

### B. Taking Damage (Life Manipulation)
Necromancers wear Cloth, but they have massive health pools to fuel their dark magic.
*   **Health Pool:** **High Stamina Scaling**. Necromancers gain HP from Stamina at a rate similar to tanks (1.4x scaling), giving them the largest HP pool of any caster. This is necessary to fuel their Lich form and blood magic.
*   **Mitigation:** Bone Shield (Absorb).
*   **Survival:** **Lifetap**. Drains health from the enemy to heal the Necromancer.
*   **Escape:** **Feign Death**. The ultimate survival tool.

---

## 3. The Toolkit

### A. Damage Tools
1.  **Venom of the Snake (Poison DoT):** Fast ticking damage.
2.  **Boil Blood (Disease DoT):** Long duration damage.
3.  **Ignite Blood (Fire DoT):** High damage, lowers AC.
4.  **Lifetap:** Direct damage that heals the caster.

### B. The Undead
1.  **Skeleton Pet:** A solid tank/DPS pet. (Rogue/Warrior variants).
2.  **Wake the Dead:** Summons temporary pets from nearby corpses.

### C. Utility
1.  **Lich Form:** Converts HP into Mana every tick. Combined with Lifetaps/Regen, this gives the Necromancer **Infinite Mana**.
2.  **Feign Death:** Drop aggro instantly.
3.  **Resurrection:** Can resurrect players (consuming an Essence Emerald).
4.  **Summon Corpse:** Drag a player's corpse to their location.

### D. Area Effect (AE) Potential
*   **Epidemic:** (If using Viral mechanic) Spreads DoTs to nearby enemies.
*   **Corpse Explosion:** Detonates a corpse for massive AOE damage.

---

## 4. Solo Strategy
*   **Level 1-30:** Pet tanking + Lifetaps.
*   **Level 30-60:** **Fear Kiting**. Snare -> Fear -> DoT -> Pet Attack. The mob runs away and dies. You take 0 damage.
*   **Level 70+:** **Aggro Kiting**. Use your superior movement speed and DoTs to kite massive trains of mobs.

---

## 5. Class Scores (1-10)

| Category | Score | Notes |
| :--- | :--- | :--- |
| **Single Target DPS** | **8/10** | Ramps up slowly, but very high total damage. |
| **AE DPS** | **7/10** | Strong with viral DoTs or Corpse Explosion. |
| **Tankiness** | **6/10** | Cloth armor, but Lifetaps make them surprisingly durable. |
| **Sustainability** | **10/10** | Infinite Mana (Lich) + Infinite HP (Lifetap). |
| **Utility** | **10/10** | Feign Death, Rez, Corpse Summon. |
| **Pet Power** | **7/10** | Strong, but not as strong as Magician pets. |

---

## 6. Summary
The Necromancer is the **King of Solo**. You have the perfect toolkit: Snare, Fear, Pet, Self-Heal, Mana-Regen, and Feign Death. You can kill anything that can be snared or feared.

---

## 7. The Big Three: Core Abilities

These abilities tap into the ultimate power over life and death, scaling with dark intelligence and level.

### I. [Offensive] Blighted Apocalypse
*   **Description:** Instantly detonates all active Damage-over-Time effects on the target, dealing their remaining damage plus a massive bonus.
*   **Scaling:** $BonusDamage = (\text{Level}^{2.1} \times 1.4) + (\text{INT} \times 15)$.
*   **Cooldown:** 45 Seconds.
*   **Duration:** Instant.
*   **Solo Use:** The perfect "finisher" for a target that has been rotted down.
*   **Group/Synergy:** None (Pure DPS).

### II. [Defensive] Vampiric Embrace
*   **Description:** The Necromancer enter a state of total vampirism. Every spell that deals damage also heals the Necromancer for a percentage of that damage.
*   **Scaling:** $HealChance = (\text{Level} \times 1) + (\text{INT} \times 0.1)\%$. $HealAmount = (\text{Level}^{1.5} \times 0.5)$.
*   **Cooldown:** 120 Seconds.
*   **Duration:** 12 Seconds.
*   **Solo Use:** Combined with Lich form, this ensures the Necromancer stays at 100% health even while losing it to mana regen.
*   **Group/Synergy:** None (Survival focus).

### III. [Utility/Synergy] Soul Shackle
*   **Description:** Chains the target's soul, weakening its connection to the physical and magical nodes of the world. Reduces the target's **All Resistances** and increases **Damage from Spells** for all allies.
*   **Scaling:** $ResistReduction = (\text{Level} \times 0.5) + (\text{INT} \times 0.1)$. $DmgBonus = (\text{Level} \times 0.2)\%$.
*   **Cooldown:** 90 Seconds.
*   **Duration:** 12 Seconds.
*   **Solo Use:** Makes your DoTs and Lifetaps impossible to resist.
*   **Synergy:** **Magic Penetration Multiplier**. This is a massive "Raid-wide" buff. It ensures that every caster's spells land for full damage, even against high-resistance bosses. It pairs exceptionally well with the Druid's `Primal Attunement`.
*   **Stats:** Scales with **INT** (Necrotic Insight).
*   **Synergy Value:** High. Makes every caster in the raid feel significantly more powerful.
