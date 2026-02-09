# Warrior - Class Design Document

## 1. Class Fantasy: "The Unstoppable Juggernaut"
The Warrior is the embodiment of physical perfection and martial discipline. In a world of magic and monsters, the Warrior stands firm with nothing but steel and grit. They do not dodge like a Rogue or ward like a Wizard; they **endure**. They are the anvil upon which the enemy breaks.

*   **Primary Stat:** **Stamina (STA)**
*   **Class Mastery:** **"Iron Skin"** - Stamina increases the Soft Cap for AC and provides superior flat damage mitigation compared to any other class.

---

## 2. Combat Profile

### A. Dealing Damage (The Grinder)
Warriors are not burst assassins. They are sustained damage machines.
*   **Primary Source:** Dual Wield (DPS) or 2H (Burst).
*   **Mechanic:** **"Fervor"**. As the Warrior fights, they build attack speed and critical chance naturally.
*   **Procs:** Warriors rely heavily on Weapon Procs (augmented by DEX) to supplement their physical damage.

### B. Taking Damage (Face Tanking)
Warriors take the hit. They don't rely on RNG (Avoidance) or Magic (Runes).
*   **Mitigation:** Highest AC Softcap in the game.
*   **Absorption:** "Stance" Disciplines that reduce incoming damage by a flat % (e.g., Defensive Stance).
*   **Survival:** Massive HP Pool (1.5x Multiplier from STA) allows them to survive spikes that would one-shot others.

---

## 3. The Toolkit

### A. Damage Tools
1.  **Mighty Strike (Disc):** Guarantees Critical Hits for a short duration.
2.  **Blade Flurry (Passive):** Chance to strike all enemies in front of the Warrior (Frontal AE).
3.  **Execute (Disc):** A custom Combat Ability (Spell) that deals massive damage, but fails if the target is above 20% HP.
4.  **Bash/Kick:** Interrupts casting and deals minor damage.
5.  **Heroic Throw (Disc):** A ranged weapon toss that pulls a target, generates high aggro, and starts building Fervor before the Warrior is in melee range.
6.  **Colossal Smash (Disc):** A crushing overhead blow that applies an "Armor Shattered" debuff, reducing the target's physical mitigation for a short duration and amplifying all incoming melee damage.

### B. Mitigation Tools
1.  **Defensive Discipline:** The ultimate "Oh Shit" button. Reduces all incoming damage by 50% but lowers damage output.
2.  **Stonewall (Passive):** If the Warrior stands still for 3 seconds, they gain increased AC and Knockback resistance.
3.  **Second Wind (Disc):** Instantly heals the Warrior for 40% HP (Long Cooldown).
4.  **Last Bastion (Disc):** When dropped below 30% HP, the Warrior can trigger this to gain an additional burst of damage reduction and crowd-control immunity for a few seconds, letting them stabilize through lethal spikes.

### C. Utility
1.  **Taunt/Bellow:** Forces the enemy to attack the Warrior (Essential for pet protection if using clicky pets).
2.  **War Cry:** AOE Buff that increases Party/Pet Attack Power and Haste.
3.  **Intimidate:** Chance to Fear a target (CC).
4.  **Battle Standard (Disc):** Plants a temporary banner at the Warrior's location that grants nearby allies bonus HP and minor damage mitigation, reinforcing their role as the center of the fight.

### D. Area Effect (AE) Potential
*   **Rampage (Passive):** Passive chance to hit surrounding mobs on every swing.
*   **Whirlwind (Disc):** **(10s Cooldown)** Active AE attack hitting all targets within melee range.
*   **Thunderclap (Disc):** **(12s Cooldown)** Short range PBAOE that generates massive aggro and deals damage. Replaces the long-cooldown "Area Taunt".
*   **Shockwave Charge (Disc):** A short-range charge that closes the gap to a target and releases a point-blank shockwave on impact, dealing light damage and applying a brief stun or slow to nearby enemies.

---

## 4. Solo Strategy
*   **Level 1-30:** Face tank. High Regen (STA) keeps downtime low. Dual Wield for fast kills.
*   **Level 30-60:** Use "Intimidate" to fear-kite dangerous mobs or "Defensive" to tank multiples.
*   **Level 70+:** You are a Raid Boss. You pull the entire room. You pop "Whirlwind" and "Defensive". You out-regen the damage of 10 mobs while grinding them down.

---

## 5. Class Scores (1-10)

| Category | Score | Notes |
| :--- | :--- | :--- |
| **Single Target DPS** | **7/10** | Consistent, high sustained physical damage. |
| **AE DPS** | **6/10** | Good with Rampage/Whirlwind, but not a Wizard. |
| **Tankiness** | **10/10** | The Gold Standard. Best Mitigation & HP. |
| **Sustainability** | **9/10** | High STA = Massive HP Regen. No mana to manage. |
| **Utility** | **3/10** | Limited to Buffs/Debuffs. No travel/invis. |
| **Pet Power** | **0/10** | No pets (unless via items). |

---

## 6. Summary
The Warrior is the **Low Risk, High Consistency** solo class. You will rarely die, but you won't kill as fast as a Rogue. You solve problems by hitting them until they stop moving, and you can take a beating that would kill a god.

---

## 7. The Big Three: Core Abilities

These abilities are designed to be high-impact active "clicks" that define the Warrior's combat rhythm.

### I. [Offensive] Colossal Smash
*   **Description:** A crushing overhead blow that deals massive physical damage and shatters the target's armor.
*   **Stats:** Scales with **STR** and **Level**.
*   **Scaling:** $Damage = (\text{Level}^{2.1} \times 1.0) + (\text{STR} \times 10)$
*   **Cooldown:** 30 Seconds.
*   **Duration:** 12 Seconds (Armor Debuff).
*   **Synergy:** **Armor Shatter**. Reduces Target AC by 20%. This benefits all physical attackers (Rogue, Monk, Ranger, Pets).

### II. [Utility] Heroic Throw
*   **Description:** A powerful ranged toss that deals physical damage and generates massive aggro.
*   **Stats:** Scales with **STR** and **Level**.
*   **Scaling:** $Damage = (\text{Level}^{2.0} \times 0.8) + (\text{STR} \times 5)$
*   **Cooldown:** 45 Seconds.
*   **Duration:** 6 Seconds (Aggro Lock/Snare).
*   **Synergy:** **Target Isolation**. Snares the target and forces it to focus on the Warrior, protecting squishier allies.

### III. [Defensive] Ignore Pain
*   **Description:** The Warrior enters a trance of pure endurance, ignoring a portion of all incoming damage.
*   **Stats:** Scales with **STA** and **Level**.
*   **Scaling:** $Absorb = (\text{Level}^{2.0} \times 1.5) + (\text{STA} \times 10)$
*   **Cooldown:** 90 Seconds.
*   **Duration:** 12 Seconds.
*   **Synergy:** None (Pure Survival). Allows the Warrior to survive high-damage phases without burdening healers.
