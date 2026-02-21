# Multiclass Test Tracker & Test Plan

**Status**: Active Testing
**Date**: 2026-02-21
**Technical Docs**: [IMPLEMENTATION_PLAN.md](IMPLEMENTATION_PLAN.md)

## Recent Implementation Updates

- **2026-02-21**: THJ parity follow-up for profile class hydration.
  - `zone/client_packet.cpp::CompleteConnect()` now seeds `m_pp.classes` directly from `GestaltClasses` and primes `m_classes_bits_cache` (THJ-style).
  - `zone/client.cpp::GetClassesBits()` now prefers `m_pp.classes` when present before bucket fallback, matching THJ runtime behavior and reducing stale bucket-path reads.
  - `zone/client_packet.cpp::CompleteConnect()` ordering updated so legacy bucket migration runs before `m_pp.classes` hydration.
  - DLL fallback hardened: if runtime multiclass mask is still missing, cached char-select multiclass mask (from Deity override) is now used as final fallback for `GetUsableClasses` routing.

- **2026-02-21**: THJ parity follow-up for char-select multiclass payload shaping.
  - `world/worlddb.cpp` now queries `GestaltClasses` without extra scope filters (THJ-style lookup behavior).
  - Char-select class/deity fields are now set from multiclass bits during initial character entry construction (instead of RoF2-only post-pass deity patching).
  - Start-zone lookup now uses the actual outgoing `cse->Deity` value, matching THJ’s multiclass char-select flow.

- **2026-02-21**: THJ parity alignment for multiclass client data path.
  - Reverted `zone/client.cpp::SendEdgeStats()` class-key payload so multiclass mask is no longer sent via EdgeStat key `200` (THJ-style behavior).
  - Updated DLL profile parsing in `extras/eq-core-dll-main/src/eqgame.cpp` to resolve multiclass mask from `OP_PlayerProfile` using THJ-compatible offsets (`19564` primary, `19568` fallback).
  - Added diagnostics:
    - `[PLAYER_PROFILE_MASK] ...`
    - `[PLAYER_PROFILE_MASK_WARN] ...`
  - `GetUsableClasses` now uses an effective mask (`EdgeStat mask` OR `profile classes mask`) so equip gating works with profile-only multiclass delivery.

- **2026-02-21**: DLL warning-noise cleanup in `extras/eq-core-dll-main/src/MQ2DataTypes.h`.
  - Replaced legacy `static enum ...` class declarations with `enum ...` to address repeated MSVC warning `C4091`.
  - Goal: reduce high-volume compile warning noise so multiclass/debug warnings are easier to spot.

- **2026-02-21**: DLL debug logging path hardening in `extras/eq-core-dll-main/src/eqgame.cpp`.
  - `LogDebug` / `LogRawDebug` now mirror writes to both local `dinput8_debug.log` and repo log `logs/dinput8_debug.log`.
  - Added startup breadcrumb line with multiclass option states to confirm active DLL and loaded toggles at launch.

- **2026-02-21**: DLL multiclass equip gating fix in `extras/eq-core-dll-main/src/eqgame.cpp`.
  - `EQCharacter_GetUsableClasses_Detour` changed from RVA whitelist mode to mask-first mode.
  - Behavior now returns server multiclass class mask by default for usability/equip checks, with a tiny native-only denylist for display-only context.
  - Goal: prevent missed equip callsites (RVA drift) that caused client error `"your class isn't right"` for valid multiclass equipment.
  - Added debug toggle `isMulticlassUsableClassesVerboseLoggingEnabled` (`_options.h`) to dump every `GetUsableClasses` decision to `dinput8_debug.log`.
  - Added one-time warning log when multiclass override is enabled but server mask is still `0`.

- **2026-02-21**: DLL usable-classes verbose logging noise reduction in `extras/eq-core-dll-main/src/eqgame.cpp`.
  - `GetUsableClasses` verbose logging now deduplicates identical consecutive decisions and emits a compact `suppressed=<N> repeats` summary.
  - Added `ctx=<name>` context tag to logs for known RVAs (`equip_validation`, `item_use_check`, `tooltip_classes`, etc.).
  - Goal: keep `dinput8_debug.log` readable while preserving callsite/debug context.

