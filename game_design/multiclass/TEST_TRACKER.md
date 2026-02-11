# Multiclass Test Tracker & Test Plan

**Status**: Active Testing
**Date**: 2026-02-10
**Technical Docs**: [IMPLEMENTATION_PLAN.md](IMPLEMENTATION_PLAN.md)

This document contains detailed step-by-step procedures for every verification point of the Multiclass system.

**Instructions**:

1. Execute the **Procedure** steps in-game.
2. Mark **[x] Pass** or **[x] Fail**.
3. Add notes in the **Comments** section if issues arise.

---

## 🛠️ Global Testing Commands

- `#addclass <id>` : Add a class (Ids: 1=WAR, 2=CLR, 3=PAL, 4=RNG, 5=SHD, 6=DRU, 7=MNK, 8=BRD, 9=ROG, 10=SHM, 11=NEC, 12=WIZ, 13=MAG, 14=ENC, 15=BST, 16=BER)
- `#removeclass <id>`
- `#addclass list`
- `#level 60` (Recommended for most tests)
- `#setskill <id> <value>`

---

## 1. Core Logic & Persistence

### 1.1 Add/Remove Class

**Goal**: Ensure bitmask updates correctly on command.
**Procedure**:

1. Create a character or log in.
2. Type `#addclass 1` (Warrior).
3. Type `#addclass list` -> Verify "Warrior" is listed.
4. Type `#removeclass 1`.
5. Type `#addclass list` -> Verify "Warrior" is NOT listed.
**Status**: [ ] Pass  [ ] Fail
**Comments**: __________________________________________________

### 1.2 Persistence

**Goal**: Ensure class bits survive a session change.
**Procedure**:

1. `#addclass 1` (Warrior) and `#addclass 2` (Cleric).
2. Type `#camp` to return to character select.
3. Log back in to the world.
4. Type `#addclass list`.
**Expected**: Both Warrior and Cleric are still listed.
**Status**: [ ] Pass  [ ] Fail
**Comments**: __________________________________________________

### 1.3 Client Sync (Login)

**Goal**: Ensure UI connects correctly on login.
**Procedure**:

1. Ensure your character has a mana-using class (e.g. `#addclass 12` Wizard).
2. Camp and log in.
3. Look at the Player Window.
**Expected**: You should see a Mana Bar (blue bar) even if your base class is Warrior.
**Status**: [ ] Pass  [ ] Fail
**Comments**: __________________________________________________

### 1.4 Hot-Reload

**Goal**: Ensure UI updates immediately without zoning.
**Procedure**:

1. Start as a Pure Warrior (No Mana Bar).
2. Type `#addclass 12` (Wizard).
**Expected**: The Mana Bar should appear instantly on your screen.
**Status**: [ ] Pass  [ ] Fail
**Comments**: __________________________________________________

---

## 2. Spells & Casting

### 2.1 Hybrid Scribing

**Goal**: Scribe a spell not available to base class.
**Procedure**:

1. Be a Warrior (Base).
2. `#addclass 12` (Wizard). `#level 50`.
3. `#scribe 12 10` (Scribe 'Frost Shock' or similar Level 12 spell to valid slot).
   *Alternatively*: Give spell scroll `Item ID: 15212` (Frost Shock) and click to learn.
**Expected**: System message "You have finished scribing...". Spell appears in book.
**Status**: [ ] Pass  [ ] Fail
**Comments**: __________________________________________________

### 2.2 Hybrid Casting

**Goal**: Cast a spell from a secondary class.
**Procedure**:

1. Memorize the spell from Test 2.1.
2. Target self or valid target.
3. Cast the spell.
**Expected**: Spell casts, consumes mana, and applies effect.
**Status**: [ ] Pass  [ ] Fail
**Comments**: __________________________________________________

### 2.3 Bard Song Casting

**Goal**: Verify access to Bard songs.
**Procedure**:

