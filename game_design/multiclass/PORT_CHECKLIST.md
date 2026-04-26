# THJServer Multiclass Port Checklist

**Last Updated:** 2026-04-25
**Master Technical Document:** [IMPLEMENTATION_PLAN.md](IMPLEMENTATION_PLAN.md)
**Test Tracker:** [TEST_TRACKER.md](TEST_TRACKER.md)

**Inputs**
- Reference report: `tools/output/multiclass_references.csv` (line hits from THJServer)
- Upstream implementation: `extras/thjserver/`

**How to compare any file**
- `git diff --no-index <relpath> extras/thjserver/<relpath>`

---

## Port Priority Guide

### Phase 1: Core APIs (HIGH PRIORITY)
Files that define the multiclass API and class checking patterns.
- `zone/client.cpp` - GetClassesBits(), HasClass(), AddExtraClass()
- `zone/client.h` - Client class declarations
- `common/ruletypes.h` - Rule definitions

### Phase 2: Spell System (HIGH PRIORITY)
Files that control spell casting, memorization, and vendor filtering.
- `zone/spells.cpp` - CastSpell(), spell level checks
- `zone/client_packet.cpp` - OP_MemorizeSpell, spell merchant packets

### Phase 3: AA System (HIGH PRIORITY)
Files that control AA visibility, purchase, and activation.
- `zone/aa.cpp` - AA filtering, SendAlternateAdvancementTable()

### Phase 4: Skills & Stats (MEDIUM PRIORITY)
Files that control skill caps, training, and stat calculations.
- `zone/client_mods.cpp` - MaxSkill(), stat calculations
- `zone/bonuses.cpp` - Bonus application

### Phase 5: Combat & Effects (MEDIUM PRIORITY)
Files that control combat abilities and spell effects.
- `zone/spell_effects.cpp` - Spell effect handlers
- `zone/attack.cpp` - Combat calculations
- `zone/effects.cpp` - Discipline handling

### Phase 6: Items & Inventory (LOWER PRIORITY)
Files that control item class restrictions.
- `zone/inventory.cpp` - Item equip validation

---

## Snapshot (from `multiclass_references.csv`)

- Referenced files: `112`
- Parity with THJServer: `36` same, `66` differ, `10` THJ-only

## Recent Port Activity (2026-04-20)

- Verified the following multiclass behaviors are already present in current code and should be treated as validation-first, not broad porting work:
  - `zone/client.cpp`: class-add/remove refresh path recalculates mana, sends `SendManaUpdate()`, and resends EdgeStatLabel stats.
  - `extras/eq-core-dll-main/src/eqgame.cpp`: mana gauge and `Max_Mana` client detours already consume server-reported mana values for multiclass presentation.
  - `zone/client_process.cpp`: merchant class filtering already uses owned-class bitmasks.
  - `zone/client_packet.cpp` and `zone/spells.cpp`: class-locked item click/equip-cast checks already use `GetClassesBits()`.
- Added class-removal spell-gem cleanup:
  - `zone/client.cpp`: `RemoveExtraClass()` now interrupts invalid in-progress casts and clears memorized gems for spells no longer usable by the remaining owned classes.
  - `zone/spells.cpp` / `zone/client.h`: shared spell-eligibility helpers now drive both memorize-time validation and class-removal cleanup, including level entitlement checks, so the rules stay consistent.
- Removed the last base-class-only restriction from class mutation:
  - `zone/client.cpp`: `GetClassesBits()` / `SetClassesBits()` now treat persisted `GestaltClasses` as authoritative multiclass state instead of silently OR-ing the legacy base class back into the owned mask.
  - `zone/client.cpp` / `zone/gm_commands/removeclass.cpp`: starting/base class removal now follows the same rules as any other owned class, with the only remaining guard being that the character must retain at least one class.
