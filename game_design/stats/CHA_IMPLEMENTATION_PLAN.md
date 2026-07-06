# Charisma (CHA) Implementation Plan

> Design sources: `game_design/stats/CHA.md`, `game_design/stats/OVERVIEW.md`, `game_design/stats/PET_SCALING.md`.
> Intent: Charisma is the Stat of Influence (loot luck, pet empowerment, charm reliability, soft mitigation, resist penetration). Deliver it with clear knobs and caps to prevent runaway scaling.
> Current architecture: keep legacy CHA behavior unless `Combat:UseNewCharismaSystems` is enabled. Tuning should be hotfixable through `zone/combat_balance.ini`; `zone/combat_balance_config.h` should hold fallback defaults only.

## Vision
- Make CHA the "off-axis" stat that amplifies support, farming, pet power, and crowd control while giving melee a light crit bump.
- Keep stacking with DEX/INT/WIS meaningful but cap combined penetration/mitigation so bosses are not trivialized.
- Gate CHA systems with `Combat:UseNewCharismaSystems` and expose tuning through runtime combat-balance config.

## Core Mechanics & Formulas (with guardrails)
- **Rare Loot Bonus**  
  `rare_bonus = min(CHA / CHA_RARE_DIVISOR, CHA_RARE_MAX_BONUS)`; applied as a multiplicative bonus when rolling rare tables.  
  *Defaults:* divisor 20 (matches doc), max bonus 0.50 (50%) to keep ultra-farm builds from guaranteeing drops.

- **Pet Stat Scaling**  
  CHA pet scaling should use the shared pet model in `PET_SCALING.md`. CHA provides the primary pet-power multiplier/class amplifier, while owner STR/STA/DEX/AGI/INT/WIS transfer at smaller configured rates and pet gear remains a separate weighted channel. Apply final pet softcaps/hardcaps after owner transfer, CHA multiplier, class scalar, and pet gear are combined.

- **Enemy Damage Reduction ("Disarming Beauty")**  
  `incoming_mult = 1 - min((CHA * Level) / CHA_DMG_REDUCTION_DIVISOR, CHA_DMG_REDUCTION_CAP)`; applied multiplicatively after armor/STA reductions.  
  *Defaults:* divisor 2500 (doc), cap 0.35 to avoid stacking to near-immune with STA/WIS.

- **Resist Penetration ("Beguiling Magic")**  
  `resist_pen = min(CHA / CHA_RESIST_DIVISOR, CHA_RESIST_CAP)`. Combine with DEX penetration under a shared cap: `min(resist_pen + dex_pen, CHA_DEX_RESIST_CAP)`.  
  *Defaults:* divisor 10, CHA cap 120, shared cap 180 so DEX+CHA cannot delete all resists on raid mobs.

- **Opportunity Crits**  
  `crit_bonus = min(CHA / CHA_CRIT_DIVISOR, CHA_CRIT_CAP)`; added after base crit chance and before DEX overflow.  
  *Defaults:* divisor 100 (doc), cap 15%.

- **Charm/CC Reliability (Enchanter/Bard focus)**  
  `charm_break_mod = max(0, base_break - (CHA / CHA_CHARM_DIVISOR))`; add a hard floor so charms never become literally permanent unless toggle is enabled. Optional permanent charm threshold `CHA_CHARM_PERMA_THRESHOLD` (doc suggests 500) gated by a rule `EnableChaPermanentCharm`.

- **Bard Song Power**  
  `song_mod = 1 + CHA / CHA_BARD_SONG_DIVISOR` (heals/damage/slows), capped by `CHA_BARD_SONG_CAP`.  
  *Defaults:* divisor 150, cap 1.5x to avoid dwarfing instrument mods.

## Runtime/Fallback Knobs
- Toggle: `Combat:UseNewCharismaSystems` with legacy-safe default unless intentionally enabled for a test shard.
- Loot: `CHA_RARE_DIVISOR = 20.0f`, `CHA_RARE_MAX_BONUS = 0.50f`.
- Pets: use shared pet-scaling keys from `PET_SCALING.md`, especially `[Curve.PET_CHA_POWER]`, `PET_CHA_MULTIPLIER_*`, `PET_CLASS_SCALAR_*`, `PET_GEAR_STAT_WEIGHT`, and `PET_OUTPUT_*`. Avoid isolated `CHA_PET_*` keys unless they are aliases mapped into the shared pet model.
- Damage reduction: `CHA_DMG_REDUCTION_DIVISOR = 2500.0f`, `CHA_DMG_REDUCTION_CAP = 0.35f`.
- Resist pen: `CHA_RESIST_DIVISOR = 10.0f`, `CHA_RESIST_CAP = 120.0f`, `CHA_DEX_RESIST_CAP = 180.0f`.
- Crits: `CHA_CRIT_DIVISOR = 100.0f`, `CHA_CRIT_CAP = 0.15f`.
- Charm: `CHA_CHARM_DIVISOR = 50.0f`, `CHA_CHARM_MIN_BREAK = 0.02f`, `CHA_CHARM_PERMA_THRESHOLD = 500`, `ENABLE_CHA_PERMA_CHARM` (default false).
- Bard: `CHA_BARD_SONG_DIVISOR = 150.0f`, `CHA_BARD_SONG_CAP = 1.5f`.

