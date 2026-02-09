# Beastlord - Class Design Document

## 1. Class Fantasy: "The Primal Duo"
The Beastlord is the master of the wild, fighting in perfect synchronization with their Warder. They are a hybrid of Monk and Shaman. In a Solo Server environment, they are the **Synergy Tanks**. Neither the Beastlord nor the Warder is strong enough alone, but together they are unstoppable.

*   **Primary Stat:** **Wisdom (WIS)**
*   **Class Mastery:** **"Feral Bond"**
    *   **Pack Leader:** Wisdom scales **Pet Stats** and **Pet Aggro Generation**.
    *   **Primal Essence:** Wisdom increases the proc rate of "Feral" procs (Slows, DoTs, Heals).

---

## 2. Combat Profile

### A. Dealing Damage (The Pack)
Beastlords deal damage through a flurry of claws and spells.
*   **Primary Source:** **Melee** (Hand-to-Hand / Piercing) + **Pet DPS**.
*   **Secondary Source:** **DoTs** and **Nukes**.
*   **Mechanic:** **"Symbiosis"**. When the Beastlord hits, the Pet gains a buff. When the Pet hits, the Beastlord gains mana/health.

### B. Taking Damage (Pet Tanking)
The Warder is the tank. The Beastlord is the off-tank.
*   **Mitigation:** Leather Armor (Medium).
*   **Survival:** **Slows**. Beastlords have access to Shaman-line slows, reducing enemy DPS by 50-75%.
*   **Recovery:** **Chloroplast** (Regen) and **Heals**.

---

## 3. The Toolkit

### A. Damage Tools
1.  **Kick / Strike:** Monk-like melee attacks.
2.  **DoTs:** Disease and Poison DoTs (weaker than Shaman/Necro).
3.  **Nukes:** Cold/Ice nukes.
4.  **Bestial Fury (Disc):** Massive attack speed and damage boost for both Master and Pet.

### B. The Warder
1.  **Warder Pet:** A unique pet based on the Beastlord's race (Bear, Tiger, Wolf, etc.).
2.  **Spirit of the Warder:** Buffs the pet's stats.

### C. Utility
1.  **Slow:** Reduces enemy attack speed. The single most important solo tool.
2.  **Haste:** Buffs attack speed for the group/pet.
3.  **Paragon of Spirit:** Regenerates Mana and HP for the group.
4.  **Spirit of Wolf:** Movement speed.

### D. Area Effect (AE) Potential
*   **Viral Claws:** (If using Viral mechanic) Melee attacks spread a disease DoT to nearby enemies.
*   **Roar of Thunder:** PBAOE Stun/Damage.

---

## 4. Solo Strategy
*   **Level 1-30:** Melee alongside pet. Heal as needed.
*   **Level 30-60:** **Slow Tanking**. Pull with Slow. Send Pet. Attack from behind. If Pet gets low, heal it. If you get low, let Pet tank.
*   **Level 70+:** **Raid Boss Soloing**. With high-tier Slows and a raid-geared Pet, you can tank almost anything.

---

## 5. Class Scores (1-10)

| Category | Score | Notes |
| :--- | :--- | :--- |
| **Single Target DPS** | **8/10** | Combined Master + Pet DPS is very high. |
| **AE DPS** | **4/10** | Limited. Mostly single target focus. |
| **Tankiness** | **8/10** | Pet is a great tank, and Slow makes enemies hit like noodles. |
| **Sustainability** | **9/10** | Paragon (Mana/HP Regen) + Slows + Heals. |
| **Utility** | **8/10** | Slow, Haste, Mana Regen. A complete package. |
| **Pet Power** | **9/10** | Almost as strong as Magician pets, but with more utility. |

---

## 6. Summary
The Beastlord is the **Self-Contained Group**. You have a Tank (Warder), a Healer (You), a Slower (You), and DPS (Both). You don't need anyone else.

---

## 6. Summary
The Beastlord is the **Self-Contained Group**. You have a Tank (Warder), a Healer (You), a Slower (You), and DPS (Both). You don't need anyone else.

---

## 7. The Big Three: Core Abilities

These abilities harness the primal bond between beast and master, scaling with wisdom and level.

### I. [Offensive] Bestial Frenzy
*   **Description:** The Beastlord and their Warder enter a state of shared bloodlust. Both the Master and the Pet gain massive **Attack Speed** and **Critical Hit Chance**.
*   **Scaling:** $Haste = (\text{Level} \times 0.5) + (\text{WIS} \times 0.2)\%$. $CritChance = (\text{Level} \times 0.2) + (\text{DEX} \times 0.1)\%$.
*   **Cooldown:** 60 Seconds.
*   **Duration:** 12 Seconds.
*   **Solo Use:** Massive burst for taking down hard targets.
*   **Synergy:** **Pet/Master Multiplier**. While this is primarily a self-buff, it ensures the Beastlord's pet is a top-tier damage dealer in any group.
*   **Stats:** Scales with **WIS** (Primal Connection).

### II. [Defensive] Protector's Symbiosis
*   **Description:** Forges a deep defensive link between the Beastlord and the Warder. A portion of damage taken is shared, and both receive a continuous **Heal Over Time**.
*   **Scaling:** $DamageShare = 50\%$. $Heal = (\text{Level}^{1.5} \times 1.0) + (\text{WIS} \times 5)$.
*   **Cooldown:** 90 Seconds.
*   **Duration:** 10 Seconds.
*   **Solo Use:** Essential for surviving dual-tanking situations or heavy AOE damage.
*   **Group/Synergy:** None (Survival focus).

### III. [Utility/Synergy] Primal Essence
*   **Description:** The Beastlord releases a wave of primal energy. All allies gain increased **Stat Caps** and **Health/Mana Regeneration**.
*   **Scaling:** $Regen = (\text{Level} \times 0.5) + (\text{WIS} \times 0.1)$. $StatBase = (\text{Level} \times 1)$.
*   **Cooldown:** 120 Seconds.
*   **Duration:** 12 Seconds.
*   **Solo Use:** Sustains your own resources during long, difficult encounters.
*   **Synergy:** **Resource Multiplier**. This is a powerful "recovery" buff for a group, helping everyone stay topped off while also raising their potential peak power by increasing stat caps.
*   **Stats:** Scales with **WIS** (Survival Instinct).
