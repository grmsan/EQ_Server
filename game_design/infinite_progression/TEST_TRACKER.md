# Infinite Item Progression Test Tracker

**Status**: Active Development
**Tracker Area**: Infinite Item Progression
**Tracker State**: Active
**Last Updated**: 2026-05-02
**Implementation Plan**: [IMPLEMENTATION_STEPS.md](IMPLEMENTATION_STEPS.md)

## Purpose

Single source of truth for infinite item progression validation. Cases are scenario-level — each validates a complete player-facing workflow, not individual function inputs.

Tests marked `[AUTO]` can be validated primarily via SQL queries or GM commands without sustained in-game play.
Tests marked `[SOAK]` require extended play sessions; exclude from standard smoke runs.

## Status Rules

- `Not Run`: Ready for validation but not executed in the current build.
- `In Progress`: Actively being tested or partially executed.
- `Blocked`: Cannot be validated due to environment, data, or design dependency.
- `Pass`: Expected behavior confirmed with objective evidence.
- `Fail`: Behavior mismatches expected results or produces a regression.

## Evidence Format

Keep notes short and reproducible:

`Observed:` one-line actual result.
`Evidence:` exact chat/log snippet, command output, or screenshot reference.
`Commands:` minimal command sequence to reproduce.
`Next:` immediate code/data target if blocked or failed.

## Current Validation Focus

1. Run **IP-01** and **IP-03** after every fresh build — confirm the core iLevel engine and tier tables are wired.
2. Run **IP-04** (Kill XP and tier-up) and **IP-09** (Ghost Copy) for the main player-facing loop.
3. Run **IP-05** through **IP-07** when economy rules or Essence math changes.
4. Run **IP-02**, **IP-08**, and **IP-12** (all `[AUTO]`) at any time — no client needed.
5. Run **IP-15** (Merge) and **IP-16** (Infusion) before merge/release.

## Global Commands

- `#ilevel [item_id]`: Show iLevel for an item
- `#ilevel all`: Batch-calculate iLevel for all items
- `#itemtier [slot_id tier]`: Show/set item tier on a slot (0=Base, 1=Enchanted, 2=Legendary, 3=Mythic)
- `#powerslot`: Show Power Slot item, tier, XP, progress bar
- `#powerslot reset` / `#powerslot setxp <N>`: Manipulate XP for testing
- `#salvage`: Manually trigger Salvage Satchel processing
- `#essence`: Show/modify Essence balance
- `#altcurrency add <id> <amount>`: Grant currency (100=CE, 101=RE)
- `#si <item_id>`: Summon item to cursor

## Run Packs

### Smoke Run (30 min)

Run `IP-01`, `IP-03`, `IP-04`, `IP-05`, `IP-09`.

### Economy Smoke (20 min)

Run `IP-06`, `IP-07`, `IP-13`, `IP-14` after Smoke Run passes.

### DB Validation (5 min — no client needed)

Run `IP-02`, `IP-08`, `IP-12` — SQL queries only.

### Full Regression

Run all active cases. Run `IP-S01` and `IP-S02` separately as soak sessions.

---

## 1) Core: iLevel and Tier Engine

### [IP-01] iLevel Calculation and Rule Weights

**Goal**: Verify the iLevel engine produces correct values, populates the DB in batch, and respects rule-driven weights.
**Steps**:

1. `#ilevel 5019` (Rusty Long Sword — expect ~14 ±2), `#ilevel 5157` (Lamentation — ~77 ±5).
2. Run `#ilevel all`, then: `SELECT COUNT(*) FROM items WHERE calculated_ilevel > 0` — count should be substantial.
3. Note Hategiver iLevel (`#ilevel 28854`). Change `WEAPON_AC_WEIGHT` rule to 6.0, run `#rules reload`, re-check — value should increase noticeably.
4. (Optional parity check) Run `python tools/validate_ilevel.py` and spot-check 3 items against `#ilevel` — values should match within ±1.

