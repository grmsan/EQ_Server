# Charisma (CHA) - Design Document

## Core Philosophy
Charisma represents force of personality, physical attractiveness, and supernatural luck. In a Solo Server environment, Charisma is the **Stat of Influence**. It bends the world to your favor—enemies hesitate to strike, merchants offer their best wares, pets fight harder, and the very laws of probability shift in your direction.

---

## 1. "Fortune's Favor" (Luck & Loot)
*Primary Beneficiaries: Everyone.*

On a Solo Server, getting the drop you need is paramount. Charisma directly influences the "Random Number Generator" (RNG).

### The Mechanic: Rare Loot Chance
*   **Formula:** `RareDropBonus% = Charisma / 20`.
*   **Impact:**
    *   **100 CHA:** **+5%** Chance for rare tables.
    *   **1000 CHA:** **+50%** Chance.
    *   *Result:* High CHA characters gear up significantly faster.

---

## 2. "Commanding Presence" (Pet Power)
*Primary Beneficiaries: Magician, Necromancer, Beastlord, Enchanter, Shaman.*

Your force of will inspires (or terrifies) your minions into fighting beyond their normal limits.

### The Mechanic: Pet Stat Scaling
*   **Formula:** `PetStatBonus% = Charisma / 10`.
*   **Impact:**
    *   **1000 CHA:** Your pet gains **+100% HP** and **+100% Damage**.
    *   *Solo Reality:* A Magician's Earth Pet becomes a Raid Tank. A Necromancer's Rogue Pet becomes a DPS machine.
    *   *Note:* This makes CHA a primary stat for Pet Classes, competing with INT.

---

## 3. "Disarming Beauty" (Damage Mitigation)
*Primary Beneficiaries: Everyone.*

Enemies find it difficult to strike you with full force. Whether it's hesitation, distraction, or awe, their attacks lack conviction.

### The Mechanic: Enemy Damage Debuff
*   **Concept:** You don't take less damage (like STA); the enemy *deals* less damage.
*   **Formula:** `EnemyDamageReduction% = (Charisma * Level) / 2500`.
*   **Impact:**
    *   **Level 70 (1000 CHA):** Enemies hit you for **-28% Damage**.
    *   *Stacking:* This stacks multiplicatively with Stamina's flat mitigation and Wisdom's spell shielding.

---

## 4. "Beguiling Magic" (Resist Penetration)
*Primary Beneficiaries: Casters, Bards.*

Your spells are not just energy; they are persuasive. You convince the target's soul to accept the magic.

### The Mechanic: Resist Debuff
*   **Formula:** `ResistPenetration = Charisma / 10`.
*   **Impact:**
    *   **1000 CHA:** **-100 Enemy Resistance**.
    *   *Synergy:* Stacks with Dexterity's "Spell Penetration".
    *   *Result:* A Caster with high DEX and CHA effectively ignores 200 points of resistance, making spells land on even the most resistant raid bosses.

---

## 5. "The Unexpected Strike" (Crit Bonus)
*Primary Beneficiaries: Melee, Hybrids.*

Enemies caught staring or hesitating leave openings.

### The Mechanic: Opportunity Crits
*   **Formula:** `BonusCritChance% = Charisma / 100`.
*   **Impact:**
    *   **1000 CHA:** **+10% Flat Crit Chance**.
    *   *Note:* This is a smaller bonus compared to DEX, but it pushes you over the 100% cap, feeding into the **Critical Damage Overflow** mechanic from the DEX design.

---

## 6. Class-Specific Benefits

### A. The Bard
*   **The Virtuoso:**
    *   **Song Power:** CHA is the primary modifier for Song effectiveness (Healing, Damage, Slows).
    *   **Crowd Control:** CHA determines the resist rate of Lull/Charm/Mez.

### B. The Pet Classes (Magician, Necro, Beastlord)
*   **The Commander:**
    *   CHA is the "Pet Stat". If you want your pet to tank a dragon, you need CHA.

#### Class Masteries (CHA)
*   **Magician - "Master Summoner":**
    *   **Double Scaling:** Magicians receive **2.0x** the benefit from Charisma for Pet Stats.
    *   *Impact:* At 1000 CHA, a Magician pet has **+200% HP/Damage** (Triple stats). It is effectively a Raid Boss.
*   **Beastlord - "Primal Bond":**
    *   **Stat Sync:** The Warder inherits the Beastlord's **Procs** and **Crit Chance** based on CHA.
*   **Necromancer - "Undead Horde":**
    *   **Quantity:** (If possible) CHA allows for the summoning of a temporary secondary pet (Swarm Pet) that lasts longer.

### C. The Enchanter (The Puppet Master)
*   **The True Summoner:**
    *   For Enchanters, Charisma is not just influence; it is **Dominion**.
    *   **Permanent Charm:** At high CHA thresholds (e.g., 500+), Charm spells become permanent (until the pet dies or is released).
    *   **Unbreakable Will:** Charm break chance is reduced to near 0%.
    *   **Soul Link:** The Enchanter transfers a portion of their own stats (STA/INT/AC) to the Charmed Pet based on CHA.
        *   *Formula:* `StatTransfer% = Charisma / 20`.
        *   *Impact:* At 1000 CHA, your Charmed Pet gains **50% of your stats** on top of its own.
    *   **Result:** An Enchanter doesn't just borrow a pet; they forge a demigod from a random NPC.

### D. The Solo Farmer
*   **The Treasure Hunter:**
    *   Any class farming for a specific drop will swap to a "Charisma Set" to maximize loot rolls.