- **2026-02-21**: DLL usable-classes routing refinement for client equip deny behavior.
  - `GetUsableClasses` override now applies when native class bits overlap owned multiclass bits (instead of strict base-class-only matching).
  - Non-overlapping class masks (including non-owned classes like ENC-only) remain native to prevent false client-side allows and inventory desync.
  - Added log fields: `base_mask` and `apply_multi` to confirm routing decisions during equip checks.

- **2026-02-21**: Ranger archery close-range server check adjustment in `zone/special_attacks.cpp`.
  - In `Client::RangedAttack`, ranger-class characters now use `0` minimum ranged distance for bow attacks on the server-side validation path.
  - Goal: allow ranger autofire/bow usage at point-blank range without server-side `RANGED_TOO_CLOSE` rejection.

- **2026-02-21**: THJ-style implied spell targeting port in `zone/spells.cpp` / `zone/mob.h`.
  - Added `Mob::GetSpellImpliedTargetID(spell_id, target_id)` and invoked it at cast start in `Mob::CastSpell`.
  - Beneficial spells now smart-reroute to a valid friendly target (`target`, then `target's target`, then self fallback).
  - Detrimental spells remain hostile-target oriented and may fall through to `target's target` or pet target when appropriate.
  - Guardrails retained for special spell categories (charm, corpse, PB-AE, cancel magic, alliance/lull, self-target).

- **2026-02-21**: `eqemu_config.json` world TCP port changed `9000 -> 9100`.
  - Reason: recurring local conflict with VS Code/Jupyter Python kernel binding `127.0.0.1:9000`, causing immediate world startup failure (`World port already in use`).
  - Action: restart world stack so all services reconnect to world on `9100`.

- **2026-02-21**: `server_manager.py` main UX layout pass (operations rail + live workspace).
  - Main tab split into a left operations rail (`Build & Deploy`, `Server Actions`, `Client Sync`, `Client Asset Status`) and right live workspace.
  - Process table replaced with compact service cards (3-column grid) with clear status tinting (`Running`, `External`, `Stopped`).
  - Per-process arguments/console remain hidden behind `Options`, with in-card hint text showing effective runtime settings.
  - Runtime log viewer now includes quick-focus buttons: `Active Zone Log`, `Active World Log`, `Most Recent Log`.
  - Added process summary strip (`Running X/Y | External Z`) for faster state scanning.

- **2026-02-21**: `zone/client_process.cpp` trainer handlers updated for multiclass parity.
  - `OPGMTraining` now allows opening class trainer windows when the trainer's class is present in `GetClassesBits()` (`HasClass()`), not only base class.
  - `OPGMEndTraining` now uses the same multiclass-aware class check.
  - Trainer skill caps in `OPGMTraining` now use the trainer class in multiclass mode (THJ-style behavior), so the window reflects that guild trainer's skill domain.

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

## 0. Tooling UX Smoke Test (Server Manager)

### 0.1 Main Layout Scan

**Goal**: Confirm main tab is visually consolidated and workflow-oriented.
**Procedure**:

1. Open `server_manager.py` UI.
2. Go to `Server Control` tab.
3. Verify the left rail shows grouped cards (`Build & Deploy`, `Server Actions`, `Client Sync`, `Client Asset Status`).
4. Verify the right side shows `Live Server Workspace`, `Service Grid`, and logs area.
**Status**: [ ] Pass  [ ] Fail
**Comments**: __________________________________________________

### 0.2 Process Card Behavior

**Goal**: Confirm process cards reflect state and keep advanced fields hidden by default.
**Procedure**:

1. Start `world` and `eqlaunch` from the service cards.
2. Confirm each card changes to running style and Start button disables.
3. Click `Options` for `eqlaunch`, set custom args, close dialog.
4. Confirm card hint text updates without exposing full argument fields in the main view.
**Status**: [ ] Pass  [ ] Fail
**Comments**: __________________________________________________

### 0.3 Runtime Log Focus

**Goal**: Confirm quick log focus actions work for active debugging.
**Procedure**:

1. Open `Runtime Log Viewer`.
2. Click `Active Zone Log` and confirm source updates to a `zone*.log`.
3. Click `Active World Log` and confirm source updates to a `world*.log`.
4. Click `Most Recent Log` and confirm source follows latest touched log file.
**Status**: [ ] Pass  [ ] Fail
**Comments**: __________________________________________________

### 0.4 DLL Equip Gate (Multiclass)

