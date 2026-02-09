# Paladin - Class Design Document

## 1. Class Fantasy: "The Holy Crusader"
The Paladin is the shield of the innocent and the hammer of justice. They are the hybrid of Warrior and Cleric. In a Solo Server environment, they are the **Stun Tanks**. They survive not just by armor, but by denying the enemy the ability to attack at all.

*   **Primary Stat:** **Wisdom (WIS)**
*   **Class Mastery:** **"Righteousness"**
    *   **Holy Shield:** Wisdom adds **Block Chance**. Successful blocks deal **Holy Damage** back to the attacker.
    *   **Divine Interruption:** Stuns work on enemies up to 5 levels higher than normal, and "Stun Immunity" on bosses is treated as "Resistant" (can still land with high CHA/WIS).

---

## 2. Combat Profile

### A. Dealing Damage (The Smiter)
Paladins deal consistent melee damage augmented by holy magic.
*   **Primary Source:** **1H Sword + Shield**.
*   **Secondary Source:** **Retribution**. Blocking an attack reflects Holy Damage.
*   **Mechanic:** **"Slay Undead"**. A passive chance to deal massive critical damage to undead targets.

### B. Taking Damage (Shield Tanking)
Paladins are the masters of the Shield.
*   **Mitigation:** High AC (Plate) + **Super Block**. Blocking an attack reduces damage significantly, bypassing the standard mitigation caps (e.g., blocking reduces damage by 90% instead of the usual 75% cap).
*   **Denial:** **Stuns**. A stunned mob deals 0 DPS.
*   **Survival:** **Lay on Hands**. Instantly heals the Paladin to full. (Scales with WIS).

---

## 3. The Toolkit

### A. Damage Tools
1.  **Crusader's Strike (Nuke):** Fast-casting Holy nuke (Bonus vs Undead).
2.  **Holy Forge (Disc):** Increases chance to Crit and Slay Undead.
3.  **Yaulp:** Stamina/Mana Regen buff.
4.  **Bash:** Shield attack that stuns.

### B. Mitigation Tools
1.  **Cease/Desist (Stuns):** Short duration stuns that generate massive aggro.
2.  **Divine Aura:** Invulnerability.
3.  **Armor of the Crusader:** AC and HP buff.

### C. Utility
1.  **Root:** Keeps enemies in place. (Root-Jousting).
2.  **Pacify:** Reduces aggro radius for splitting pulls.
3.  **Divine Intervention:** A self-buff that heals the Paladin if they take a fatal blow (Self-Rez/Cheat Death).
4.  **Cure Disease/Poison:** Self-cleansing.

### D. Area Effect (AE) Potential
*   **Radiant Block:** When the Paladin blocks an attack, a burst of light damages all nearby enemies.
*   **Weapon Procs:** High frequency weapon procs that can strike multiple targets (if equipped with specific weaponry).

---

## 4. Solo Strategy
*   **Level 1-30:** Face tank. Use Yaulp.
*   **Level 30-60:** **Undead Hunting**. Go to Unrest, Lower Guk, or Crypt of Dalnir. You will destroy everything.
*   **Level 70+:** **Stun Locking**. You pull a boss. You rotate your 3 Stun spells and Bash. The boss only gets to swing half the time. You heal the damage with Lay on Hands or HoTs.

---

## 5. Class Scores (1-10)

| Category | Score | Notes |
| :--- | :--- | :--- |
| **Single Target DPS** | **6/10** | Average vs Living. **10/10** vs Undead. |
| **AE DPS** | **4/10** | Limited AOE options. |
| **Tankiness** | **9/10** | Plate Armor + Stuns + Self Heals. |
| **Sustainability** | **7/10** | Mana is the bottleneck. No Canni/Lich. |
| **Utility** | **7/10** | Root, Pacify, Rez, Cures. Very versatile. |
| **Pet Power** | **0/10** | No pet. |

---

## 6. Summary
The Paladin is the **Specialist Tank**. Against Undead, you are a god. Against the living, you are an unmovable object that interrupts every spell and attack the enemy tries to make.

---

## 7. The Big Three: Core Abilities

These abilities define the Paladin's role as a hybrid protector and holy warrior, scaling with level and faith.

### I. [Offensive] Divine Retribution
*   **Description:** A powerful blast of holy energy that deals damage and stuns the target. Deals triple damage to Undead.
*   **Scaling:** $Damage = (\text{Level}^{2.0} \times 1.2) + (\text{WIS} \times 10)$.
*   **Cooldown:** 45 Seconds.
*   **Duration:** 6 Seconds (Stun).
*   **Solo Use:** Burst damage for undead hunting or interrupting dangerous boss spells.
*   **Group/Synergy:** High Threat.

### II. [Defensive] Radiant Bulwark
*   **Description:** A massive enhancement to the Paladin's shield, increasing AC and reflecting damage back to attackers.
*   **Scaling:** $AC = (\text{Level} \times 5) + (\text{WIS} \times 0.5)$. $Reflect = (\text{Level}^{1.5} \times 0.5)$.
*   **Cooldown:** 120 Seconds.
*   **Duration:** 12 Seconds.
*   **Solo Use:** Essential for face-tanking hard-hitting bosses.
*   **Group/Synergy:** Provides a small portion of the AC bonus to nearby group members.

### III. [Utility/Synergy] Sanctified Ground
*   **Description:** The Paladin blesses the area around them. All allies within the aura gain increased **Healing Received**.
*   **Scaling:** $Bonus = (\text{Level} \times 0.5) + (\text{WIS} \times 0.05)\%$ increased healing received.
*   **Cooldown:** 90 Seconds.
*   **Duration:** 12 Seconds.
*   **Solo Use:** Makes your self-heals and Lay on Hands much more powerful.
*   **Synergy:** **Sustainability Multiplier**. This makes Clerics, Druids, and Shamans significantly more mana-efficient when healing the group. Also boosts the self-healing of Shadowknights and Berserkers.
*   **Stats:** Scales with **WIS** (Divine Connection).
*   **Synergy Value:** High. Foundational for surviving high-damage raid encounters in a group.
