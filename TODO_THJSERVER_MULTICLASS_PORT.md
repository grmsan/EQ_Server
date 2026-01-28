# THJServer Multiclass Port Checklist

This is the file-by-file port plan to implement **all multiclass (gestalt) behaviors** found in `extras/THJServer/` into this repo’s codebase.

**Master Technical Document:** [game_design/multiclass/IMPLEMENTATION_PLAN.md](game_design/multiclass/IMPLEMENTATION_PLAN.md)

**Inputs**
- Reference report: `tools/output/multiclass_references.csv` (line hits from THJServer)
- Upstream implementation: `extras/THJServer/`

**Testing tracker**
- `TODO_THJSERVER_MULTICLASS_TESTS.md`

**How to compare any file**
- `git diff --no-index <relpath> extras/THJServer/<relpath>`

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

## Work Order (recommended)
1. **Persistence + load path**
   - Ensure `GestaltClasses` is written on character creation and on class add/remove, and is loaded consistently by zone/world.
2. **Core APIs (server authoritative “what classes do I have?”)**
   - Align on one canonical “classes bitmask” getter/setter used everywhere (THJ’s `GetClassesBits()` + `HasClass()` patterns).
3. **Spells / casting / stacking**
   - Port THJ’s multiclass logic in `zone/spells.cpp` and related spell/cast codepaths.
4. **AA visibility + timers**
   - Port THJ’s AA filtering logic and `UseDynamicAATimers` behaviors.
5. **Items + “show usable” filtering**
   - Port THJ’s equip/use gating to union-of-classes (vendor “usable” filters, item class masks).
6. **Skills / caps / UI exposure**
   - Port THJ’s skill limit logic (server-side caps) and ensure add/remove class flows expose skills.
7. **Bots, misc subsystems, and docs**
   - Sweep remaining diffs; port THJ-only files (waypoints, docs) as needed.

## Key Design Notes (THJServer assumptions)
- Primary class remains in `character_data.class`, but *gestalt membership* is persisted in `data_buckets` under `key='GestaltClasses'`.
- THJServer uses `Client::GetClassesBits()` (uint32) extensively; most checks become “union-of-classes”.
- THJServer uses rules for enabling and for special handling:
  - `Custom:MulticlassingEnabled`
  - `Custom:UseDynamicAATimers`
  - `Custom:BypassMulticlassStackConflict`

## Checklist (by file)

Legend:
- `[x]` already identical to THJServer
- `[ ]` needs porting / review

### CHANGELOG.md
- [ ] `CHANGELOG.md` (refs: 2, parity: diff)

### client_files
- [ ] `client_files/export/main.cpp` (refs: 13, parity: diff)

### common
- [ ] `common/database/database_update_manifest.cpp` (refs: 38, parity: diff)
- [x] `common/repositories/base/base_data_buckets_repository.h` (refs: 14, parity: same)
- [ ] `common/database.cpp` (refs: 8, parity: diff)
- [ ] `common/ruletypes.h` (refs: 5, parity: diff)
- [x] `common/repositories/data_buckets_repository.h` (refs: 4, parity: same)
- [ ] `common/guild_base.cpp` (refs: 3, parity: diff)
- [ ] `common/shareddb.cpp` (refs: 3, parity: diff)
- [ ] `common/CMakeLists.txt` (refs: 2, parity: diff)
- [ ] `common/database_schema.h` (refs: 2, parity: diff)
- [ ] `common/database/database_schema.h` (refs: 2, parity: THJ-only)
- [ ] `common/classes.h` (refs: 1, parity: diff)
- [x] `common/data_bucket.h` (refs: 1, parity: same)
- [x] `common/database_instances.cpp` (refs: 1, parity: same)
- [ ] `common/eq_packet_structs.h` (refs: 1, parity: diff)
- [ ] `common/repositories/db_str_repository.h` (refs: 1, parity: diff)
- [ ] `common/repositories/skill_caps_repository.h` (refs: 1, parity: diff)
- [ ] `common/repositories/spells_new_repository.h` (refs: 1, parity: diff)

### submodules
- [x] `submodules/recastnavigation/Tests/catch.hpp` (refs: 2, parity: same)

### utils
- [x] `utils/mods/legacy_combat.lua` (refs: 7, parity: same)

### world
- [ ] `world/clientlist.cpp` (refs: 12, parity: diff)
- [ ] `world/worlddb.cpp` (refs: 11, parity: diff)
- [ ] `world/client.cpp` (refs: 9, parity: diff)