**Goal**: Confirm client allows equip when item class mask matches any owned multiclass class.
**Procedure**:

1. Base class Ranger; add Warrior and Mage (`#addclass 1`, `#addclass 13`).
2. Ensure class list shows Ranger/Warrior/Mage via your class listing command.
3. Try to equip a Warrior-only item.
4. Try to equip a Mage-only item.
**Expected**: Client permits equip attempts for both items (no client-side `"your class isn't right"` block).  
Server remains final authority for any unrelated equip rules.
**Status**: [ ] Pass  [ ] Fail
**Comments**: __________________________________________________

### 0.5 DLL Equip Gate Negative Case

**Goal**: Confirm client still blocks non-owned classes while multiclass allows owned classes.
**Procedure**:

1. Base class Ranger; add Warrior and Mage (`#addclass 1`, `#addclass 13`).
2. Attempt to equip an Enchanter-only item.
3. Watch chat output and inventory slot behavior.
**Expected**: Client/server denies equip for Enchanter-only item.  
If a temporary inventory resync message appears, source/destination slot states recover and item remains not equipped.
**Status**: [ ] Pass  [ ] Fail
**Comments**: __________________________________________________

### 0.6 Ranger Autofire Point-Blank

**Goal**: Confirm ranger bow/autofire works at close range without server-side min-distance rejection.
**Procedure**:

1. Use a character with Ranger class access (`HasClass(Ranger)`), equip bow + arrows.
2. Stand directly next to a valid target (melee distance).
3. Enable autofire and observe attack attempts.
**Expected**: Bow/autofire shots execute at point-blank range; no `RANGED_TOO_CLOSE` server rejection for ranger.
**Status**: [ ] Pass  [ ] Fail
**Comments**: __________________________________________________

### 0.7 Smart Spell Targeting (THJ-style)

**Goal**: Confirm beneficial spells reroute intelligently when hostile target is selected.
**Procedure**:

1. Ensure `Spells:UseSpellImpliedTargeting` is enabled.
2. Target an NPC that is actively targeting you.
3. Cast a single-target beneficial spell (example: direct heal).
4. Repeat with no valid target selected and cast the same beneficial spell.
5. Cast a detrimental spell with the same NPC target to confirm hostile targeting still applies.
**Expected**:
- Beneficial cast lands on a valid friendly implied target (self in the common NPC-targeting-you case).
- Beneficial cast without a valid target falls back to self.
- Detrimental cast remains on appropriate hostile implied target path and does not reroute to self.
**Status**: [ ] Pass  [ ] Fail
**Comments**: __________________________________________________

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
2. Select a target (yourself works).
3. Cast.
**Expected**: Spell lands successfully, consumes mana, and applies effect.
**Status**: [ ] Pass  [ ] Fail
**Comments**: __________________________________________________

### 2.3 Bard Song Pulse (Persistence)

**Goal**: Verify Bard songs re-pulse (refresh) when they hit 0 ticks if still memorized.
**Procedure**:

1. Ensure you have the Bard class (`#addclass 8`).
2. Meditate/Sit to recover mana/endurance.
3. Memorize a beneficial song (e.g. Hymn of Restoration, lvl 6).
4. Start singing the song targeting yourself.
5. Wait for the duration to run out (hit 0 ticks).
**Expected**: The song should **not fade**. Instead, it should refresh its duration back to full (3 ticks) automatically, simulating a continuous "pulse".
**Status**: [ ] Pass  [ ] Fail
**Comments**: __________________________________________________

### 2.4 Infinite Buffs (Non-Bard)

**Goal**: Verify beneficial buffs do not expire on multiclass characters.
**Procedure**:

1. Cast a standard beneficial buff on yourself (e.g. skin/symbol/spirit of wolf).
2. Wait for ticks to count down.
**Expected**: The duration counter should either stop decreasing or reset, effectively making the buff permanent until cancelled or dispelled.
**Status**: [ ] Pass  [ ] Fail
**Comments**: __________________________________________________
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

### 3.3a Trainer End Session (Multiclass)

**Goal**: Verify training close flow is also multiclass-aware.
**Procedure**:

1. Base Class: Warrior. `#addclass 12` (Wizard).
2. Open a Wizard GM trainer window.
3. Click **Done** / close the training session.
**Expected**: Session closes normally, trainer sends departure text, no denial due to base class mismatch.
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
