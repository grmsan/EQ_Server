# Agility (AGI) - Design Document

## Core Philosophy
Agility represents speed, reflexes, and fluidity of motion. In a Solo Server environment, Agility is the **Stat of Velocity**. It determines how fast you do everything (Move, Attack, Cast) and how hard it is for enemies to land a clean hit on you.

**Scaling Philosophy: The Law of Diminishing Returns**
Unlike Strength (Linear) or Stamina (Linear), Agility scales **Asymptotically**.
*   The first 100 points provide a massive boost (0 -> 60 mph).
*   The next 900 points provide refinement (60 -> 100 mph).
*   You can never reach "Infinity" (Instant attacks or 100% Dodge), but you can get very close.

---

## 1. "The Blur" (Attack Speed / Haste)
*Primary Beneficiaries: Melee DPS, Tanks.*

Agility directly reduces the delay between weapon swings.

### The Formula (Asymptotic)
We use a curve that approaches a **100% Haste Cap** (Double Attack Speed) but never quite reaches it without insane stats.

$$ Haste\% = \frac{100 \times Agility}{Agility + 400} $$

### Impact Table
| Agility | Haste Bonus | Notes |
| :--- | :--- | :--- |
| **100** | **20%** | Equivalent to a low-level Haste spell. |
| **200** | **33%** | Equivalent to "Flowing Thought" or mid-tier gear. |
| **500** | **55%** | Raid Buff level. |
| **1000** | **71%** | High-end Solo. |
| **2000** | **83%** | God-tier. |

*   **Result:** Every point of AGI always adds speed, but you get the "meat" of the benefit early.

---

## 2. "Lightning Reflexes" (Avoidance)
*Primary Beneficiaries: Monks, Rogues, Tanks.*

Avoidance is the chance to completely negate a melee attack (Miss/Dodge/Parry/Riposte).

### The Formula (Archetype Weighted)
Not all classes are trained to dodge. A Monk flows like water; a Wizard stands like a statue.

$$ BaseAvoidance\% = \left( \frac{75 \times Agility}{Agility + 300} \right) \times ClassMultiplier $$

*   **Class Multipliers:**
    *   **Monk, Rogue, Bard:** **1.5x** (The Artful Dodgers). *Can reach ~85% cap.*
    *   **Warrior, Paladin, SK, Ranger:** **1.0x** (Standard).
    *   **Casters & Healers:** **0.5x** (The Stationary Targets). *They rely on Runes/Wards instead.*

### Impact Table (at 1000 AGI)
| Archetype | Base Calculation | Multiplier | Final Avoidance |
| :--- | :--- | :--- | :--- |
| **Monk/Rogue** | 57.6% | 1.5x | **86.4%** (Soft Cap applied) |
| **Tank/Ranger** | 57.6% | 1.0x | **57.6%** |
| **Caster** | 57.6% | 0.5x | **28.8%** |

*   **Result:** Agility is the primary defense for light fighters, while Casters get a minor benefit but shouldn't rely on it.

---

## 3. "Fleet of Foot" (Movement Speed)
*Primary Beneficiaries: Everyone.*

Running speed is quality of life. High AGI reduces the need for "Spirit of Wolf" or Mounts.

### The Formula
We want 1000 AGI to feel like a rocket.

$$ RunSpeed\% = \frac{120 \times Agility}{Agility + 200} $$

*   **Cap:** ~110% (Faster than Bard Speed).
*   **Stacking:** Does *not* stack with spells/mounts.

### Impact Table
| Agility | Run Speed | Feel |
| :--- | :--- | :--- |
| **100** | **40%** | Faster than Spirit of Wolf (Low level). |
| **500** | **85%** | Epic Mount speed. |
| **1000** | **100%** | **Velocity Cap.** (Bard Speed). |
| **2000** | **109%** | Breaking the sound barrier. |

---

## 4. "Quickened Mind" (Casting Speed)
*Primary Beneficiaries: Casters, Healers.*

*Suggestion:* To make Agility useful for Casters (who don't swing swords), AGI should reduce **Spell Cast Time** and **Global Cooldown (GCD)**.

### The Formula
$$ CastTimeReduction\% = \frac{50 \times Agility}{Agility + 500} $$

*   **Cap:** 50% reduction.

### Impact Table
| Agility | Cast Reduction | Example (10s Cast) |
| :--- | :--- | :--- |
| **100** | **8.3%** | 9.1s |
| **500** | **25.0%** | 7.5s |
| **1000** | **33.3%** | 6.7s |

*   **Solo Reality:** This is huge for kiting or getting that heal off before you die.

---

## 5. Class-Specific Benefits

### A. The Tanks (Warrior, SK, Paladin)
*   **The Dancer:**
    *   Avoidance is their primary layer of defense.
    *   Haste generates aggro (more swings = more procs).

### B. The Melee DPS (Rogue, Monk, etc.)
*   **The Blender:**
    *   Haste is their primary DPS stat.
    *   Movement speed helps with positioning (Backstabs).

### C. The Casters (Wizard, Necro, etc.)
*   **The Speed-Caster:**
    *   AGI is no longer a "dump stat". It allows them to cast nukes faster, increasing DPS and safety (less time standing still).
    *   Movement speed is critical for kiting.

### D. The Bard
*   **The Speed Limit Breaker:**
    *   *Special Rule:* Bards break the caps.
    *   Their Movement Speed cap is higher (Instrument dependent).
    *   Their Avoidance calculation treats AGI as 20% more effective.
