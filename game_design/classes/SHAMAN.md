# Shaman - Class Design Document

## 1. Class Fantasy: "The Spirit Walker"
The Shaman is the conduit between the physical and spiritual worlds. They are the masters of attrition. They slow their enemies to a crawl, rot their bodies with disease, and cannibalize their own spirit to fuel their magic. In a Solo Server environment, they are the **Unstoppable Force**.

*   **Primary Stat:** **Wisdom (WIS)**
*   **Class Mastery:** **"Feral Avatar"**
    *   **Battle Shaman:** Wisdom grants a massive bonus to **Melee Damage** and **Proc Rate**, allowing them to fight alongside their pet.
    *   **Contagion:** Disease DoTs (e.g., Plague) become **Viral**. When an infected enemy takes damage, they release spores that infect nearby enemies.
    *   **Ancestral Curse:** Slows penetrate resistance and exceed the standard % cap.

---

## 2. Combat Profile

### A. Dealing Damage (The Rot)
Shamans kill slowly but surely.
*   **Primary Source:** **DoTs** (Poison/Disease). These are long-duration, high-efficiency killers.
*   **Secondary Source:** **Melee**. With "Feral Avatar", a Shaman hits like a Monk while their DoTs tick.
*   **Pet:** A Spirit Wolf that adds DPS and can off-tank.

### B. Taking Damage (The Slow Tank)
Shamans wear Chain. They are sturdy.
*   **Mitigation:** **Slow**. A slowed enemy deals 75% less damage. This is effectively 75% mitigation.
*   **Survival:** **Regeneration**. Shamans have the strongest Regen buffs in the game.
*   **Recovery:** **Cannibalize**. Converts HP into Mana. Combined with Regen, this means infinite mana.

---

## 3. The Toolkit

### A. Damage Tools
1.  **Breath of Wunshi (DoT):** Massive Poison DoT.
2.  **Pox of Bertoxxulous (DoT):** Spreading Disease DoT.
3.  **Spirit Strike (Nuke):** Cold damage nuke (less efficient than DoTs).
4.  **Spirit Wolf (Pet):** A solid pet that benefits from Shaman buffs.

### B. Mitigation Tools
1.  **Turgur's Insects (Slow):** Reduces enemy attack speed by 75%. The single most powerful debuff in the game.
2.  **Outer Realm (Buff):** AC and HP buff.
3.  **Chloroplast (Regen):** Massive HP regeneration.

### C. Utility
1.  **Cannibalize:** The Shaman's signature. HP -> Mana.
2.  **Shrink:** Essential for dungeon crawling.
3.  **Sow:** Movement speed.
4.  **Malo:** Reduces enemy resistances (makes spells land).

### D. Area Effect (AE) Potential
*   **Pandemic:** Pull a group, infect the tankiest mob with "Plague", and watch it spread to the rest.
*   **Rain of Poison:** Targeted AOE DoT.

---

## 4. Solo Strategy
*   **Level 1-30:** Melee with buffs.
*   **Level 30-60:** **Root-Rot**. Root the mob, apply DoTs, Cannibalize mana back, repeat.
*   **Level 70+:** **Face Tank**. Slow the mob. Your Regen out-heals their slowed damage. You melee them with your pet while your DoTs eat them alive. You end the fight with 100% Mana.

---

## 5. Class Scores (1-10)

| Category | Score | Notes |
| :--- | :--- | :--- |
| **Single Target DPS** | **8/10** | High sustained DPS via DoTs + Melee. |
| **AE DPS** | **4/10** | Limited AOE options compared to Druid/Wizard. |
| **Tankiness** | **9/10** | Chain Armor + Slow + Regen = Unkillable. |
| **Sustainability** | **10/10** | Cannibalize = Infinite Mana. |
| **Utility** | **9/10** | Slow, Buffs, Sow, Shrink. |
| **Pet Power** | **7/10** | Good pet, but not a Mage pet. |

---

## 6. Summary
The Shaman is the **King of Attrition**. You can kill anything that can be slowed. You never run out of mana. You never run out of health. It might take a minute to kill the dragon, but the dragon has absolutely zero chance of killing you.

---

## 7. The Big Three: Core Abilities

These abilities reflect the Shaman's role as a primordial force of nature and spirit, scaling with wisdom and level.

### I. [Offensive] Avatar of War
*   **Description:** The Shaman imprints the spirit of the ultimate warrior onto an ally. Drastically increases **Attack Speed**, **Accuracy**, and **Double Attack** chance.
*   **Scaling:** $Accuracy = (\text{Level} \times 2) + (\text{WIS} \times 0.2)$. $Haste = (\text{Level} \times 0.5) + (\text{WIS} \times 0.2)\%$.
*   **Cooldown:** 60 Seconds.
*   **Duration:** 12 Seconds.
*   **Solo Use:** Turns the Shaman and their pet into a melee whirlwind.
*   **Synergy:** **Melee Multiplier**. This is the single best melee buff in the game. It makes any physical attacker (Warrior, Rogue, Monk, etc.) deal exponentially more damage.
*   **Stats:** Scales with **WIS** (Spiritual Connection).

### II. [Defensive] Ancestral Guard
*   **Description:** The Shaman calls upon the spirits of the ancestors to shield an ally. Grants a high-potency **Damage Shield** and a unique "Super-Slow" that affects even slow-immune bosses.
*   **Scaling:** $Slow = (\text{Level} \times 0.5) + (\text{STA} \times 0.05)\%$. $DS = (\text{Level}^{1.5} \times 0.5) + (\text{WIS} \times 2)$.
*   **Cooldown:** 90 Seconds.
*   **Duration:** 8 Seconds.
*   **Solo Use:** Vital for surviving the hardest-hitting bosses.
*   **Group/Synergy:** Provides a major defensive layers to the group's tank.

### III. [Utility/Synergy] Spirit Transfuse
*   **Description:** The Shaman sacrifices their own vitality to restore the spirits of their allies. Converts Shaman HP into **Group Mana Restoration**.
*   **Scaling:** $ManaRestore = (\text{Level} \times 2) + (\text{WIS} \times 5)$ per ally.
*   **Cooldown:** 120 Seconds.
*   **Solo Use:** Instant mana restoration to keep the Shaman's own casting cycle going.
*   **Synergy:** **Energy Multiplier**. This is a powerful tool for magic-based groups, acting as an "emergency battery" to keep healers and nukers from going dry.
*   **Stats:** Scales with **WIS** (Sacrificial Wisdom).
