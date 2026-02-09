# Cleric - Class Design Document

## 1. Class Fantasy: "The Divine Aegis"
The Cleric is the vessel of the gods. In a Solo Server environment, they are not just "heal bots"; they are **Battle Medics** clad in plate armor. They survive by converting their massive healing potential into defense and unleashing the wrath of the divine upon their enemies.

*   **Primary Stat:** **Wisdom (WIS)**
*   **Class Mastery:** **"Divine Retribution"**
    *   **Holy Smite:** Wisdom adds raw **Holy Damage** to all Undead Nukes (making them work on *all* enemies) and adds a "Holy Shock" proc to melee attacks.
    *   **Aegis:** Overhealing (healing a full HP target) converts 50% of the heal into a temporary **Absorb Shield**.

---

## 2. Combat Profile

### A. Dealing Damage (The Smiter)
Clerics are slow but inevitable.
*   **Primary Source:** **Undead Nukes** (Converted to Universal via Mastery).
*   **Secondary Source:** **"Hammer" Pets**. Animated weapons that fight for the Cleric.
*   **Mechanic:** **"Yaulp"**. A short-duration buff that increases Haste, Strength, and Mana Regen, encouraging the Cleric to stay in melee range.

### B. Taking Damage (The Plate Wall)
Clerics wear Plate Armor, giving them high AC.
*   **Mitigation:** High AC + Wisdom-based Spell Shielding.
*   **Survival:** **"Aegis"**. By spamming heals on themselves, they build massive Absorb Shields, effectively extending their HP bar indefinitely as long as they have mana.
*   **Panic Button:** **Divine Barrier**. Invulnerability for 18 seconds (but you can't cast).

---

## 3. The Toolkit

### A. Damage Tools
1.  **Condemnation (Nuke):** Fast-casting Holy damage.
2.  **Summon Hammer (Pet):** A floating warhammer that attacks enemies.
3.  **Word of War (PBAOE):** A point-blank AOE nuke that damages enemies and stuns them.
4.  **Retribution (Reverse DS):** A debuff on the enemy that causes them to take Holy damage every time they attack.

### B. Mitigation Tools
1.  **Divine Aura/Barrier:** Complete Invulnerability.
2.  **Armor of the Faithful:** Massive AC and HP buff.
3.  **Sanctuary:** Reduces aggro generation (useful when letting the Hammer pet tank).

### C. Utility
1.  **Divine Intervention (Buff):** If the Cleric dies, they are instantly healed to full HP (10 min cooldown). Replaces the need for "Resurrection" in solo play.
2.  **Pacify:** Reduces aggro radius, allowing the Cleric to split pulls.
3.  **Root:** Keeps an enemy in place while the Cleric nukes or heals.
4.  **Complete Heal:** The most mana-efficient heal in the game.

### D. Area Effect (AE) Potential
*   **Word Spells:** PBAOE Nukes.
*   **Earthquake:** A massive AOE nuke with a long cooldown.

---

## 4. Solo Strategy
*   **Level 1-30:** Melee with Yaulp. Tank with Plate Armor.
*   **Level 30-60:** **Root-Rot**. Root the mob, apply Retribution, send in the Hammer pet, and nuke.
*   **Level 70+:** **Aegis Tanking**. Pull a group. Cast a massive heal on yourself to build a 10k Absorb Shield. Spam "Word of War" to stun-lock and kill the pack while the shield holds.

---

## 5. Class Scores (1-10)

| Category | Score | Notes |
| :--- | :--- | :--- |
| **Single Target DPS** | **6/10** | Respectable with Holy Nukes, but mana intensive. |
| **AE DPS** | **7/10** | "Word" spells are very strong PBAOE. |
| **Tankiness** | **9/10** | Plate Armor + Absorb Shields + Infinite Healing. |
| **Sustainability** | **8/10** | Great HP sustain, but Mana is the limiting factor. |
| **Utility** | **6/10** | Rez is great, but lacks Snare/Slow/Teleport. |
| **Pet Power** | **4/10** | The Hammer is a DOT, not a tank. |

---

## 6. Summary
The Cleric is the **Indestructible Caster**. You wear the heaviest armor and wield the strongest magic. You don't kite; you stand your ground, stun your enemies, and blast them with holy light.

---

## 7. The Big Three: Core Abilities

These abilities represent the zenith of divine power, scaling with wisdom and holiness.

### I. [Offensive] Celestial Wrath
*   **Description:** A blinding beam of pure celestial light that strikes the target for massive Holy damage and applies a short-duration Blind effect.
*   **Scaling:** $Damage = (\text{Level}^{2.1} \times 1.25) + (\text{WIS} \times 15)$.
*   **Cooldown:** 45 Seconds.
*   **Duration:** Instant / 8 Seconds (Blind).
*   **Solo Use:** Massive burst damage and crowd control for dangerous mobs.
*   **Group/Synergy:** None (Pure DPS).

### II. [Defensive] Divine Aegis
*   **Description:** The Cleric focuses their healing energy inward, creating an shimmering barrier of light that absorbs incoming damage.
*   **Scaling:** $Absorb = (\text{Level}^{2.0} \times 2.0) + (\text{WIS} \times 20)$.
*   **Cooldown:** 120 Seconds.
*   **Duration:** 10 Seconds.
*   **Solo Use:** Essentially doubles the Cleric's HP pool, allowing them to tank multiple enemies.
*   **Group/Synergy:** Can be cast on others, but has reduced efficiency.

### III. [Utility/Synergy] Aura of Purity
*   **Description:** A cleansing light emanates from the Cleric. All allies within the aura have a chance to automatically **Cure** negative status effects every 6 seconds and gain increased **Mana Regeneration**.
*   **Scaling:** $ManaRegen = (\text{Level} \times 0.5) + (\text{WIS} \times 0.1)$. $CureChance = (\text{Level} \times 0.5)\%$.
*   **Cooldown:** 90 Seconds.
*   **Duration:** 12 Seconds.
*   **Solo Use:** Essential for maintaining mana during long fights and staying free of debuffs.
*   **Synergy:** **Resource & Survival Multiplier**. This is the ultimate "comfort" aura for a group. It allows casters (Wizards, Magicians) to stay high on mana and protects tanks from being crippled by slows or dots.
*   **Stats:** Scales with **WIS** (Divine Favor).
*   **Synergy Value:** High. Makes any group significantly more resilient and reduces downtime to near zero.
