# Druid - Class Design Document

## 1. Class Fantasy: "The Force of Nature"
The Druid is the master of the wild. They are the jack-of-all-trades, capable of healing, nuking, and dotting. In a Solo Server environment, they are the **Kiting Masters**. They control the battlefield with snares and roots, whittling down enemies with the wrath of nature while never being touched.

*   **Primary Stat:** **Wisdom (WIS)**
*   **Class Mastery:** **"Nature's Wrath"**
    *   **Thorns:** Wisdom scales the damage of **Damage Shields** exponentially. (Enemies kill themselves hitting the tank/pet).
    *   **Living Swarm:** Insect DoTs (e.g., Drones of Doom) become **Viral**. If an infected enemy is within range of another enemy, the swarm spreads, refreshing its duration.
    *   **Wildfire:** Fire spells apply a stacking "Burn" effect that increases subsequent Fire damage.

---

## 2. Combat Profile

### A. Dealing Damage (The Kite)
Druids have the most diverse damage toolkit.
*   **Primary Source:** **DoTs** (Insect/Fire) and **Nukes** (Fire/Cold).
*   **Secondary Source:** **Damage Shield**. A high-WIS Druid can kill a mob simply by letting it hit them (or their pet).
*   **Mechanic:** **"Quad Kiting"**. Using AOE Snares and AOE Nukes to kill 4 mobs at once.

### B. Taking Damage (Don't)
Druids wear Leather. They are squishy.
*   **Mitigation:** Low. Leather armor offers little protection.
*   **Survival:** **Speed**. Spirit of Wolf and Snare mean the enemy should never reach you.
*   **Recovery:** Strong HoTs (Heal over Time) allow them to recover while running.

---

## 3. The Toolkit

### A. Damage Tools
1.  **Starfire (Nuke):** Fast-casting Fire damage.
2.  **Swarm of Beetles (DoT):** Long-duration Magic DoT + Mini-Stuns.
3.  **Thorns (Buff):** Massive Damage Shield.
4.  **Lightning Strike (AE):** Targeted AOE nuke for Quad Kiting.

### B. Mitigation Tools
1.  **Skin like Nature:** HP and AC buff.
2.  **Bladecoat:** Self-only AC buff that stacks with everything.
3.  **Spirit of Wolf:** Movement speed is the best defense.

### C. Utility
1.  **Teleportation:** Can travel to any zone instantly. (The ultimate convenience).
2.  **Snare:** Reduces enemy movement speed. (Essential for kiting).
3.  **Charm Animal:** Can take control of animals (very strong in certain zones).
4.  **Evacuate:** Instantly teleports the group to safety.

### D. Area Effect (AE) Potential
*   **Viral Swarms:** Cast "Drifting Death" on one mob, pull it through a camp, and watch the entire zone get infected.
*   **Quad Kiting:** The Druid's signature move. Snare 4 mobs, group them up, and cast AOE Lightning until they die.

---

## 4. Solo Strategy
*   **Level 1-30:** Damage Shield tanking. Put Thorns on yourself and melee.
*   **Level 30-60:** **Quad Kiting**. Pull 4 mobs, snare them, run in circles, and AOE nuke.
*   **Level 70+:** **Charm Kiting**. Charm a Dire Wolf. Buff it with Thorns and Haste. Let it tank while you drop DoTs and Nukes.

---

## 5. Class Scores (1-10)

| Category | Score | Notes |
| :--- | :--- | :--- |
| **Single Target DPS** | **8/10** | Strong DoTs and Nukes. |
| **AE DPS** | **9/10** | The masters of open-world AOE farming. |
| **Tankiness** | **3/10** | Leather armor. If you get caught, you die. |
| **Sustainability** | **7/10** | Good Mana Regen buffs, but Quad Kiting is mana heavy. |
| **Utility** | **10/10** | Ports, Snares, Buffs, Tracking. |
| **Pet Power** | **6/10** | Charm is strong, but restricted to Animals. |

---

## 6. Summary
The Druid is the **High Mobility, High Utility** solo class. You are the master of travel and the open world. You dictate the terms of the engagement. If the fight goes bad, you leave. If the fight goes well, you kill 4 enemies at once.

---

## 7. The Big Three: Core Abilities

These abilities harness the untamed fury of the natural world, scaling with wisdom and environmental connection.

### I. [Offensive] Wrath of the Heavens
*   **Description:** Calls down a massive strike of lightning followed by a localized hail storm. Deals Fire, Cold, and Magic damage.
*   **Scaling:** $Damage = (\text{Level}^{2.1} \times 1.3) + (\text{WIS} \times 18)$.
*   **Cooldown:** 45 Seconds.
*   **Duration:** Instant / 6 Seconds (Elemental Debuff).
*   **Solo Use:** Elite burst damage for finishing quad-kites or taking down bosses.
*   **Group/Synergy:** None (Pure DPS).

### II. [Defensive] Spirit of the Grove
*   **Description:** The Druid becomes one with the earth. While active, they are immune to Root and Snare, and receive a massive Health Regeneration boost.
*   **Scaling:** $Regen = (\text{Level}^{1.5} \times 2.0) + (\text{WIS} \times 5)$ per tick.
*   **Cooldown:** 120 Seconds.
*   **Duration:** 10 Seconds.
*   **Solo Use:** Essential for survival when a kite goes wrong or when being chased by multiple enemies.
*   **Group/Synergy:** None (Survival focus).

### III. [Utility/Synergy] Primal Attunement
*   **Description:** Elements hum in resonance around the Druid. All allies gain increased **Spell Damage** and **Spell Critical Chance**.
*   **Scaling:** $SpellDamage = (\text{Level} \times 1.0) + (\text{WIS} \times 0.1)\%$. $CritChance = (\text{Level} \times 0.1)\%$.
*   **Cooldown:** 90 Seconds.
*   **Duration:** 12 Seconds.
*   **Solo Use:** Massive boost to your own Nukes and DoTs.
*   **Synergy:** **Caster Force Multiplier**. This is the primary damage buff for Wizards, Magicians, Necromancers, and even Shamans. It turns a group of casters into a firing squad.
*   **Stats:** Scales with **WIS** (Nature's Harmony).
*   **Synergy Value:** Extreme. Essential for any magic-based group composition.
