# Enchanter - Class Design Document

## 1. Class Fantasy: "The Reality Bender"
The Enchanter is the master of the mind and the weaver of illusions. They do not fight with muscle or steel; they fight with the enemy's own strength. In a Solo Server environment, the Enchanter is the **Puppet Master**. They turn the strongest monster in the zone into their personal bodyguard while they watch from behind a wall of magical force.

*   **Primary Stat:** **Charisma (CHA)**
*   **Class Mastery:** **"Dominion"** - Charisma grants Permanent Charm, Unbreakable Will, and Soul Link (Stat Transfer to Pet).

---

## 2. Combat Profile

### A. Dealing Damage (Proxy Warfare)
Enchanters deal damage by forcing others to do it for them.
*   **Primary Source:** **Charm Pet**. The DPS of an Enchanter is effectively the DPS of the strongest mob in the zone + 50% (Soul Link).
*   **Secondary Source:** **Mind DoTs** (Choke, Suffocate) and **Psychic Nukes** (Chromatic damage).
*   **Mechanic:** **"Feedback"**. Damage taken by the Enchanter reflects back to the attacker as Mana Drain or Stun.

### B. Taking Damage (Mana Tanking)
Enchanters are physically frail but magically impregnable.
*   **Mitigation:** **Runes**. Enchanters layer "Rune" spells (Absorb Shields) that take damage instead of HP.
*   **Avoidance:** **Crowd Control**. If the enemy is Stunned, Mezed, or Charmed, they deal 0 damage.
*   **Survival:** If the Rune breaks and the Pet dies, the Enchanter dies. High Risk, High Reward.

---

## 3. The Toolkit

### A. Damage Tools
1.  **Charm (Spell):** Takes control of an NPC. With High CHA, this is permanent and buffs the pet.
2.  **Animation (Pet):** A summoned pet (Sword/Shield) for when no charmable mobs are around. (Buffed by CHA).
3.  **Mind Nuke:** Direct damage that also lowers Magic Resist.
4.  **Feedback (Buff):** Damage Shield that drains mana/stuns attackers.

### B. Mitigation Tools
1.  **Rune of Protection:** Absorbs X amount of damage. (Scales with WIS/INT).
2.  **Bedlam:** A "Spell Shield" that absorbs Magic Damage specifically.
3.  **Displacement:** A short-range teleport (Blink) to drop aggro or gain distance.

### C. Utility
1.  **Mesmerize (AE/Single):** Puts enemies to sleep. The ultimate "Pause Button".
2.  **Clarity (Buff):** Massive Mana Regen. Essential for sustaining the Rune/Nuke rotation.
3.  **Slow/Haste:** Drastically reduces enemy DPS (Slow) and doubles Pet DPS (Haste).
4.  **Invisibility/Illusions:** Perfect for navigating dangerous zones to find the perfect pet.

### D. Area Effect (AE) Potential
*   **PBAOE Stun:** Stuns all enemies around the Enchanter (Color Spray).
*   **PBAOE Mez:** Sleeps all enemies around the Enchanter.
*   **AE DoTs:** Gravity Flux (Damage + Fling).

---

## 4. Solo Strategy
*   **Level 1-30:** Use the Animation pet and Nukes. Runes keep you safe.
*   **Level 30-60:** **Charm Kiting**. Charm a mob, give it a torch (haste), and have it destroy its friends. Keep Runes up in case it breaks.
*   **Level 70+:** **God Mode**. You Charm a Raid Trash mob. It has 500k HP. You give it your stats (Soul Link). It now has 750k HP and hits for 5k. You stand back, keep Runes up, and loot the bodies.

---

## 5. Class Scores (1-10)

| Category | Score | Notes |
| :--- | :--- | :--- |
| **Single Target DPS** | **9/10** | Dependent on finding a good pet. If you have a good pet, you are top tier. |
| **AE DPS** | **5/10** | Good control (Stuns), but damage is mostly single-target. |
| **Tankiness** | **4/10** | Physical (HP/AC) is low. |
| **Magical Tankiness** | **10/10** | Runes + CC make them immune to damage if played perfectly. |
| **Sustainability** | **10/10** | Clarity + Mana Drain = Infinite Mana. |
| **Utility** | **10/10** | The King of Utility. Slow, Haste, CC, Mana Regen. |
| **Pet Power** | **10/10** | The strongest pet in the game (because it IS the enemy). |

---

## 6. Summary
The Enchanter is the **High Skill, Infinite Ceiling** solo class. You are as strong as your knowledge of the zone. If you know which mob hits the hardest, you win. If you let your Rune drop or your Charm break without a backup plan, you die.

---

## 7. The Big Three: Core Abilities

These abilities allow the Enchanter to manipulate the flow of battle, scaling with charisma and level.

### I. [Offensive] Echoes of Power
*   **Description:** The Enchanter amplifies the magical resonance of all allies. Increases **Spell Damage** and **Spell Penetration**.
*   **Stats:** Scales with **CHA** and **Level**.
*   **Scaling:** $DamageBonus = (\text{Level} \times 1.0) + (\text{CHA} \times 0.1)\%$
*   **Cooldown:** 60 Seconds.
*   **Duration:** 12 Seconds.
*   **Synergy:** **Magic Penetration**. Reduces enemy Magic Resist by 50. This ensures that Wizards, Magicians, and even the Enchanter's own mind-spells land for full damage.

### II. [Utility] Mental Fracture
*   **Description:** Crushes the target's will, significantly reducing their **Magic Resistance** and **Attack Speed**.
*   **Stats:** Scales with **CHA** and **Level**.
*   **Scaling:** $Slow = (\text{Level} \times 0.5) + (\text{CHA} \times 0.05)\%$.
*   **Cooldown:** 45 Seconds.
*   **Duration:** 12 Seconds.
*   **Synergy:** **Enfeeble**. Stackable with standard slows. Makes high-end bosses hit significantly slower, protecting the tank/pet.

### III. [Defensive] Mind Ward
*   **Description:** Creates a reactive shield that absorbs all damage and restores Mana upon taking a hit.
*   **Stats:** Scales with **CHA** and **Level**.
*   **Scaling:** $Absorb = (\text{Level}^{2.0} \times 1.2) + (\text{CHA} \times 15)$.
*   **Cooldown:** 120 Seconds.
*   **Duration:** 8 Seconds.
*   **Synergy:** None (Pure Survival). Allows the Enchanter to survive a target switch or a charm break.
