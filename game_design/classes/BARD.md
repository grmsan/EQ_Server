# Bard - Class Design Document

## 1. Class Fantasy: "The Virtuoso"
The Bard is the jack-of-all-trades. They weave magic through music, buffing allies, mesmerizing enemies, and running faster than the wind. In a Solo Server environment, they are the **Swarm Kiters**. They don't fight one enemy; they fight the entire zone at once.

*   **Primary Stat:** **Charisma (CHA)**
*   **Class Mastery:** **"The One Man Band"**
    *   **Soloist's Echo:** When a song affects *only* the Bard (or Bard + Pet), its potency is increased by **100%**. (e.g., Haste is doubled, Regen is doubled, Damage Shield is doubled).
    *   **Harmonic Blade:** Charisma adds **Magic Damage** to melee attacks, allowing the Bard to fight as a magical warrior.
    *   **Mesmerizing Presence:** Charisma increases the chance for Charm and Mesmerize to land.

---

## 2. Combat Profile

### A. Dealing Damage (The Battle Bard)
Bards are no longer just support; they are magical duelists.
*   **Primary Source:** **Melee** (Dual Wield) enhanced by **Harmonic Blade**.
*   **Secondary Source:** **AOE DoT Songs** (Chants) which now tick for massive damage.
*   **Mechanic:** **"Twisting"**. The Bard maintains 4-5 songs simultaneously. With "Soloist's Echo", this means they have Haste, Regen, AC, and Damage Shield all running at double strength.

### B. Taking Damage (The Dancing Tank)
Bards wear Plate and move with supernatural grace.
*   **Mitigation:** Plate Armor (High AC) + **Amplified Defensive Songs**.
*   **Survival:** **Speed & Regen**. With amplified Health Song, a Bard can regenerate HP as fast as a Troll.
*   **Panic Button:** **Fading Memories**. Instantly drops all aggro and turns the Bard invisible. The best escape tool in the game.

---

## 3. The Toolkit

### A. Damage Tools
1.  **Chants (DoT):** Disease, Fire, and Cold DoTs that hit all enemies in range.
2.  **Bellow:** Sonic direct damage.
3.  **Boastful Bellow:** A large nuke (AA/Disc).

### B. Control Tools
1.  **Lullaby (Mez):** Puts enemies to sleep.
2.  **Charm:** Takes control of an enemy to fight for you. (High risk, high reward).
3.  **Fear:** Causes enemies to flee.
4.  **Snare:** Slows enemy movement.

### C. Utility
1.  **Selo's Accelerando:** The fastest movement speed buff in the game.
2.  **Mana Song:** Regenerates Mana for the group (or self).
3.  **Health Song:** Regenerates HP.
4.  **Resist Songs:** Massive boost to elemental resistances.

### D. Area Effect (AE) Potential
*   **Supercharged Chants:** DoT songs deal significantly higher damage, allowing the Bard to melt swarms of enemies rather than just annoying them.
*   **Denon's Disruptive Discord:** PBAOE damage song.

---

## 4. Solo Strategy
*   **Level 1-30:** **Battle Bard**. Buff yourself with double-strength songs and melee mobs down.
*   **Level 30-60:** **Swarm Kiting**. Gather 20-30 mobs. Run in a circle. Play your supercharged DoT songs. They die much faster now.
*   **Level 70+:** **God Mode**. You have maxed defenses, double-strength regen, and high melee DPS. You can face-tank bosses or kite entire zones.

---

## 5. Class Scores (1-10)

| Category | Score | Notes |
| :--- | :--- | :--- |
| **Single Target DPS** | **8/10** | High melee DPS due to Harmonic Blade + Haste. |
| **AE DPS** | **10/10** | Can kill unlimited mobs simultaneously (fast). |
| **Tankiness** | **9/10** | Plate armor + Amplified Defensive Songs. |
| **Sustainability** | **10/10** | Mana Song + Health Song = Zero downtime. |
| **Utility** | **10/10** | Speed, Mana, CC, Tracking. |
| **Pet Power** | **8/10** | Charm pets are strong, but you don't *need* them anymore. |

---

## 6. Summary
The Bard is the **Supercharged Soloist**. You are a group of 6 players rolled into one. You have the Haste of an Enchanter, the Regen of a Shaman, the Armor of a Warrior, and the Speed of... well, a Bard.

---

## 6. Summary
The Bard is the **Supercharged Soloist**. You are a group of 6 players rolled into one. You have the Haste of an Enchanter, the Regen of a Shaman, the Armor of a Warrior, and the Speed of... well, a Bard.

---

## 7. The Big Three: Core Abilities

These abilities represent the peak of musical mastery, turning the Bard into a harmonic epicenter that scales with charisma and level.

### I. [Offensive] Sound & Fury
*   **Description:** The Bard enters a state of violent musical resonance. Every melee hit releases a sonic shockwave that deals magic damage to the target and all nearby enemies.
*   **Scaling:** $Damage = (\text{Level}^{2.0} \times 0.8) + (\text{CHA} \times 10)$.
*   **Cooldown:** 45 Seconds.
*   **Duration:** 12 Seconds.
*   **Solo Use:** Massive boost to single-target and area DPS.
*   **Synergy:** **Magic Damage Multiplier**. This ability significantly boosts the "proc" rate and damage for any weapon-based magic effects, making it powerful when paired with magical melee classes like the Paladin or Shadowknight.
*   **Stats:** Scales with **CHA** (Sonic Prowess).

### II. [Defensive] Sonic Barrier
*   **Description:** The Bard weaves a high-frequency wall of sound. For the duration, there is a high chance to deflect all projectiles and reflect 50% of spell damage back to the caster.
*   **Scaling:** $Reflection = 50\%$. $DeflectChance = (\text{Level} \times 0.5) + (\text{CHA} \times 0.1)\%$.
*   **Cooldown:** 90 Seconds.
*   **Duration:** 8 Seconds.
*   **Solo Use:** Essential for surviving heavy caster mobs or large groups of archers.
*   **Group/Synergy:** None (Survival focus).

### III. [Utility/Synergy] Virtuoso's Reach
*   **Description:** The Bard projects a field of perfect harmony. All allies within the aura have the **Effectiveness** of their active buffs and songs increased by a percentage.
*   **Scaling:** $Multiplier = (\text{Level} \times 0.5) + (\text{CHA} \times 0.1)\%$ increase to buff potency.
*   **Cooldown:** 120 Seconds.
*   **Duration:** 12 Seconds.
*   **Solo Use:** Massive boost to your own songs, effectively doubling your power.
*   **Synergy:** **Buff Potency Multiplier**. This is the ultimate "force multiplier" for any group. It makes everyone's buffs (Clarity, Haste, AC, Heals) significantly stronger, effectively turning a normal group into a raid-tier force.
*   **Stats:** Scales with **CHA** (Harmonic Mastery).