- Cleaned up the remaining compatibility-class leakage:
  - `zone/client.cpp` / `common/repositories/character_data_repository.h`: the legacy `character_data.class` field is now synced to a deterministic owned compatibility class whenever the owned multiclass bitmask changes.
  - `zone/client_packet.cpp`: login hydration now reuses the shared persisted multiclass-bucket fallback logic and no longer seeds the runtime cache by OR-ing the legacy class bit back into `m_pp.classes`.
  - `world/worlddb.cpp`: char-select shaping now uses `GestaltClasses` as-is and picks the deterministic bitmask-derived compatibility class for the single-class compatibility slot instead of reintroducing or randomly selecting a removed starting class.
- Cleaned up the pre-testing review findings:
  - `zone/client_process.cpp`: memorize-time spell checks now reuse the shared multiclass spell helper instead of keeping a second inline implementation.
  - `extras/eq-core-dll-main/src/WaypointPOCWnd.cpp`: GM dashboard actions now execute direct client commands instead of routing through the waypoint `/say` bridge, and the tool-window commands now reliably show the requested window instead of toggling it closed.
  - `zone/bonuses.cpp`: passive AA bonuses are gated directly by `CanUseAlternateAdvancementRank(rank)` in the bonus path.
  - `zone/client.cpp` / `zone/exp.cpp` / `zone/guild_mgr.cpp`: guild member refreshes now publish owned-class bitmasks after class mutation and level-up, and they force a guild-members-list reload so roster projection stays aligned with current multiclass ownership.
- Practical implication: remaining multiclass item work should focus on augment/edge-case validation instead of assuming inventory support is largely unported.

## Recent Port Activity (2026-02-26)

- THJ waypoint + Bazaar quest compatibility foundation:
  - Added waypoint protocol structs/opcodes:
    - `common/eq_packet_structs.h`: `WaypointList_*` / `WaypointRequest_*`
    - `common/emu_oplist.h`: `OP_WaypointList`, `OP_WaypointRequest`
    - `utils/patches/patch_RoF2.conf`: opcode mappings for RoF2 (`0x1402`, `0x1403`)
  - Added zone handling for waypoint request packets:
    - `zone/client_packet.cpp`: `Handle_OP_WaypointRequest`, opcode registration
  - Added client waypoint API used by imported THJ quest scripts:
    - `zone/client.h` / `zone/client.cpp`:
      - `SendWaypointList`, `UnlockWaypoint`, `IsWaypointUnlocked`
      - `CheckWaypointGroupFeature`, `EnableWaypointGroupFeature`
      - group/autotransport state buckets and waypoint transport flow
  - Added missing script bindings:
    - `zone/perl_client.cpp`: `HasClassID`, waypoint methods
    - `zone/lua_client.h` / `zone/lua_client.cpp`: `HasClassID`, waypoint methods
  - Expanded THJ quest API compatibility used by imported Bazaar/seasonal/progression scripts:
    - `zone/client.h` / `zone/client.cpp`: `HasClass(string)`, `GrantPetNameChange(class_id)`
    - `zone/exp.cpp`: `IsSeasonal`, `GetKillCount`, `ConsumeUnspentAA`, `ConsumeItemOnCursor`
    - `zone/inventory.cpp`: `SummonFixedItem`, `ReturnItem`
    - `zone/perl_client.cpp` + `zone/lua_client.h/.cpp`: exported/bound the above APIs plus `CheckTitle`, `IsPetNameChangeAllowed`
  - Added missing seasonal rule expected by THJ-style scripts:
    - `common/ruletypes.h`: `Custom:EnableSeasonalCharacters`
  - Added DB bootstrap migration:
    - `utils/sql/custom/2026_02_26_thj_waypoints_bazaar_bootstrap.sql`
    - seeds `thj_waypoints*` base content, creates Bazaar THJ NPCs (Tearel, Vision_of_Ayonae, A_Fading_Memory, class trainers `20..35`, pet bag merchant), adds Bazaar map door `doorid=146`, and seeds per-zone `#TPTriggerN` waypoint discovery spawns from `thj_waypoints`.