1. Base Class: Paladin.
2. `#addclass 8` (Bard). `#level 50`.
3. `#scribe 120` (Selo's Accelerando). Memorize it.
4. Cast Selo's Accelerando.
**Expected**: Cast completes, buff icon appears. Duration should be > 1 tick (scaled to Level 50).
**Status**: [ ] Pass  [ ] Fail
**Comments**: __________________________________________________

### 2.4 Bard Song (Melee Checking)

**Goal**: Verify songs do not stop melee combat (Cast-While-Moving logic).
**Procedure**:

1. Engage a combat dummy/target with Auto-Attack ON.
2. Cast Selo's Accelerando (Bard Song).
**Expected**: Auto-attack continues swinging *during* the cast bar. Combat does not stop.
**Status**: [ ] Pass  [ ] Fail
**Comments**: __________________________________________________

### 2.5 Standard Casting (Melee Checking)

**Goal**: Verify standard spells DO stop melee combat.
**Procedure**:

1. `#addclass 12` (Wizard). Memorize a nuke (e.g., `#scribe 12 10`).
2. Engage target with Auto-Attack ON.
3. Cast the Nuke.
**Expected**: Auto-attack stops immediately when casting begins to prevent "Battle Mage" exploitation unless intended.
**Status**: [ ] Pass  [ ] Fail
**Comments**: __________________________________________________

### 2.6 Group Targets

**Goal**: Verify group checks recognize the caster.
**Procedure**:

1. Form a group with another player or bot.
2. Cast a Group Buff for a secondary class (e.g. Cleric `Heroism`).
**Expected**: Buff lands on you and group members.
**Status**: [ ] Pass  [ ] Fail
**Comments**: __________________________________________________

---

## 3. Combat & Skills

### 3.1 Skill Caps

**Goal**: Verify the "Best of" skill cap logic.
**Procedure**:

1. Base Class: Wizard (Low 1H Blunt cap).
2. `#addclass 1` (Warrior). `#level 60`.
3. `#setskill 0 300` (1H Blunt).
**Expected**: Skill sets to ~200+ (Warrior cap), not capped at Wizard limit (~100).
**Status**: [ ] Pass  [ ] Fail
**Comments**: __________________________________________________

### 3.2 Skill Training (Level 1)

**Goal**: Verify low-level skill access.
**Procedure**:

1. Make Level 1 Ranger.
2. `#addclass 1` (Warrior).
3. Open Skills window or use `#setskill 30 1` (Kick).
**Expected**: Success. (Ranger normally gets Kick at Lvl 5, Warrior at Lvl 1).
**Status**: [ ] Pass  [ ] Fail
**Comments**: __________________________________________________

### 3.3 Trainer Window

**Goal**: Verify GM Trainer access.
**Procedure**:

1. Base Class: Warrior. `#addclass 12` (Wizard).
2. Target a Wizard GM NPC.
3. Right-click / hail.
**Expected**: The Skill Trainer window opens. (Normally says "I am not your master").
**Status**: [ ] Pass  [ ] Fail
**Comments**: __________________________________________________

### 3.4 Discipline Learning

**Goal**: Verify Discipline tome consumption.
**Procedure**:

1. Base Class: Ranger. `#addclass 1` (Warrior).
2. Use `#summon 20683` (Tome of Stone Stance).
3. Right-click Tome.
**Expected**: "You have learned Stone Stance".
**Status**: [ ] Pass  [ ] Fail
**Comments**: __________________________________________________

### 3.5 Discipline Usage

**Goal**: Verify Combat Ability activation.
**Procedure**:

1. Open Combat Abilities window (Ctrl+C).
2. Create button for "Stone Stance".
3. Press button.
**Expected**: "You assume the durability of stone." (Buff active).
**Status**: [ ] Pass  [ ] Fail
**Comments**: __________________________________________________

### 3.6 Monk Specials (Flying Kick)

**Goal**: Verify `DoClassAttacks` handles secondary Monk.
**Procedure**:

1. Base: Warrior. `#addclass 7` (Monk). `#level 60`.
2. `#setskill 38 300` (Flying Kick). `#setskill 30 300` (Kick).
3. Turn on Auto-Attack.
**Expected**: Combat log shows "You try to flying kick..." messages, not "You try to kick...".
**Status**: [ ] Pass  [ ] Fail
**Comments**: __________________________________________________

### 3.7 Damage Caps

**Goal**: Verify melee damage limits.
**Procedure**:

1. Base: Cleric. `#level 20`.
2. Hit a dummy. Note max damage.
3. `#addclass 1` (Warrior).
4. Hit dummy.
**Expected**: Max damage potential increases (Warrior table vs Cleric table).
**Status**: [ ] Pass  [ ] Fail
**Comments**: __________________________________________________

---

## 4. Items & Equipment

### 4.1 Equip Logic

**Goal**: Verify "Classes: WIZ" item works on WAR/WIZ.
**Procedure**:

1. Base: Warrior. `#addclass 12` (Wizard).
2. `#summon 9444` (Gossamer Robe - WIZ only).
3. Equip to Chest.
**Expected**: Item equips successfully. Main view updates.
**Status**: [ ] Pass  [ ] Fail
**Comments**: __________________________________________________

### 4.2 Race Logic

**Goal**: Verify Race restrictions are STRICT data checks (NOT bypassed).
**Procedure**:

1. Race: Human. Class: Warrior.
2. `#summon 4516` (Small Banded Helm - Small races only).
3. Attempt to equip.
**Expected**: Error message "Your race cannot wear this item."
**Status**: [ ] Pass  [ ] Fail
**Comments**: __________________________________________________

### 4.3 Weapon Procs

**Goal**: Verify class-restricted procs.
**Procedure**:

1. Base: Warrior. `#addclass 12` (Wizard).
2. Summon a weapon with a Wizard-Specific proc (or custom item).
3. Swing until proc.
**Expected**: Proc fires successfully.
**Status**: [ ] Pass  [ ] Fail
**Comments**: __________________________________________________

### 4.4 Right-Click Items

**Goal**: Verify `CheckItemRaceClassDietyRestrictions`.
**Procedure**:

1. Base: Warrior. `#addclass 2` (Cleric).
2. Summon Item with "Effect: ... (Must be Cleric)".
3. Right click.
**Expected**: Cast bar appears / Effect triggers.
**Status**: [ ] Pass  [ ] Fail
**Comments**: __________________________________________________

### 4.5 Epic Quests

**Goal**: Verify Quest turn-ins.
**Procedure**:

1. Find a secondary class Epic Quest NPC.
2. Hand in a required item.
**Expected**: NPC accepts item (Quest text triggers), does not return it saying "I have no need for this".
**Status**: [ ] Pass  [ ] Fail
**Comments**: __________________________________________________

---

## 5. XP & Stats

### 5.1 XP Bonus

**Goal**: Verify Hybrid XP modifiers.
**Procedure**:

1. `#addclass 1` (Warrior - Bonus XP) or `#addclass 9` (Rogue - Bonus XP).
2. Kill a specific mob ID. Note XP gain message (if descriptive) or debug output.
3. `#removeclass 1`. Kill same mob.
**Expected**: XP values differ based on class bonuses defined in `exp.cpp`.
**Status**: [ ] Pass  [ ] Fail
**Comments**: __________________________________________________

### 5.2 Food Consumption

**Goal**: Verify Monk hunger rate.
**Procedure**:

1. `#addclass 7` (Monk).
2. Wait for hunger ticks.
**Expected**: Food consumes faster than standard classes.
**Status**: [ ] Pass  [ ] Fail
**Comments**: __________________________________________________

### 5.3 Regen

**Goal**: Verify HP Regen formulas.
**Procedure**:

1. Be Iksar or Troll (optional).
2. `#addclass 7` (Monk) or `#addclass 15` (Beastlord).
3. Sit down.
4. Observe HP tick size.
**Expected**: Higher regen tick than standard Warrior/Cleric.
**Status**: [ ] Pass  [ ] Fail
**Comments**: __________________________________________________

---

## 6. AA System

### 6.1 Visibility

**Goal**: Verify AA Tabs.
**Procedure**:

1. `#level 60`. `#addclass 2` (Cleric) `#addclass 12` (Wizard).
2. Open AA Window (`V`).
**Expected**: You see tabs/AAs for both Cleric (e.g., MGB) and Wizard (e.g., Manaburn).
**Status**: [ ] Pass  [ ] Fail
**Comments**: __________________________________________________

### 6.2 Purchase & Activation

**Goal**: Buy and use a secondary AA.
**Procedure**:

1. Grant AA points (`#setaa 100`).
2. Buy a secondary class active AA (e.g. Exodus for Wizard on a Warrior).
3. Make hotkey. Use it.
**Expected**: Ability activates.
**Status**: [ ] Pass  [ ] Fail
**Comments**: __________________________________________________

### 6.3 Passive Effects

**Goal**: verify passive bonuses.
**Procedure**:

1. Buy "Innate Run Speed" (General) or a Class passive (e.g. Spell Casting Mastery).
2. Verify effect (Run faster / Less mana cost).
**Expected**: Passive persists.
**Status**: [ ] Pass  [ ] Fail
**Comments**: __________________________________________________
