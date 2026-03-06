# Infinite Progression Code Review

Date: 2026-03-06  
Reviewer: Codex  
Scope: Full review of `game_design/infinite_progression/*` with implementation cross-check against active code/SQL/tools and method comparison to `extras/THJServer`.

## Executive Summary

Core implementation is broadly solid and now correctly uses item-instance XP (`custom_data("Exp")`) in a THJ-like way.  
The highest-risk issue is an item ID collision (`200520`) between **Purified Solvent** and **Infusion Catalyst I**, which can break safe augment removal.  
There is also behavior/test/doc drift that will create false failures and operational confusion unless corrected.

## Findings (Ordered by Severity)

### F1 - Critical: SQL/item ID collision between Solvent and Infusion Catalyst I

- `utils/sql/item_progression/step08_09_augments_vendors_merge.sql:378` defines `200520` as `Purified Solvent`.
- `utils/sql/item_progression/step11_infusion_pool_catalysts.sql:20` reuses `200520` as `Infusion Catalyst I` (`REPLACE INTO`).
- `tools/generate_augments.py:41` still reserves `SOLVENT_ID = 200520`.
- `zone/augment_infusion.cpp:35` hardcodes catalyst I as `200520`.
- `zone/tradeskills.cpp:189-201` expects a solvent/distiller item type for safe aug removal.

Impact:
- Running Step 11 SQL overwrites solvent row semantics and can break AV-06 style safe aug removal flows.
- This is a data contract break across Steps 8/9/11, not just a doc issue.

### F2 - High: Quest auto-tier logic is globally scoped in `SummonItem`, not quest-scoped

- `zone/inventory.cpp:197-213` applies `QuestItemDefaultTier` inside `Client::SummonItem` for all non-tiered item summons.
- `common/ruletypes.h:1247` describes this as quest-granted behavior, but callsite is global.

Impact:
- Any code path calling `SummonItem` can be affected (GM/admin tools, scripts, spell/item summon pathways, tradeskill reward paths), not just quest rewards.
- This can cause unintentional tiering and hard-to-debug side effects.

### F3 - High: `IMPLEMENTATION_STEPS.md` has material status/architecture drift

- Version mismatch: header says v1.1, footer says v1.0  
  (`IMPLEMENTATION_STEPS.md:3`, `IMPLEMENTATION_STEPS.md:1260`)
- Step 10 marked complete while deferred checklist items remain unchecked  
  (`IMPLEMENTATION_STEPS.md:33`, `IMPLEMENTATION_STEPS.md:1029-1033`)
- Step 3 still documents bucket-based XP despite implementation using item-instance custom_data  
  (`IMPLEMENTATION_STEPS.md:246`, `:285-307` vs `zone/power_slot_xp.cpp:27`, `:82`, `:227`)
- Quick reference references stale/nonexistent paths and old command layout  
  (`IMPLEMENTATION_STEPS.md:1249-1252`)
- Step 11 notes reference wrong SQL path  
  (`IMPLEMENTATION_STEPS.md:1126`)

Impact:
- Testers and implementers are working from mixed models (bucket vs item-instance, old file layout), increasing false failures and onboarding friction.

### F4 - High: Test tracker has behavior mismatches against implemented code

- DT-04 expects zone-wide broadcast  
  (`TEST_TRACKER.md:861-871`)
  but implementation sends to killer + group/raid only  
  (`zone/attack.cpp:3114-3157`).
- DT-05 expects quest rewards to use drop-tier RNG  
  (`TEST_TRACKER.md:874-883`)
  but current behavior is explicit tier assignment (`grant_tiered_item`) and/or global default tier via `SummonItem`  
  (`zone/questmgr.cpp:200-223`, `zone/inventory.cpp:197-213`).

Impact:
- Current tests can fail while code is behaving as implemented, reducing trust in the tracker.

### F5 - Medium: Ghost projection rule disable can leave stale ghost in place

- `zone/ghost_copy.cpp:137-139` returns immediately when `StatProjectionEnabled` is false.
- Existing ghost cleanup is not performed before returning.
- GC-13 expectation implies disabled system should leave no active ghost (`TEST_TRACKER.md:792-802`).

Impact:
- Rule toggle may not produce immediate in-world behavior expected by testers/players.

### F6 - Medium: Tier ID constraint is documented stricter than generator enforcement

- Runtime helper contract says base IDs must be `< 250,000` for decode math correctness  
  (`common/item_tier.h:32`, `:48`, `:53`).
- Generator only skips IDs above link-mask bound `298,575`  
  (`tools/generate_tiered_items.py:52`, `:557-560`).

Impact:
- If base IDs in `[250,000..298,575]` are ever processed, modulo/division decode assumptions can break.

### F7 - Medium/Low: Some tests are high-effort/low-signal for a time-constrained project

- Probabilistic long-run tests:
  - `DT-01` asks for 100+ kills and 2σ distribution checks (`TEST_TRACKER.md:825-833`)
  - `ZD-01` requires very low-rate natural drop confirmation (`TEST_TRACKER.md:1227-1237`)
- These are valid as deep validation but poor as primary smoke gates under schedule pressure.

Impact:
- Time spent on noisy statistical checks instead of deterministic regression checks.

## THJ Method Comparison

### What matches THJ (good)

- THJ uses item-instance progression state via `custom_data("Exp")`:  
  `extras/THJServer/zone/exp.cpp:543`, `:569`, `:604`
- Current implementation now does the same:  
  `zone/power_slot_xp.cpp:9-11`, `:27`, `:82`, `:227`

### What intentionally differs (and why)

- THJ tier/family math uses 1,000,000 bands:  
  `extras/THJServer/common/item_instance.cpp:887`, `:896`, `:900`
- Current implementation uses `250,000` offset designed to stay within item-link mask for chat-link reliability:  
  `common/item_tier.h:29-36`

Assessment:
- This divergence is justified for your stated requirement that chat links work reliably.
- Net: THJ-like in per-instance progression behavior, better fit for link constraints.

## Targeted Test Tracker Recommendations (Non-Fluff Focus)

1. Keep deterministic validation first:
   - DB presence/range checks, container wiring checks, rule toggle checks, and explicit command-path checks.
2. Reclassify heavy RNG tests (`DT-01`, natural low-rate `ZD-01`) as periodic soak tests, not smoke gates.
3. Update DT-04 and DT-05 to match actual current behavior or change code to match intended behavior.
4. Add one high-value regression test for `QuestItemDefaultTier` scoping:
   - verify non-quest `SummonItem` pathways are not unintentionally tiering (or explicitly document that they are).

