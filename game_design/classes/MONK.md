# Monk - Class Design Document

## 1. Class Fantasy: "The Martial Artist"
The Monk is a discipline of body and mind. They fight with their fists or staves, relying on speed and fluidity rather than armor. In a Solo Server environment, they are the **Avoidance Tanks**. They can take a hit, but they prefer not to be there when the punch lands.

*   **Primary Stat:** **Agility (AGI)**
*   **Class Mastery:** **"Flow Like Water"**
    *   **Fluid Motion:** Agility adds to **Double Attack** chance and **Dodge** chance.
    *   **Inner Peace:** Increases out-of-combat Health Regeneration significantly, minimizing downtime.

---

## 2. Combat Profile

### A. Dealing Damage (The Flurry)
Monks attack fast. Very fast.
*   **Primary Source:** **Hand-to-Hand** (Fists) or **2H Blunt** (Staves).
*   **Secondary Source:** **Special Attacks** (Flying Kick, Tiger Claw, Dragon Punch).
*   **Mechanic:** **"Technique"**. Monks chain special attacks between auto-attacks.

### B. Taking Damage (Avoidance)
Monks wear Leather but have the best defensive skills in the game.
*   **Mitigation:** Low AC (Leather).
*   **Survival:** **Block / Dodge / Riposte**. A high-AGI Monk can avoid 40-50% of incoming attacks entirely.
*   **Recovery:** **Mend**. A powerful self-heal on a moderate cooldown (e.g., 6 minutes -> reduced to 2 minutes for Solo Server?).

---

## 3. The Toolkit

### A. Damage Tools
1.  **Flying Kick:** High damage finisher.
2.  **Tiger Claw / Dragon Punch:** Fast strikes.
3.  **Eagle Strike:** Can stun or interrupt.
4.  **Hundred Fists (Disc):** Massive attack speed boost.

### B. Mitigation Tools
1.  **Stonestance (Disc):** Drastically reduces incoming damage for a short time.
2.  **Whirlwind (Disc):** Ripostes all attacks from the front.
3.  **Mend:** Heals 25-50% of HP. Critical for solo survival.

### C. Utility
1.  **Feign Death:** The ultimate pulling tool. Split mobs, drop aggro, survive wipes.
2.  **Intimidation:** Fear mechanic (rarely used solo, but available).
3.  **Safe Fall:** No fall damage.

### D. Area Effect (AE) Potential
*   **Spinning Kick:** Hits all enemies in melee range.
*   **Destructive Force:** A discipline that allows the Monk to hit every enemy in front of them with every swing.

---

## 4. Solo Strategy
*   **Level 1-30:** Punch things. Use Mend when low.
*   **Level 30-60:** **Pulling Specialist**. Use Feign Death to separate named mobs from their guards. Fight them 1v1.
*   **Level 70+:** **Avoidance Tanking**. With high AGI and raid gear, you become almost unhittable by standard mobs.

---

## 5. Class Scores (1-10)

| Category | Score | Notes |
| :--- | :--- | :--- |
| **Single Target DPS** | **9/10** | Consistent, high sustained DPS. |
| **AE DPS** | **6/10** | Good with specific Disciplines active. |
| **Tankiness** | **6/10** | Great avoidance, but spikes of damage hurt. |
| **Sustainability** | **6/10** | Mend is great, but has a cooldown. |
| **Utility** | **8/10** | Feign Death is invaluable. |
| **Pet Power** | **0/10** | No pet. |

---

## 6. Summary
The Monk is the **Tactical Soloist**. You control the engagement terms with Feign Death. You overwhelm the enemy with speed. You are self-sufficient, requiring no weapons or armor to be effective (though they help).

---

## 7. The Big Three: Core Abilities

These abilities channel the Monk's inner focus into physical perfection, scaling with agility and level.

### I. [Offensive] Falling Star Strike
*   **Description:** A devastating aerial kick that channels the Monk's momentum into a single point. Deals massive physical damage and can stun.
*   **Scaling:** $Damage = (\text{Level}^{2.1} \times 1.4) + (\text{AGI} \times 15)$.
*   **Cooldown:** 30 Seconds.
*   **Duration:** Instant / 6 Seconds (Stun).
*   **Solo Use:** Elite burst damage to finish off enemies or interrupt spells.
*   **Group/Synergy:** None (Pure DPS).

### II. [Defensive] Zen Meditation
*   **Description:** The Monk enters a state of perfect calm. For a short duration, they become immune to all physical damage but cannot attack. On completion, they are healed.
*   **Scaling:** $Heal = (\text{Level}^{1.8} \times 1.0) + (\text{AGI} \times 5)$.
*   **Cooldown:** 120 Seconds.
*   **Duration:** 6 Seconds.
*   **Solo Use:** The ultimate "reset" button. Use it when Mend is on cooldown to survive a heavy burst.
*   **Group/Synergy:** Allows the Monk to "off-tank" a massive hit for the group.

### III. [Utility/Synergy] Vulnerable Points
*   **Description:** A series of precise strikes that expose the target's weaknesses. Increases the **Critical Hit Chance** of all allies attacking the target.
*   **Scaling:** $Bonus = (\text{Level} \times 0.2) + (\text{AGI} \times 0.05)\%$ increased crit chance.
*   **Cooldown:** 60 Seconds.
*   **Duration:** 12 Seconds.
*   **Solo Use:** Boosts your own massive flurry of crits.
*   **Synergy:** **Critical Force Multiplier**. This is powerful for every single class, as it increases the chance for everyone to deal double (or triple) damage. Pairs exceptionally well with the Berserker's `Savage Exposure`.
*   **Stats:** Scales with **AGI** (Fluidity).
*   **Synergy Value:** Extreme. Crit chance is the most sought-after stat for end-game DPS.
