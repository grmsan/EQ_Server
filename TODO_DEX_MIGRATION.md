# TODO: DEX Migration Follow-Ups

As part of the new DEX precision system (`Combat:UseNewDexFormulas`), some legacy behaviors are currently bypassed. Track and re-enable as we iterate on class balance/focus.

## Legacy code currently skipped/bypassed
- **zone/attack.cpp::TryCriticalHit**: When `UseNewDexFormulas` is true, the new DEX crit path returns after applying the new crit logic. The legacy flow (rolled dex bonus + innate crit + class-specific checks) and special cases below are skipped:
  - Slay Undead (paladin AA) block in `TryCriticalHit` (immediately after NPC crit check).
  - Subsequent class-specific crit handling in the legacy path.
- **zone/attack.cpp::TryPetCriticalHit**: New DEX-based pet crit path returns early; legacy PetCriticalHit AA-based chance is skipped under the new rule.
- **Proc chains**: Added DEX-based multi-proc chaining; no legacy proc chain existed, but note this can change proc behavior.
- **Class-specific crit behaviors**: When `UseNewDexFormulas` is true, legacy crit flow (including class-specific nuances) is skipped. Slay Undead (paladin-only) is called out separately above; audit other class-specific crit behaviors that should be ported (e.g., ranger/rogue/berserker nuances).

## Action items
- Revisit Slay Undead (paladins) and other class-specific crit behaviors to decide how/if to reintegrate them into the new DEX path.
- Confirm desired pet crit behavior under the new system (owner DEX scaled) vs. legacy PetCriticalHit AA-only.
- Audit any other legacy crit-side effects that should be ported (e.g., class-specific messaging/flags).
- Wire `CombatBalance::DEX_WHIRLWIND_EXTRA_DIVISOR` to an actual skill/discipline hook once we formalize Whirlwind/AoE multi-hit handling. (Riposte extra hit is implemented.)

## Next steps
- Playtest/tune DEX crit, proc, and twincast divisors now that the new paths are wired (melee, spells, DOTs).
- Decide whether to enable `ENABLE_DEX_DOT_TWINCAST` by default after balance review.
- Validate DEX-based multi-hit additions (backstab/frenzy/flurry/riposte) against desired class feel.