- `zone/attack.cpp`:
  - Ported THJ-style proc slot handling (`MAX_PROCS` scan + `procCount` cap).
  - Ported THJ-style primary extra-attack roll behavior (per-hit roll instead of one-roll burst).
  - Ported THJ-style legacy crit chance path while preserving `Combat:UseNewDexFormulas`.
  - Ported THJ-style archery ordering (mastery applied after archery path).
  - Added THJ-style bow minimum clamp + heroic DEX scaling on legacy dex path.
  - Added THJ-style legacy berserker `DevastatingFrenzyDamageMultiplier`.
  - Ported `GetMeleeImpliedTarget` helper and wired it into player/player-pet melee round paths.
  - Ported THJ NPC/pet weapon-instance proc path in `NPC::Attack` (`weapon_instance` lookup + proc calls using instance pointer).
  - Preserved existing custom/new-formula branches to avoid regressions.
- `common/ruletypes.h`:
  - Added THJ-aligned custom combat rules used by the new attack parity paths:
    - `ScaleAutoAttackByHStr`, `ScaleAutoAttackHStrSoftCap`, `ScaleAutoAttackHStrScaleFloor`, `ScaleAutoAttackHStrScaleFactor`
    - `ScaleBowByHDex`, `ScaleBowByHDexDivide`, `ScaleBowHDexSoftCap`, `ScaleBowHDexScaleFloor`, `ScaleBowHDexScaleFactor`
    - `ScaleBowMinimumDamageMultiplier`, `ScaleBowMinimumDamageDivisor`
    - `DevastatingFrenzyDamageMultiplier`
  - Added `EnablePetBags` rule for THJ-style class pet bag equipment sync.
- `zone/client.h` / `zone/client.cpp`:
  - Ported THJ pet bag API and core behavior:
    - `IsPetBagActive`, `IsValidPetBagForClass`, `IsValidPetBag`
    - `GetActivePetBag`, `GetActivePetBagSlot`
    - `DoPetBagResync`, `DoPetBagFlush`
  - Added THJ class->bag ID mapping (including legacy fallback IDs).
- `zone/npc.h` / `zone/npc.cpp`:
  - Ported `NPC::GetPetOriginClass()` to route pet bag sync by originating class spell list.
- `zone/pets.cpp`:
  - Added pet bag resync hooks when pets are attached/set for a client (non-charm paths).
- `zone/client_packet.cpp`:
  - Added pet bag resync after zoned pet state restore.
  - Added pet bag resync for augmentation actions performed inside the active pet bag.
- `zone/inventory.cpp`:
  - Added pet bag flush when a pet bag is destroyed from cursor.
  - Added pet bag resync when moving/swapping an active pet bag.
- `zone/spell_effects.cpp`:
  - Added charm-path pet bag sync + initial charmed inventory serialization.
  - Added charmed inventory restoration on charm break (for pets modified by bag sync).
  - Added summon-pet path resync so newly summoned pets pick up bag equipment immediately.
- `utils/sql/custom/2026_02_26_thj_pet_bags.sql`:
  - Added DB migration that creates THJ pet bag item IDs and merchant entries for pet-capable classes.
- THJ-style custom instance foundation port:
  - `common/ruletypes.h`: added `StaticInstanceVersion`, `StaticInstanceTemplateVersion`, `FarmingInstanceVersion`, `FarmingInstanceTemplateVersion`.
  - `common/zone_store.cpp`: remap static/farming instance versions to template versions for zone lookups/fallback.
  - `zone/spawngroup.cpp`, `zone/trap.cpp`, `zone/zone.cpp`, `zone/zonedb.cpp`: remap static/farming versions to template versions for spawn groups, traps, zone points, and bulk NPC loads.
  - `zone/spawn2.cpp`: ported THJ-style spawn behavior:
    - static version disables practical respawn (`respawntime=604800000`, `variance=0`)
    - farming version suppresses long-respawn spawns (`respawntime >= 7200`) to prevent raid-style spawns.
  - `zone/lua_general.cpp`: added `eq.is_static_instance()` and `eq.is_farming_instance()` Lua helpers used by THJ quest scripts.
  - `quests/plugins/cata_instance_utils.pl`: non-respawning instance lockout changed to 24 hours (once per day); lifetime remains 7 days.
