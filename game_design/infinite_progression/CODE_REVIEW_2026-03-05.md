# Infinite Progression Code Review

Date: 2026-03-05  
Reviewer: Codex  
Scope: Step 1-3 implementation status, test tracker quality, and method comparison vs `extras/THJServer`

## Review Constraints from Project Owner

1. Chat links must work.
2. Progression behavior should be THJ-like (item-instance based progression, not shared by base item family).
3. Keep future-step tests in tracker if they remain useful.

## Executive Summary

Core Step 1-3 code is present and generally structured well. The largest issues are documentation/test drift and one major behavioral divergence from THJ-style progression: Power Slot XP is currently tracked by base item ID, not by item instance. There is also a latent risk in the current tier ID scheme if the base item pool grows.

## Findings

### F1 - High: Implementation docs are inconsistent with actual status and behavior

- `game_design/infinite_progression/IMPLEMENTATION_STEPS.md:5` still says `Status: Plan — Not Yet Started` while steps 1-3 are marked complete at `:23-25`.
- Step 2 notes claim `+1,000,000/+2,000,000/+3,000,000` offsets:
  - `game_design/infinite_progression/IMPLEMENTATION_STEPS.md:192`
  - `game_design/infinite_progression/IMPLEMENTATION_STEPS.md:193`
  - `game_design/infinite_progression/IMPLEMENTATION_STEPS.md:205`
- Actual implementation uses `TIER_ID_OFFSET = 250,000`:
  - `common/item_tier.h:31`
  - `common/item_tier.h:42`
  - `common/item_tier.h:47`

Impact: Team members and testers will execute against stale assumptions and report false failures.

### F2 - High: Test tracker includes command surface that is not implemented yet

- Tracker lists commands not currently registered (`#salvage`, `#essence`, `#tierup`, `#feed`, `#override`, `#regenerate items`):
  - `game_design/infinite_progression/TEST_TRACKER.md:62-70`
- Only these progression commands are currently registered:
  - `zone/command.cpp:177` (`#ilevel`)
  - `zone/command.cpp:178` (`#itemtier`)
  - `zone/command.cpp:179` (`#powerslot`)
- No `command_salvage/essence/tierup/feed/override/regenerate` symbols found in zone command files.

Impact: current smoke/regression instructions are not executable as written for Steps 4+.

### F3 - High: Test expectations for tiered item IDs are incorrect for current implementation

- Tracker expects IDs using +1,000,000 offsets:
  - `game_design/infinite_progression/TEST_TRACKER.md:191` (`1028854`)
  - `game_design/infinite_progression/TEST_TRACKER.md:203` (`2028854`)
  - `game_design/infinite_progression/TEST_TRACKER.md:215` (`3028854`)
- Current code and generator use +250,000/+500,000/+750,000:
  - `common/item_tier.h:31`
  - `tools/generate_tiered_items.py:46`

Impact: Step 2 tests will fail even when implementation is correct.

### F4 - Medium: Tier ID scheme is currently functional but has future collision/headroom risk

- Tier math is based on fixed 250k bands:
  - `common/item_tier.h:31-54`
  - `tools/generate_tiered_items.py:46-50`
  - `tools/generate_tiered_items.py:409-414`
- This is compatible with your current DB shape, but only while base IDs stay in the supported range.
- DB snapshot at review time (local query):  
  `max_id=950001`, `base_lt_250k=117949`, `ench/leg/myth=117949 each`.

Impact: If future base content grows outside intended range, tier decode (`% TIER_ID_OFFSET`, `/ TIER_ID_OFFSET`) can break assumptions.

### F5 - Medium: Power Slot XP is base-item-family scoped, not THJ-like per-item-instance

- XP key is `power_xp_{base_item_id}`:
  - `zone/power_slot_xp.cpp:24`
  - `zone/power_slot_xp.cpp:171`
- Tier/base extraction drives the same-family behavior:
  - `zone/power_slot_xp.cpp:145-146`

Impact: Two different physical instances of the same base item share progression, which does not match requested THJ-like instance progression.

### F6 - Medium: Existing consume/feed helper still hardcodes 1,000,000-family matching

- Local server logic:
  - `zone/exp.cpp:1409`
  - `zone/exp.cpp:1410`
- This logic assumes THJ-style million-based families and conflicts with current 250k tier-family mapping.

Impact: Future Step 6 feed behavior may be incorrect unless this path is refactored before wiring.

### F7 - Low/Medium: Group/raid Power Slot XP distribution may over-credit edge cases

- Award path iterates full group/raid member arrays:
  - `zone/attack.cpp:3025`
  - `zone/attack.cpp:3031`

Impact: Depending on intended eligibility rules (range, participation, alive state), this can award XP to members that did not meaningfully participate.

### F8 - Low: Some tests are low-signal or pre-marked without evidence

- Pre-marked pass with blank notes:
  - `game_design/infinite_progression/TEST_TRACKER.md:118`
  - `game_design/infinite_progression/TEST_TRACKER.md:133`
- `IL-03` expected criteria is vague:
  - `game_design/infinite_progression/TEST_TRACKER.md:145`
- `CalculateILevel()` always returns at least 1:
  - `common/item_ilevel.cpp:99`

Impact: reduced confidence in regression signal; harder to separate real regressions from test ambiguity.

## THJ Method Comparison (Relevant to Requested Direction)

The closest THJ reference behavior is in `extras/THJServer`:

- Item tier/family style and upgrade chain via million offsets:
  - `extras/THJServer/common/item_instance.cpp:887`
  - `extras/THJServer/common/item_instance.cpp:900`
- Instance progression state stored on item custom data (`Exp`) and advanced via server XP flow:
  - `extras/THJServer/zone/exp.cpp:583`
  - `extras/THJServer/zone/exp.cpp:604`
  - `extras/THJServer/zone/exp.cpp:784`
  - `extras/THJServer/zone/exp.cpp:958`

Current implementation diverges in one major way:
- Your Power Slot XP is keyed per base item family (`power_xp_{base_id}`), not per item instance.

## Recommendations (Aligned to Owner Constraints)

1. Update docs and tracker first (low effort, immediate clarity).
2. Convert progression state from base-item bucket keys to item-instance state (THJ-like), preferably in item `custom_data`.
3. Keep chat-link compatibility by retaining link-safe tier IDs; add explicit guardrails/documented constraints for valid base-ID ranges.
4. Before implementing Step 6, replace/feed-refactor any remaining `% 1000000` matching logic in local `zone/exp.cpp`.
5. Tighten tracker pass criteria: remove pre-marked passes and require evidence fields for all executed cases.

## Suggested Priority

- P0: F1, F2, F3, F5
- P1: F6, F4
- P2: F7, F8

