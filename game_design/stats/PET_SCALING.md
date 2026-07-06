# Pet Scaling - Shared Stat Transfer Model

Date: 2026-06-29
Owner: Server/Combat Systems
Status: Design target for hotfixable implementation

## Purpose

Pet scaling should be a shared subsystem, not a separate one-off rule inside STR, DEX, INT, or CHA.

The desired model is:

1. Pets inherit a small, configurable portion of the owner's relevant stats.
2. CHA acts as the main pet-power stat by multiplying or amplifying that inherited contribution.
3. Pet gear remains a separate contribution channel.
4. Every lever is hotfixable through `zone/combat_balance.ini` and reloadable with `#combatbalance reload`.

This avoids scattered pet formulas where STR gives one kind of pet inheritance, INT gives another, DEX gives another, and CHA separately multiplies HP/damage without a shared cap.

## Design Goals

- Pet classes should care about CHA as the primary pet-scaling stat.
- Non-CHA owner stats should still matter at a small transfer rate.
- Pet gear should always remain valuable and should not be erased by owner stats.
- Pet scaling should be globally tunable without rebuilding or restarting the server.
- Pets should not become unbounded when owner stats, CHA, buffs, and pet gear all stack.
- Swarm pets, charmed pets, bots, and NPC-owned pets need explicit policy instead of inheriting player-pet behavior accidentally.

## Contribution Channels

| Channel | Example Source | Intended Role |
| :--- | :--- | :--- |
| Pet base stats | NPC/pet template | Baseline pet identity. |
| Pet gear stats | Equipped pet gear / summoned pet gear | Direct pet progression. |
| Owner stat transfer | Owner STR/STA/DEX/AGI/INT/WIS/CHA | Small inherited benefit from the owner. |
| Owner CHA pet multiplier | Owner CHA | Main pet-class investment lever. |
| Class/archetype scalar | Magician/Necro/Beastlord/Enchanter/etc. | Class identity and pet-specialist tuning. |
| Final pet output cap | Runtime safety cap | Prevents stacked channels from trivializing content. |

## Recommended Formula Shape

Use one shared helper rather than per-stat ad hoc logic:

```text
owner_transfer_stat =
    owner_STR * PET_OWNER_STR_TRANSFER +
    owner_STA * PET_OWNER_STA_TRANSFER +
    owner_DEX * PET_OWNER_DEX_TRANSFER +
    owner_AGI * PET_OWNER_AGI_TRANSFER +
    owner_INT * PET_OWNER_INT_TRANSFER +
    owner_WIS * PET_OWNER_WIS_TRANSFER +
    owner_CHA * PET_OWNER_CHA_TRANSFER

cha_multiplier = ApplyCurve([Curve.PET_CHA_POWER], owner_CHA)
class_multiplier = PET_CLASS_SCALAR_<owner class or pet archetype>

pet_effective_bonus =
    (owner_transfer_stat * PET_OWNER_TRANSFER_GLOBAL_SCALAR * cha_multiplier * class_multiplier)
    + (pet_gear_stat * PET_GEAR_STAT_WEIGHT)

final_pet_output = ApplyFinalSoftcapAndHardcap(pet_base_output + pet_effective_bonus)
```

This is a shape, not a tuning commitment. Exact divisors, transfer rates, curves, caps, and class multipliers belong in `zone/combat_balance.ini`.

## Runtime Keys

Suggested scalar keys:

- `PET_OWNER_TRANSFER_GLOBAL_SCALAR`
- `PET_OWNER_STR_TRANSFER`
- `PET_OWNER_STA_TRANSFER`
- `PET_OWNER_DEX_TRANSFER`
- `PET_OWNER_AGI_TRANSFER`
- `PET_OWNER_INT_TRANSFER`
- `PET_OWNER_WIS_TRANSFER`
- `PET_OWNER_CHA_TRANSFER`
- `PET_GEAR_STAT_WEIGHT`
- `PET_CHA_MULTIPLIER_MIN`
- `PET_CHA_MULTIPLIER_MAX`
- `PET_OUTPUT_SOFTCAP_START`
- `PET_OUTPUT_SOFTCAP_POWER`
- `PET_OUTPUT_HARDCAP_VALUE`
- `PET_CLASS_SCALAR_MAGICIAN`
- `PET_CLASS_SCALAR_NECROMANCER`
- `PET_CLASS_SCALAR_BEASTLORD`
- `PET_CLASS_SCALAR_ENCHANTER`
- `PET_CLASS_SCALAR_OTHER`