- Build status:
  - `zone` target compiles clean after changes.

## Recent Port Activity (2026-02-27)

- THJ-style `Bazaar and Back` AA foundation:
  - Added server-side AA handler in `zone/aa.cpp`:
    - first use (outside Bazaar): save return location to character bucket and teleport to Bazaar.
    - second use (inside Bazaar): return to saved zone/instance/coords; validates instance existence and re-adds character to instance when needed.
    - reuses AA cooldown path (60s from AA rank recast).
  - Added auto-grant hook for all characters on connect in `zone/client_packet.cpp` via `EnsureBazaarAndBackAA()`.
  - Added DB migration `utils/sql/custom/2026_02_27_bazaar_and_back_aa.sql`:
    - seeds spell, db_str text, `aa_ability`, and `aa_ranks` entries for `Bazaar and Back`.

## Work Order (recommended)
Project priority has shifted to source-level THJ gap discovery. Use this checklist as a triage input, not a porting mandate.

1. **Inventory and classify gaps**
   - Start with [THJ_GAP_REGISTER.md](../../THJ_GAP_REGISTER.md) and `THJ-GAP-00`.
2. **Core multiclass gap pass**
   - Run `THJ-GAP-01` over the high-priority multiclass files below.
3. **Mechanics/pet gap pass**
   - Run `THJ-GAP-02` over pet, familiar, combat, proc, and taunt-related diffs.
4. **Quest/script/API gap pass**
   - Run `THJ-GAP-03` over Bazaar, waypoint, Lua, Perl, quest, plugin, and SQL/data surfaces.
5. **Infrastructure gap pass**
   - Run `THJ-GAP-04` over DB manifests, schema, rules, opcodes, and tooling.
6. **Convert only meaningful gaps**
   - Each real gap should become one implementation, validation, or design-decision packet. Do not port a file just because it differs.

## Key Design Notes (Current Assumptions)
- `character_data.class` is a single owned compatibility class for stock packet/client paths; authoritative membership is persisted in `data_buckets` under `key='GestaltClasses'`.
- THJServer uses `Client::GetClassesBits()` (uint32) extensively; most checks become "union-of-classes".
- THJServer uses rules for enabling and for special handling:
  - `Custom:MulticlassingEnabled`
  - `Custom:UseDynamicAATimers`

---

## Checklist (by file)

Legend:
- `[x]` already identical to THJServer
- `[ ]` needs porting / review

### common
- [ ] `common/database/database_update_manifest.cpp` (refs: 38, parity: diff)
- [x] `common/repositories/base/base_data_buckets_repository.h` (refs: 14, parity: same)
- [ ] `common/database.cpp` (refs: 8, parity: diff)
- [ ] `common/ruletypes.h` (refs: 5, parity: partial - THJ combat custom rules ported 2026-02-26, broader rule parity still diff)
- [x] `common/repositories/data_buckets_repository.h` (refs: 4, parity: same)
- [x] `common/guild_base.cpp` (refs: 3, parity: implemented GestaltClasses guild roster projection 2026-02-26)
- [x] `common/shareddb.cpp` (refs: 3, parity: implemented multiclass spell/disc timer + target-type transforms 2026-02-26)
- [ ] `common/classes.h` (refs: 1, parity: diff)
- [x] `common/data_bucket.h` (refs: 1, parity: same)
- [x] `common/database_instances.cpp` (refs: 1, parity: same)
- [ ] `common/eq_packet_structs.h` (refs: 1, parity: diff)

### world
- [ ] `world/clientlist.cpp` (refs: 12, parity: diff)
- [ ] `world/worlddb.cpp` (refs: 11, parity: diff)
- [ ] `world/client.cpp` (refs: 9, parity: diff)