### zone
- [x] `zone/cli/tests/databuckets.cpp` (refs: 86, parity: same)
- [ ] `zone/client.cpp` (refs: 70, parity: diff)
- [ ] `zone/bot.cpp` (refs: 69, parity: diff)
- [ ] `zone/client_packet.cpp` (refs: 61, parity: diff)
- [ ] `zone/mob.cpp` (refs: 45, parity: diff)
- [ ] `zone/spell_effects.cpp` (refs: 39, parity: diff)
- [ ] `zone/attack.cpp` (refs: 34, parity: diff)
- [ ] `zone/aa.cpp` (refs: 27, parity: diff)
- [ ] `zone/spells.cpp` (refs: 27, parity: diff)
- [ ] `zone/client_mods.cpp` (refs: 26, parity: diff)
- [ ] `zone/client_process.cpp` (refs: 24, parity: diff)
- [ ] `zone/special_attacks.cpp` (refs: 23, parity: diff)
- [ ] `zone/heal_rotation.cpp` (refs: 22, parity: diff)
- [ ] `zone/npc.cpp` (refs: 18, parity: diff)
- [ ] `zone/effects.cpp` (refs: 17, parity: diff)
- [ ] `zone/bonuses.cpp` (refs: 16, parity: diff)
- [ ] `zone/entity.cpp` (refs: 16, parity: diff)
- [x] `zone/bot_command.h` (refs: 12, parity: same)
- [ ] `zone/merc.cpp` (refs: 12, parity: diff)
- [ ] `zone/exp.cpp` (refs: 11, parity: diff)
- [x] `zone/bot_commands/item_use.cpp` (refs: 10, parity: same)
- [x] `zone/bot.h` (refs: 10, parity: same)
- [ ] `zone/lua_client.cpp` (refs: 9, parity: diff)
- [ ] `zone/lua_mob.cpp` (refs: 9, parity: diff)
- [ ] `zone/perl_mob.cpp` (refs: 8, parity: diff)
- [ ] `zone/thj_waypoints.h` (refs: 8, parity: THJ-only)
- [x] `zone/client_bot.cpp` (refs: 7, parity: same)
- [ ] `zone/client.h` (refs: 7, parity: diff)
- [ ] `zone/embparser.cpp` (refs: 7, parity: diff)
- [x] `zone/lua_bot.cpp` (refs: 7, parity: same)
- [ ] `zone/thj_waypoints.cpp` (refs: 7, parity: THJ-only)
- [x] `zone/bot_commands/bot.cpp` (refs: 6, parity: same)
- [x] `zone/gm_commands/merchantshop.cpp` (refs: 6, parity: same)
- [ ] `zone/lua_npc.cpp` (refs: 6, parity: diff)
- [ ] `zone/lua_zone.cpp` (refs: 6, parity: diff)
- [ ] `zone/perl_zone.cpp` (refs: 6, parity: diff)
- [ ] `zone/tradeskills.cpp` (refs: 6, parity: diff)
- [ ] `zone/zonedb.cpp` (refs: 6, parity: diff)
- [ ] `zone/aggro.cpp` (refs: 4, parity: diff)
- [ ] `zone/inventory.cpp` (refs: 4, parity: diff)
- [ ] `zone/lua_mob.h` (refs: 4, parity: diff)
- [ ] `zone/mob.h` (refs: 4, parity: diff)
- [ ] `zone/perl_client.cpp` (refs: 4, parity: diff)
- [ ] `zone/botspellsai.cpp` (refs: 3, parity: diff)
- [x] `zone/gm_commands/show/quest_globals.cpp` (refs: 3, parity: same)
- [ ] `zone/guild_mgr.cpp` (refs: 3, parity: diff)
- [x] `zone/lua_bot.h` (refs: 3, parity: same)
- [ ] `zone/lua_client.h` (refs: 3, parity: diff)
- [ ] `zone/lua_npc.h` (refs: 3, parity: diff)
- [ ] `zone/lua_zone.h` (refs: 3, parity: diff)
- [x] `zone/qglobals.cpp` (refs: 3, parity: same)
- [ ] `zone/questmgr.cpp` (refs: 3, parity: diff)
- [ ] `zone/tune.cpp` (refs: 3, parity: diff)
- [x] `zone/bot_commands/depart.cpp` (refs: 2, parity: same)
- [x] `zone/bot_commands/pull.cpp` (refs: 2, parity: same)
- [x] `zone/bot_commands/view_combos.cpp` (refs: 2, parity: same)
- [ ] `zone/bot_database.cpp` (refs: 2, parity: diff)
- [x] `zone/cli/benchmark_databuckets.cpp` (refs: 2, parity: same)
- [ ] `zone/corpse.cpp` (refs: 2, parity: diff)
- [x] `zone/gm_commands/gearup.cpp` (refs: 2, parity: same)
- [ ] `zone/gm_commands/toggleimprovedmodels.cpp` (refs: 2, parity: THJ-only)
- [x] `zone/merc.h` (refs: 2, parity: same)
- [ ] `zone/mob_ai.cpp` (refs: 2, parity: diff)
- [ ] `zone/raids.cpp` (refs: 2, parity: diff)
- [ ] `zone/zone.cpp` (refs: 2, parity: diff)
- [ ] `zone/zone.h` (refs: 2, parity: diff)
- [ ] `zone/api_service.cpp` (refs: 1, parity: diff)
- [x] `zone/bot_command.cpp` (refs: 1, parity: same)
- [x] `zone/bot_commands/apply_potion.cpp` (refs: 1, parity: same)
- [x] `zone/bot_commands/track.cpp` (refs: 1, parity: same)
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