Suggested curve sections:

- `[Curve.PET_OWNER_TRANSFER]`
- `[Curve.PET_CHA_POWER]`
- `[Curve.PET_HP]`
- `[Curve.PET_DAMAGE]`
- `[Curve.PET_CRIT]`
- `[Curve.PET_PROC]`
- `[Curve.PET_MITIGATION]`

## Feature Gates

Use existing stat feature gates for stat-specific owner contributions, but keep pet scaling behind a pet-specific policy gate if implementation needs one.

Recommended hierarchy:

1. Pet scaling disabled: use legacy pet behavior.
2. Pet scaling enabled + `Combat:UseHotfixableCombatBalance = false`: use compiled fallback constants.
3. Pet scaling enabled + `Combat:UseHotfixableCombatBalance = true`: use runtime values.

Candidate rule:

- `Combat:UseNewPetStatScaling`

CHA pet empowerment should require both:

- `Combat:UseNewPetStatScaling`
- `Combat:UseNewCharismaSystems`

This lets us test generic owner stat transfer separately from CHA as the pet-power stat.

## Interaction With Individual Stats

- STR: contributes to pet physical damage/ATK through the shared owner transfer model. Do not add separate STR-only pet inheritance unless it feeds this helper.
- STA: contributes to pet HP/mitigation through the shared owner transfer model at a small rate.
- DEX: contributes to pet crit/proc/twincast through the shared owner transfer model at a small rate.
- AGI: contributes to pet avoidance/haste through the shared owner transfer model at a small rate.
- INT: contributes to pet spell/offensive scaling, especially for caster pets, through the shared owner transfer model.
- WIS: contributes to pet defensive/support scaling where relevant.
- CHA: acts as the main pet-power multiplier and class/archetype amplifier, not as an unrelated second pet system.

## Pet Gear Policy

Pet gear should be a direct pet contribution channel:

- Pet gear should not be multiplied too aggressively by owner CHA.
- Pet gear should have its own weight so it can be tuned independently from owner stats.
- If pet gear becomes mandatory or too strong, reduce `PET_GEAR_STAT_WEIGHT` or lower final pet output caps.

## Pet Type Policy

Each implementation packet must explicitly state which pet types are affected:

- Permanent player pets
- Temporary swarm pets
- Charmed NPC pets
- Bot pets
- NPC-owned pets
- Mercenary-related pets, if any

Default recommendation:

- Permanent player pets: affected.
- Swarm pets: affected only if explicitly enabled with reduced scalar.
- Charmed NPC pets: separate policy because they already have NPC stats and can become dangerous quickly.
- NPC-owned pets: unaffected unless encounter design explicitly opts in.

## Validation

Minimum validation for any pet-scaling implementation:

- Gate false: legacy pet behavior unchanged.
- Gate true + hotfix false: compiled fallback constants used.
- Gate true + hotfix true: runtime values used after `#combatbalance reload`.
- Invalid runtime value: logs warning and falls back.
- Owner stat transfer test: set `PET_OWNER_TRANSFER_GLOBAL_SCALAR` high and verify pet output changes.
- CHA multiplier test: set `[Curve.PET_CHA_POWER]` or `PET_CHA_MULTIPLIER_MAX` high and verify CHA-heavy owner produces a much stronger pet.
- Pet gear test: equip pet gear and verify `PET_GEAR_STAT_WEIGHT` changes its impact independently from owner stat transfer.
- Safety test: stack high owner stats, high CHA, buffs, and pet gear; verify final softcap/hardcap prevents runaway pets.

The exact numbers are not important during first implementation. The goal is to prove each lever works, reloads, and can be rolled back without restarting the server.