### zone (HIGH PRIORITY)
- [x] `zone/cli/tests/databuckets.cpp` (refs: 86, parity: same)
- [x] `zone/client.cpp` (refs: 70, parity: partial/ongoing)
- [x] `zone/client_packet.cpp` (refs: 61, parity: implemented OP_PlayerProfile/OP_MemorizeSpell)
- [ ] `zone/mob.cpp` (refs: 45, parity: partial - base Mob GetClassesBits/HasClass + caster/melee archetype checks ported 2026-02-26)
- [x] `zone/spell_effects.cpp` (refs: 39, parity: implemented Bard Pulse/Infinite Buffs)
- [ ] `zone/attack.cpp` (refs: 34, parity: partial - THJ proc/crit/archery/combat scaling parity pass ported 2026-02-26; smart-target helper + residual diffs remain)
- [ ] `zone/aa.cpp` (refs: 27, parity: partial - Mnemonic Retention + Fury of Magic multiclass gates ported 2026-02-26; passive AA ownership now enforced through the shared entitlement checks used by the bonus path)
- [x] `zone/spells.cpp` (refs: 27, parity: implemented dynamic AA timers)
- [ ] `zone/client_mods.cpp` (refs: 26, parity: partial - CalcBaseMana/CalcBaseEndurance now best-of-owned-classes 2026-02-26)
- [ ] `zone/client_process.cpp` (refs: 24, parity: partial - multiclass trainer open/end gating + trainer-class skill caps ported 2026-02-21)
- [ ] `zone/special_attacks.cpp` (refs: 23, parity: diff)
- [ ] `zone/effects.cpp` (refs: 17, parity: diff)
- [ ] `zone/bonuses.cpp` (refs: 16, parity: partial - passive AA ownership gating aligned with multiclass enablement 2026-04-20)
- [ ] `zone/entity.cpp` (refs: 16, parity: diff)
- [ ] `zone/client.h` (refs: 7, parity: diff)
- [ ] `zone/inventory.cpp` (refs: 4, parity: diff)
- [ ] `zone/mob.h` (refs: 4, parity: partial - bitmask-aware Mob API declarations ported 2026-02-26)

### zone (MEDIUM PRIORITY - Bots/Mercs)
- [ ] `zone/bot.cpp` (refs: 69, parity: diff)
- [ ] `zone/heal_rotation.cpp` (refs: 22, parity: diff)
- [ ] `zone/npc.cpp` (refs: 18, parity: diff)
- [x] `zone/bot_command.h` (refs: 12, parity: same)
- [ ] `zone/merc.cpp` (refs: 12, parity: diff)
- [ ] `zone/exp.cpp` (refs: 11, parity: diff)
- [x] `zone/bot_commands/item_use.cpp` (refs: 10, parity: same)
- [x] `zone/bot.h` (refs: 10, parity: same)
- [x] `zone/client_bot.cpp` (refs: 7, parity: same)
- [x] `zone/lua_bot.cpp` (refs: 7, parity: same)
- [x] `zone/bot_commands/bot.cpp` (refs: 6, parity: same)
- [ ] `zone/botspellsai.cpp` (refs: 3, parity: diff)
- [x] `zone/lua_bot.h` (refs: 3, parity: same)
- [ ] `zone/bot_database.cpp` (refs: 2, parity: diff)
- [x] `zone/merc.h` (refs: 2, parity: same)
- [x] `zone/bot_command.cpp` (refs: 1, parity: same)
- [x] `zone/bot_commands/apply_potion.cpp` (refs: 1, parity: same)
- [x] `zone/bot_commands/track.cpp` (refs: 1, parity: same)
- [x] `zone/bot_commands/depart.cpp` (refs: 2, parity: same)
- [x] `zone/bot_commands/pull.cpp` (refs: 2, parity: same)
- [x] `zone/bot_commands/view_combos.cpp` (refs: 2, parity: same)

