# Berserker - Class Design Document

## 1. Class Fantasy: "The Raging Storm"
The Berserker is a whirlwind of destruction. They wield massive 2H Axes and wear Chain armor. They do not care about defense; they care about killing the enemy before the enemy kills them. In a Solo Server environment, they are the **Lifesteal Tanks**.

*   **Primary Stat:** **Strength (STR)**
*   **Class Mastery:** **"Bloodlust"**
    *   **Overkill:** Strength increases **Critical Hit Chance** and **Cleave Damage**.
    *   **Blood Craze:** Critical hits have a chance to heal the Berserker for a percentage of the damage dealt.

---

## 2. Combat Profile

### A. Dealing Damage (The Decapitator)
Berserkers deal massive physical damage.
*   **Primary Source:** **2H Axe**.
*   **Secondary Source:** **Thrown Axes** (Ranged/Utility).
*   **Mechanic:** **"Frenzy"**. A special attack that deals damage and can trigger various effects (Stun, Snare, Bleed).

### B. Taking Damage (Kill or Be Killed)
Berserkers are squishy for a melee class.
*   **Mitigation:** Chain Armor (Medium).
*   **Survival:** **Stuns** and **Lifesteal**.
*   **Panic Button:** **Unstoppable**. Removes all CC effects and grants temporary immunity.

---

## 3. The Toolkit

### A. Damage Tools
1.  **Frenzy:** The core attack.
2.  **Volley:** Throws multiple axes at once.
3.  **Decapitate:** A chance to instantly kill the target (Level 60+).
4.  **Blind Rage (Disc):** Increases damage taken but massively increases damage dealt.

### B. Mitigation Tools
1.  **Stunning Cry:** Stuns all enemies in a cone.
2.  **Blood Pact:** Sacrifices HP to gain Attack Power (Risk/Reward).
3.  **Toughness:** Passive HP boost.

### C. Utility
1.  **Snare (Axe):** Thrown axe that snares the target.
2.  **Jolt:** Reduces aggro (useless solo) -> Changed to **Intimidate** (Fear).

### D. Area Effect (AE) Potential
*   **Rampage:** Hits all enemies in front of the Berserker.
*   **Whirlwind Axe:** Throws an axe that spirals out, hitting everything.

---

## 4. Solo Strategy
*   **Level 1-30:** Kill things fast. Bandage between fights.
*   **Level 30-60:** **Chain Pulling**. Use "Blood Craze" to heal off mobs. If you stop killing, you die.
*   **Level 70+:** **Swarm Killing**. Gather 5-6 mobs, pop Rampage and Blind Rage, and cleave them all down instantly.

---

## 5. Class Scores (1-10)

| Category | Score | Notes |
| :--- | :--- | :--- |
| **Single Target DPS** | **9/10** | Extremely high. |
| **AE DPS** | **9/10** | The best melee AOE in the game. |
| **Tankiness** | **5/10** | High HP, but low mitigation. |
| **Sustainability** | **5/10** | Relies on killing to heal (Blood Craze). |
| **Utility** | **2/10** | Very low utility. Just damage and snare. |
| **Pet Power** | **0/10** | No pet. |

---

## 6. Summary
The Berserker is the **High-Octane Soloist**. You live on the edge. If you crit, you live. If you miss, you die. It is a fast-paced, adrenaline-fueled playstyle.

---

## 7. The Big Three: Core Abilities

These abilities are the cornerstone of the Berserker's power, scaling exponentially with level and stats.

### I. [Offensive] Decapitating Cyclone
*   **Description:** A massive circular slash that deals physical damage to all enemies in range.
*   **Scaling:** $Damage = (\text{Level}^{2.1} \times 1.5) + (\text{STR} \times 15)$.
*   **Cooldown:** 30 Seconds.
*   **Duration:** Instant / 6 Seconds (Bleed).
*   **Solo Use:** Primary tool for swarm clearing and high burst.
*   **Group/Synergy:** None (Pure DPS).

### II. [Defensive] Bloodthirster's Resolve
*   **Description:** Enters a state where every successful melee hit heals the Berserker.
*   **Scaling:** $Heal = (\text{Level}^{1.9} \times 0.5) + (\text{STA} \times 2)$ per hit.
*   **Cooldown:** 90 Seconds.
*   **Duration:** 12 Seconds.
*   **Solo Use:** Essential for staying alive while tanking multiple mobs.
*   **Group/Synergy:** None (Survival focus).

### III. [Utility/Synergy] Savage Exposure
*   **Description:** A brutal strike that leaves the target vulnerable. Increases the **Melee Critical Damage** of all attackers against that target.
*   **Scaling:** $Bonus = (\text{Level} \times 0.5) + (\text{DEX} \times 0.05)\%$ increased crit damage.
*   **Cooldown:** 60 Seconds.
*   **Duration:** 12 Seconds.
*   **Solo Use:** Increases your own massive crit numbers.
*   **Synergy:** **Melee Force Multiplier**. This makes Rogues, Monks, Warriors, and Paladins deal significantly more damage when grouped with a Berserker.
*   **Stats:** Scales with **DEX** (Precision).
*   **Synergy Value:** High. Makes every melee class feel more powerful.
