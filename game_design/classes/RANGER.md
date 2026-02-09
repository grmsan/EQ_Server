# Ranger - Class Design Document

## 1. Class Fantasy: "The Master of the Wild"
The Ranger is the versatile hunter. They are equally skilled with a bow and dual blades. They use nature magic to track, snare, and sustain themselves. In a Solo Server environment, they are the **Hybrid Skirmishers**.

*   **Primary Stat:** **Dexterity (DEX)**
*   **Class Mastery:** **"Hunter's Focus"**
    *   **True Shot:** Dexterity increases **Bow Damage** and **Critical Hit Chance**.
    *   **Survivalist:** Dexterity adds to Defense and Parry chance.

---

## 2. Combat Profile

### A. Dealing Damage (The Switch-Hitter)
Rangers can fight at any range.
*   **Primary Source:** **Archery** (Ranged) or **Dual Wield** (Melee).
*   **Secondary Source:** **Nature Spells** (DoTs/Nukes).
*   **Mechanic:** **"Headshot"**. A passive chance to instantly kill humanoid targets with a bow.

### B. Taking Damage (Light Tank)
Rangers wear Chain armor and have defensive spells.
*   **Mitigation:** Chain Armor + Skin Spells (AC Buffs).
*   **Survival:** **Kiting**. If the enemy hits too hard, Snare it and shoot it from a distance.
*   **Recovery:** **Heals**. Rangers have access to Druid healing spells (up to a certain level).

---

## 3. The Toolkit

### A. Damage Tools
1.  **Autofire (Bow):** Consistent ranged DPS.
2.  **Dual Wield:** Fast melee attacks.
3.  **Fire/Ice Nukes:** Magic damage to supplement physical attacks.
4.  **Trueshot (Disc):** Massive boost to Bow damage for a short time.

### B. Mitigation Tools
1.  **Skin like Nature:** AC and HP buff.
2.  **Weapon Shield (Disc):** Parries all attacks for a short duration (Panic Button).
3.  **Chloroplast:** Health Regeneration buff.

### C. Utility
1.  **Snare:** The best snare in the game. Essential for kiting.
2.  **Tracking:** Find any mob in the zone. The ultimate solo tool for finding named mobs.
3.  **Sow (Spirit of Wolf):** Movement speed buff.
4.  **Harmony:** Reduces aggro radius for splitting pulls (Outdoors only).

### D. Area Effect (AE) Potential
*   **Hail of Arrows:** Hits a target area with arrows.
*   **Lightning Swipe:** Frontal cone melee attack.

---

## 4. Solo Strategy
*   **Level 1-30:** Melee hybrid. Buff up, slow the mob, chop it down.
*   **Level 30-60:** **Bow Kiting**. Snare the mob, run away, shoot it. Repeat. You take 0 damage.
*   **Level 70+:** **Headshot Farming**. Find a zone with Humanoids. Pull 50 of them. Headshot them all before they reach you.

---

## 5. Class Scores (1-10)

| Category | Score | Notes |
| :--- | :--- | :--- |
| **Single Target DPS** | **8/10** | Strong melee, amazing ranged. |
| **AE DPS** | **5/10** | Decent with bow AEs. |
| **Tankiness** | **5/10** | Chain armor is okay, but not great. |
| **Sustainability** | **8/10** | Heals, Regen, and Kiting make them very sustainable. |
| **Utility** | **10/10** | Tracking + Snare + SoW = The ultimate hunter. |
| **Pet Power** | **0/10** | No pet (usually). |

---

## 6. Summary
The Ranger is the **Versatile Hunter**. You have a tool for every situation. If it hits hard, kite it. If it runs, snare it. If it hides, track it. You are the master of the open world.

---

## 7. The Big Three: Core Abilities

These abilities bridge the gap between Archer and Druidic warrior, scaling with agility and level.

### I. [Offensive] Verdant Storm
*   **Description:** A magical volley of spectral arrows that rains down on the target, dealing physical and magic damage.
*   **Scaling:** $Damage = (\text{Level}^{2.1} \times 1.25) + (\text{DEX} \times 15)$.
*   **Cooldown:** 30 Seconds.
*   **Duration:** Instant / 6 Seconds (Nature DoT).
*   **Solo Use:** Massive burst damage from range.
*   **Group/Synergy:** None (Pure DPS).

### II. [Defensive] Barkskin Aegis
*   **Description:** Hardens the Ranger's skin to the density of ironwood. Reduces all incoming physical damage by a flat amount and reflects damage from thorns.
*   **Scaling:** $Mitigation = (\text{Level} \times 1) + (\text{DEX} \times 0.1)$. $Thorns = (\text{Level}^{1.5} \times 0.4)$.
*   **Cooldown:** 120 Seconds.
*   **Duration:** 12 Seconds.
*   **Solo Use:** Critical for survival when kiting fails or in tight dungeons.
*   **Group/Synergy:** None (Survival focus).

### III. [Utility/Synergy] Nature's Mark
*   **Description:** Shoots a glowing tracer arrow into the target. All allies attacking the marked target gain increased **Accuracy** and a chance to perform an **Extra Attack**.
*   **Scaling:** $Accuracy = (\text{Level} \times 2) + (\text{DEX} \times 0.2)$. $ExtraAttackChance = (\text{Level} \times 0.1)\%$.
*   **Cooldown:** 60 Seconds.
*   **Duration:** 12 Seconds.
*   **Solo Use:** Significantly boosts your own melee and archery throughput.
*   **Synergy:** **Accuracy Multiplier**. Ensures that every melee class in the group hits their target consistently, especially against high-evasion bosses. The extra attack chance is a massive DPS boost for the entire group.
*   **Stats:** Scales with **DEX** (Marksmanship).
*   **Synergy Value:** Extreme. Makes every physical attacker in the group feel significantly "smoother" to play.