## Implementation Steps
1) **Config/Rules**  
   - Add fallback constants, runtime keys, and RuleB toggle `Combat:UseNewCharismaSystems`.
   - Wire shared cap for resist penetration with DEX runtime/fallback keys.

2) **Rare Loot**  
   - In loot generation (`zone/loot.cpp` / `NPC::AddLootDrop` path), multiply rare table roll odds by `1 + rare_bonus` when toggle is on.  
   - Add debug log/metrics hook to track average rare rolls per CHA bucket for tuning.

3) **Pet Empowerment**  
   - In pet stat build (`NPC::CalcBonuses` or pet construction), call the shared pet-scaling helper from `PET_SCALING.md`.
   - Include owner stat transfer, pet gear weight, CHA pet-power curve, class/archetype multipliers, and final caps in one combined calculation.
   - Allow pets without owners to skip owner/CHA scaling.
   - For Beastlords, route proc/crit inheritance through the shared pet model instead of adding a separate CHA-only path.

4) **Incoming Damage Reduction**  
   - In melee/spell damage intake (`Mob::CheckIncreaseIncomingDmg`, `Mob::ResistSpell`, or the shared mitigation path), apply `incoming_mult` after STA/AC/WIS reductions.  
   - Ensure hate generated uses post-reduction damage so high CHA does not trivialize aggro.

5) **Resist Penetration**  
   - In spell resist checks (`ResistSpell` / `Mob::GetResistMod`), subtract `resist_pen` from target resists, then apply shared cap with DEX.  
   - Flag NPCs that should ignore penetration (raid bosses) with a rule or immunity flag.

6) **Opportunity Crits**  
   - In melee/spell crit chance aggregation, add `crit_bonus` before clamping/overflow; overflow still feeds DEX-style crit damage.  
   - Apply to pets via pet crit scalar if pets inherit CHA.

7) **Charm/Bard Features**  
   - Adjust charm break chance (charm timers in `Mob::AI_Process` / `Client::CheckCharm`) using CHA divisor and floor.  
   - Gate permanent charm and stat sharing (`StatTransfer% = CHA / 20`) behind rules; apply stat sharing in charmed NPC bonus calc with softcap to avoid unkillable pets.  
   - Bard song potency/resist checks: add CHA modifier to song power and CC resist rolls within configured cap.

8) **Surfacing/Debugging**  
   - Add server-side debug prints for CHA-derived values (rare bonus, pet scalar, dmg reduction, resist pen) behind a GM command or debug rule for tuning sessions.  
   - If client UI supports it, surface CHA contributions in stat window; otherwise, add `/charisma` debug command.

## Safety & Fun Guardrails
- Cap rare loot at 50% bonus to preserve hunt loops; revisit after playtests if grind feels too long/short.
- Pet softcap/final cap comes from the shared pet model and must account for owner stats, CHA, pet gear, and class scalar together.
- Damage reduction applies multiplicatively and is capped, so STA/WIS/CHA stacking cannot reach near-zero damage.
- Shared resist penetration cap (CHA+DEX) prevents 200+ penetration from deleting raid resists; set NPC flag for penetration immunity on specific encounters.
- Charm permanence is gated by a rule and threshold; default off to avoid trivializing content.
- Bard/Enchanter scaling capped so instrument mods and spell ranks still matter.

## Testing Matrix
- **Loot:** 0/200/500/1000 CHA kills on a boss with known rare rate; log effective rare % vs target.
- **Pets:** Use `PET_SCALING.md` validation: compare pet HP/DPS at 0/300/600/1000 CHA while also testing owner stat transfer and pet gear weight independently; confirm final softcap/hardcap behavior.
- **Mitigation:** Measure incoming DPS on a tank at 70 with 0 vs 1000 CHA while stacking STA/WIS to ensure total reduction stays within expected band (~30-50%).  
- **Resists:** Land-rate tests on high-resist NPCs with DEX-only vs CHA-only vs both; confirm shared cap.  
- **Charm:** Duration/break-rate at 200/500/800 CHA; ensure floor and optional perma toggle function.  
- **Crits:** DPS parsing with CHA-only crit bonus to ensure it is additive and not overshadowing DEX overflow.

## Deliverables
- Runtime keys + fallback constants + rule toggle in ruletypes.
- Implementation hooks in loot, pet calc, mitigation, resist checks, crit aggregation, charm/song logic.
- Debug/telemetry for tuning and a short doc update in `CHA.md` once values are finalized.