### zone (LOWER PRIORITY - Scripting/Lua/Perl)
- [ ] `zone/lua_client.cpp` (refs: 9, parity: diff)
- [ ] `zone/lua_mob.cpp` (refs: 9, parity: diff)
- [ ] `zone/perl_mob.cpp` (refs: 8, parity: diff)
- [ ] `zone/embparser.cpp` (refs: 7, parity: diff)
- [x] `zone/gm_commands/merchantshop.cpp` (refs: 6, parity: same)
- [ ] `zone/lua_npc.cpp` (refs: 6, parity: diff)
- [ ] `zone/lua_zone.cpp` (refs: 6, parity: diff)
- [ ] `zone/perl_zone.cpp` (refs: 6, parity: diff)
- [ ] `zone/tradeskills.cpp` (refs: 6, parity: diff)
- [ ] `zone/zonedb.cpp` (refs: 6, parity: partial - preload GestaltClasses into PlayerProfile on zone load 2026-02-26)
- [ ] `zone/aggro.cpp` (refs: 4, parity: partial - bard aggro cap bypass while multiclass enabled ported 2026-02-26)
- [ ] `zone/lua_mob.h` (refs: 4, parity: diff)
- [ ] `zone/perl_client.cpp` (refs: 4, parity: diff)
- [x] `zone/gm_commands/show/quest_globals.cpp` (refs: 3, parity: same)
- [ ] `zone/guild_mgr.cpp` (refs: 3, parity: diff)
- [ ] `zone/lua_client.h` (refs: 3, parity: diff)
- [ ] `zone/lua_npc.h` (refs: 3, parity: diff)
- [ ] `zone/lua_zone.h` (refs: 3, parity: diff)
- [x] `zone/qglobals.cpp` (refs: 3, parity: same)
- [ ] `zone/questmgr.cpp` (refs: 3, parity: diff)
- [ ] `zone/tune.cpp` (refs: 3, parity: partial - ranger archery + berserker frenzy class checks switched to HasClass 2026-02-26)
- [ ] `zone/titles.cpp` (refs: 1, parity: partial - title eligibility uses HasClass for multiclass ownership 2026-02-26)
- [ ] `zone/corpse.cpp` (refs: 2, parity: diff)
- [x] `zone/gm_commands/gearup.cpp` (refs: 2, parity: same)
- [ ] `zone/mob_ai.cpp` (refs: 2, parity: diff)
- [ ] `zone/raids.cpp` (refs: 2, parity: diff)
- [ ] `zone/zone.cpp` (refs: 2, parity: diff)
- [ ] `zone/zone.h` (refs: 2, parity: diff)
- [ ] `zone/api_service.cpp` (refs: 1, parity: diff)
- [x] `zone/global_loot_manager.cpp` (refs: 1, parity: same)
- [x] `zone/gm_commands/databuckets.cpp` (refs: 1, parity: same)
- [ ] `zone/lua_item.cpp` (refs: 1, parity: diff)
- [x] `zone/lua_item.h` (refs: 1, parity: same)
- [x] `zone/lua_spell.cpp` (refs: 1, parity: same)
- [x] `zone/lua_spell.h` (refs: 1, parity: same)
- [x] `zone/mob_info.cpp` (refs: 1, parity: same)
- [x] `zone/npc_scale_manager.cpp` (refs: 1, parity: same)
- [x] `zone/perl_bot.cpp` (refs: 1, parity: same)
- [ ] `zone/perl_questitem_data.cpp` (refs: 1, parity: diff)
- [x] `zone/perl_spell.cpp` (refs: 1, parity: same)
- [x] `zone/qglobals.h` (refs: 1, parity: same)
- [x] `zone/cli/benchmark_databuckets.cpp` (refs: 2, parity: same)

### THJ-only files (evaluate for inclusion)
- [ ] `zone/thj_waypoints.h` (refs: 8, parity: THJ-only)
- [ ] `zone/thj_waypoints.cpp` (refs: 7, parity: THJ-only)
- [ ] `zone/gm_commands/toggleimprovedmodels.cpp` (refs: 2, parity: THJ-only)
- [ ] `common/database/database_schema.h` (refs: 2, parity: THJ-only)