**Expected**: Item iLevel values match expected ranges, batch populates the DB, and rule weight changes take effect immediately.
**Status**: [x] Not Run  [ ] In Progress  [ ] Blocked  [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [IP-02] [AUTO] DB Tier Verification

**Goal**: Confirm tiered item tables are fully populated with correct stats — SQL only, no client needed.
**Steps**:

1. `python tools/generate_tiered_items.py --verify` — should report expected total.
2. `SELECT COUNT(*) FROM items WHERE id BETWEEN 200000 AND 210000` — verify tier band count.
3. Spot-check: `SELECT id, Name, damage, ac, hp FROM items WHERE id IN (28854, 278854, 528854, 778854)` — verify Base/Enchanted/Legendary/Mythic Hategiver.
   - Enchanted (278854): DMG 30, AC 50, HP 170
   - Legendary (528854): DMG ~39, heroics visible
   - Mythic (778854): Same stats as Legendary, +1 aug slot

**Expected**: Tier band count matches design target (~117,958 per tier). Spot-checked items have scaled stats matching the design.
**Status**: [x] Not Run  [ ] In Progress  [ ] Blocked  [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [IP-03] Tier Upgrade, Downgrade, and Augment Preservation

**Goal**: Verify the full tier-change lifecycle including stat scaling and augment carry-through.
**Steps**:

1. `#si 28854` (Hategiver), equip it, record slot and base stats.
2. `#itemtier <slot> 1` (Enchanted) — inspect stats: DMG 30, AC 50, HP 170.
3. `#itemtier <slot> 2` (Legendary) — inspect: DMG ~39, heroics visible.
4. Socket an augment into the Legendary version.
5. `#itemtier <slot> 3` (Mythic) — verify stats match Legendary, augment still present, +1 aug slot visible.
6. `#itemtier <slot> 0` (Base) — verify revert to original stats; confirm aug behavior is consistent.

**Expected**: Stats match expected scaling at each tier; augments survive all tier changes; Mythic adds aug slot without stat increase.
**Status**: [x] Not Run  [ ] In Progress  [ ] Blocked  [ ] Pass  [ ] Fail
**Notes**: ______________________________

---

## 2) Kill XP and Power Slot

### [IP-04] Kill XP, Tier-Up, and Milestones

**Goal**: Verify the full kill → XP → tier-up loop including milestone messages and max-tier cap.
**Steps**:

1. Place a Base weapon in Power Slot, kill a white-con mob — expect `+40 item XP` message.
2. `#powerslot setxp 240`, kill a white-con mob (+40 → 280 = 28%) — should trigger 25% milestone message.
3. `#powerslot setxp 960`, kill a mob — expect tier-up to Enchanted with message and sound, XP resets to 0.
4. Place a Mythic item in Power Slot, kill a mob — no XP message; `#powerslot` shows "Maximum tier reached!".
5. Verify `#powerslot` progress bar: `#powerslot setxp 500` on a Base item → should show 500/1000, 50%, bar rendered.

**Expected**: XP increments on valid kills, milestones trigger at 25%/50%/75%/90%, tier-up resets XP and promotes item, Mythic earns no further XP.
**Status**: [x] Not Run  [ ] In Progress  [ ] Blocked  [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [IP-05] Con Scaling, Named/Raid Multipliers, and XP Persistence

**Goal**: Verify kill XP varies by con color, named/raid multipliers work, and per-item XP persists across swaps.
**Steps**:

1. Kill a grey-con mob (expect 0 XP), green-con (~5), white-con (~40), yellow-con (~45).
2. Kill a `rare_spawn` NPC at white-con — expect `+120 item XP` (×3 named multiplier).
3. `#npcedit level 55` on a target and kill it — expect `+200 item XP` (×5 raid-tier multiplier).
4. Item A in Power Slot, `#powerslot setxp 500`. Swap to Item B, `#powerslot setxp 200`. Swap back to A — verify still 500.
5. (Optional group test) Form a group, kill a mob — all members with Power Slot items receive XP independently.

**Expected**: Con multipliers, named/raid bonuses, and per-item XP storage all function correctly.
**Status**: [x] Not Run  [ ] In Progress  [ ] Blocked  [ ] Pass  [ ] Fail
**Notes**: ______________________________

---

## 3) Essence Economy

### [IP-06] Salvage and Essence Currency

**Goal**: Verify the Salvage Satchel yields correct Essence and applies tier bonuses.
**Steps**:

1. Place a non-magic item (e.g., Rusty Long Sword) in Salvage Satchel, click Combine — expect rejection with "not magic" message.
2. Place Hategiver (magic, iLevel ~269) in Satchel, click Combine — expect +169 CE (269 - 100).
3. Create an Enchanted Hategiver (`#itemtier <slot> 1`), salvage it — expect ~194 CE (169 × 1.15 tier bonus).
4. Open Alternate Currency tab in Character Sheet — verify CE balance matches expected total.

**Expected**: Non-magic items rejected; magic items yield iLevel-based CE; Enchanted+ items get tier bonus; balance visible in Alt Currency tab.
**Status**: [x] Not Run  [ ] In Progress  [ ] Blocked  [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [IP-07] Consume Item and Consume Essence AAs

**Goal**: Verify both Consume AAs work end-to-end including rejection cases.
**Steps**:

1. Place Base Hategiver in Power Slot. Put a matching Base Hategiver on cursor. Activate Consume Item AA (32100) — expect +33% threshold XP, cursor item destroyed.
2. Put a different weapon on cursor. Activate Consume Item AA — expect "Only duplicates of your Power Source item can be consumed."
3. Place a Mythic item in Power Slot, put a matching item on cursor. Activate — expect "already at maximum tier" rejection.
4. Place Base item in Power Slot, `#powerslot setxp 500`. Grant 500+ CE (`#altcurrency add 100 500`). Activate Consume Essence AA (32101) — expect 500 CE deducted, item tiers up.
5. Set CE to 0 (`#altcurrency add 100 -9999` or use `#essence`). Activate Consume Essence — expect "You have no Common Essence to consume."

**Expected**: Matching item consumed for XP, non-matches rejected with clear messages, Consume Essence depletes balance and can trigger tier-up, zero balance rejected gracefully.
**Status**: [x] Not Run  [ ] In Progress  [ ] Blocked  [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [IP-08] [AUTO] Daily and Weekly Quest Config

**Goal**: Confirm the daily/weekly Essence quests are correctly configured — SQL only, no client needed.
**Steps**:

1. `SELECT id, title, repeatable, replay_timer_seconds FROM tasks WHERE id IN (600000, 600001)` — confirm 86400 (24h) and 604800 (7d) timers.
2. `SELECT taskid, activitytype, goalcount FROM task_activities WHERE taskid IN (600000, 600001)` — confirm activitytype=2 (Kill), goalcount 25 and 150.
3. Confirm `quests/global/player_task_rewards.lua` exists and contains EVENT_TASK_COMPLETE handlers for both task IDs.
4. (Optional in-game) Complete the daily task in-game — verify "You receive 500 Common Essence and 20 Rare Essence!" message and Alt Currency tab updates.

**Expected**: Both tasks configured with correct repeat timers, kill activity type, and goal counts. Lua reward script present with both handlers.
**Status**: [x] Not Run  [ ] In Progress  [ ] Blocked  [ ] Pass  [ ] Fail
**Notes**: ______________________________

---

## 4) Ghost Copy (Power Source → Equipment)

### [IP-09] Ghost Copy Lifecycle

**Goal**: Verify ghost copy placement, removal, regeneration, zone persistence, and rule toggle end-to-end.
**Steps**:

1. Empty Primary slot, equip a 1H weapon in Power Source — expect ghost appears in Primary with placement message.
2. With ghost active, pick up from Primary slot — expect original item on cursor, both PS and Primary empty.
3. Equip a 2H weapon in PS with both hands empty — expect ghost in Primary. Then equip a shield in Secondary — expect ghost disappears (2H needs both hands free).
4. Remove real item from Primary to re-create ghost from PS item — expect ghost reappears.
5. Zone to another zone with ghost active — verify ghost reappears in Primary after zone-in.
6. `#rules set Custom:StatProjectionEnabled false` + `#rules reload`, place weapon in PS — expect no ghost, `#powerslot` shows INACTIVE.

**Expected**: Ghost logic follows slot availability, pickup removes both copies cleanly, zone persistence works, and rule toggle disables the system.
**Status**: [x] Not Run  [ ] In Progress  [ ] Blocked  [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [IP-10] Ghost Stats and Tier Refresh

**Goal**: Verify ghost slot provides expected stats without double-counting and refreshes on tier-up.
**Steps**:

1. Note base stats with empty PS and Primary. Equip a weapon in PS (ghost appears in Primary). Verify stat gains match equipping the item normally in Primary. Confirm stats are NOT doubled.
2. With Base tier weapon in PS (ghost active), `#powerslot setxp <threshold - 1>`, kill a mob to trigger tier-up — verify ghost in Primary refreshes to the Enchanted tier version.
3. With ghost active, click to equip a different weapon over the ghost slot — verify real weapon takes the slot and ghost is removed cleanly; PS item stays in PS.

**Expected**: Stats match one copy of the weapon (not double); ghost refreshes when the PS item tiers up; slot replacement is clean.
**Status**: [x] Not Run  [ ] In Progress  [ ] Blocked  [ ] Pass  [ ] Fail
**Notes**: ______________________________

---

## 5) Drop Tiers

### [IP-11] Drop Tiers, Announcements, and Quest Promotion

**Goal**: Verify dropped items can arrive pre-tiered, rare drops trigger announcements, and quest rewards honor tier config.
**Steps**:

1. Loot an Enchanted drop — inspect stats, expect ×2 base stats and 2 aug slots.
2. `#rules set ItemProgression:DropTierEnabled false` + `#rules reload`, kill 10 mobs — all drops should be Base tier.
3. Kill or force a Legendary/Mythic drop — verify killer and group members see a color-coded announcement; non-group players in zone do NOT see it.
4. Set `QuestItemDefaultTier = 1` (Enchanted), complete a quest with an item reward — verify reward arrives Enchanted.

**Expected**: Pre-tiered drops have scaled stats, rule toggle stops tier promotion, rare drop announcements scope to group, quest tier promotion is explicit (not RNG).
**Status**: [x] Not Run  [ ] In Progress  [ ] Blocked  [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [IP-12] [AUTO] Zone Aug and Global Loot DB Check

**Goal**: Confirm all zone flavor augs, named mob augs, and global loot entries are correctly configured — SQL only.
**Steps**:

1. `SELECT COUNT(*) FROM items WHERE id BETWEEN 201000 AND 201058` — expect 59 rows.
2. `SELECT id, description, zone FROM global_loot WHERE id BETWEEN 100 AND 158` — confirm 59 zone-targeted entries, each with a zone value.
3. `SELECT id, description, rare FROM global_loot WHERE id BETWEEN 170 AND 172` — confirm rare=1 on all 3 entries.
4. `SELECT id, Name, heroic_str, heroic_sta FROM items WHERE id BETWEEN 201500 AND 201510` — verify 11 named augs with multi-stat values.
5. `SELECT * FROM global_loot WHERE zone IN ('mistmoore','sebilis','chardok','velketor') AND id BETWEEN 100 AND 999` — expect 0 rows (no duplicate zone coverage).

**Expected**: 59 zone augs, 59 global_loot zone entries, 3 rare named loot entries, 11 named aug items, 0 duplicates in already-covered zones.
**Status**: [x] Not Run  [ ] In Progress  [ ] Blocked  [ ] Pass  [ ] Fail
**Notes**: ______________________________

---

## 6) Vendors and Augments

### [IP-13] Vendor Access and Currency Purchases

**Goal**: Verify all 4 augment/essence vendors spawn in Bazaar and accept their respective currencies.
**Steps**:

1. Enter Bazaar. Target and open each vendor: `Augment_Weaponsmith` (181200), `Essence_Provisioner` (181201), `Essence_Artificer` (181202), `Augment_Forgemaster`.
2. Buy a "Combat Stone I" from Weaponsmith — confirm platinum deducted, item received.
3. `#altcurrency add 100 5000`, buy "Stone of Might" from Provisioner — confirm 1000 CE deducted.
4. `#altcurrency add 101 5000`, buy "Shard of Flame" from Artificer — confirm 500 RE deducted.
5. Confirm Forgemaster sells Augment Forge container and Infusion Pool (for CE).

**Expected**: All 4 vendors present and interactable; platinum, CE, and RE purchases deduct the correct currency and deliver items.
**Status**: [x] Not Run  [ ] In Progress  [ ] Blocked  [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [IP-14] Aug Socket, Combat Proc, Stat Bonus, and Removal

**Goal**: Verify socketing, proc behavior, stat application, level gating, and removal with solvent.
**Steps**:

1. Get an Enchanted weapon and "Combat Stone V" (Weaponsmith). Socket it into the weapon. Equip and fight a mob — watch for proc message (~100 damage).
2. Get an Enchanted armor piece and "Stone of Might" (Provisioner). Socket it. Inspect tooltip for +heroic STR; check character sheet for STR increase.
3. Buy "Purified Solvent" (Provisioner, 100 CE). Use on the augmented armor — aug returned safely to inventory, item retains base stats, solvent consumed.
4. Attempt to socket "Combat Stone VII" (lv60 req) on a level 5 character — expect equip failure; try again at level 60 — should succeed.

**Expected**: Proc aug fires in combat, stat aug shows in tooltip and character sheet, solvent safely removes aug, level-gated aug blocked below requirement.
**Status**: [x] Not Run  [ ] In Progress  [ ] Blocked  [ ] Pass  [ ] Fail
**Notes**: ______________________________

---

## 7) Aug Merge and Infusion

### [IP-15] Aug Merge Happy Path and Rejection Cases

**Goal**: Verify the L1→L2 merge produces correct output and rejection cases protect items.
**Steps**:

1. Buy 3× "Stone of Might" (L1, 1000 CE each) and 1× "Lesser Merge Catalyst" (250 CE) from Forgemaster. Place all 4 in the Augment Forge, click Combine — expect "Stone of Might II" with +6 Heroic STR and +10 ATK.
2. Try merging 2× "Stone of Might" + 1× "Stone of Agility" + catalyst — expect "Augments must be the same type and level" error, items remain.
3. Try 3× L2 augs + Lesser Catalyst (wrong tier) — expect wrong-catalyst-tier error, items remain.
4. Try 3× L5 augs + any catalyst — expect "already at maximum level (5)" error.

**Expected**: Valid merge produces correct L2 output; all rejection cases return clear messages without consuming items.
**Status**: [x] Not Run  [ ] In Progress  [ ] Blocked  [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [IP-16] Augment Infusion and Transmutation

**Goal**: Verify the infusion flow and transmutation via Salvage Satchel work end-to-end.
**Steps**:

1. `#si 200530` (Infusion Pool), `#si 200300` (Stone of Might I), `#si 200520` (Infusion Catalyst I). Place aug + catalyst in Pool, Combine — expect +1 heroic STR (3→4), catalyst consumed, "infused to level 1" message.
2. Place a fresh Stone of Might I + Infusion Catalyst III in Pool — expect "Wrong catalyst tier. This augment needs Infusion Catalyst I" and both items remain.
3. Place a regular weapon + catalyst in Pool — expect non-aug rejection.
4. `#si 200302` (Stone of Might III, L3). Place in Salvage Satchel, Combine — expect +1000 CE, +25 RE, "[TRANSMUTED]" in the breakdown.
5. Place a magic sword + an L1 aug in Satchel together, Combine — expect sword salvaged (iLevel-based CE) and aug transmuted (125 CE) in one operation with combined totals.

**Expected**: Infusion applies +1 primary stat per catalyst, wrong-tier and non-aug rejections are clean, transmutation returns correct per-level Essence amounts, and mixed salvage+transmute totals combine correctly.
**Status**: [x] Not Run  [ ] In Progress  [ ] Blocked  [ ] Pass  [ ] Fail
**Notes**: ______________________________

---

## Soak Tests (Run Separately)

These require extended play sessions and cannot be fit into a standard smoke run. Run occasionally to confirm statistical distribution.

### [IP-S01] [SOAK] Tier Distribution on Drops

**Goal**: Verify drops follow the 80/15/4/1 distribution over a large sample.
**Steps**: Kill 100+ mobs, record tier of each drop, tally results.
**Expected**: ~80% Base, ~15% Enchanted, ~4% Legendary, ~1% Mythic (within 2σ).
**Status**: [x] Not Run  [ ] In Progress  [ ] Blocked  [ ] Pass  [ ] Fail
**Notes**: ______________________________

### [IP-S02] [SOAK] Zone Flavor Aug Drop Rate

**Goal**: Verify zone flavor augs drop at ~0.3% from zone mobs.
**Steps**: Kill 200+ mobs in Blackburrow (or any zone with a flavor aug), check for zone-specific aug in loot.
**Expected**: At least 1 drop in 200-300 kills for the target zone's aug (e.g., Gnoll Fang Chip, item 201000).
**Status**: [x] Not Run  [ ] In Progress  [ ] Blocked  [ ] Pass  [ ] Fail
**Notes**: ______________________________

---

## Agent Closeout Requirement

When an agent validates or changes infinite progression work, it must update this tracker:

1. Set one clear status per case.
2. Add concise notes using `Observed`, `Evidence`, `Commands`, and `Next`.
3. Mark blocked cases as `Blocked`, not `Fail`, unless game behavior itself failed.
4. Prefer updating existing scenario cases over adding new micro-cases.
