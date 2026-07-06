# Classic Ranger Legendary Build

Purpose:
- Build a concrete level 50 ranger from the current `expansion = 0` item pool.
- Prioritize DEX, but prefer broader items when the DEX loss is small.
- Use the server's current Legendary scaling rules so the result can be used for balance targets.

Important caveat:
- This is based on the **current server Classic loot-table pool**, not a hand-curated "strict vanilla EQ" list.
- That means custom Classic-era items that exist in your live DB are included if they are on Classic loot tables.
- If you want a strict-era reference build later, the next step is adding a curated exclusion list for custom items.

Method:
- Source pool: `zone.expansion = 0`, `reclevel <= 50`, ranger-usable, base items only.
- Excluded: generated `Enchanted`, `Legendary`, and `Mythic` rows.
- Slot scoring:
  - DEX is weighted highest.
  - STR, STA, AGI, WIS, AC, HP, haste, attack, and resists all matter.
  - Broad-stat items are favored over narrow one-stat items when the DEX difference is small.
  - Range slot heavily weights bow ratio, then stats.
- Legendary scaling:
  - Attributes: `x2.0`
  - Combat stats: `x2.6`
  - Haste: `+5`
  - Attack on weapons: `base attack + damage * 2`
  - Spell damage: `base INT * 2`
  - Heal power: `base WIS * 2`

## Slot-by-Slot Build

| Slot | Item | Why it won |
|---|---|---|
| Head | Helm of the Untamed | Moderate DEX with useful STR/WIS and solid armor value. |
| Face | Abalone Engraved Tribal Mask | Huge DEX with broad secondary stats and large HP. |
| Neck | Bile Etched Obsidian Choker | Clean triple-stat neck with strong HP. |
| Shoulders | Fetid Shiverback Hide Mantle | DEX plus STA/WIS instead of a pure narrow stat piece. |
| Back | Cloak of Combustion | Best example of your rule: high DEX plus very strong broad stats. |
| Chest | Breastplate of the Untamed | Good DEX and rounded physical/wisdom value. |
| Arms | Thorny Vine Vambraces | Simple DEX-forward arms slot. |
| Wrist 1 | Oozing Bracer | Best DEX wrist with extra STA/WIS. |
| Wrist 2 | Hotof's Bracer | No DEX, but broad STR/WIS/CHA package beats filler wrists. |
| Hands | Thorny Vine Gauntlets | Strong DEX plus STR on a slot with thin options. |
| Waist | Runed Bolster Belt | Easy winner because haste plus DEX/STR/STA is too efficient. |
| Legs | Greaves of the Untamed | Balanced slot with DEX/STR/AGI/WIS and good AC. |
| Feet | Leatherfoot Sandals | Broader stat package beats narrow foot fillers. |
| Finger 1 | Moss Encrusted Band | Elite all-around ring with meaningful DEX, HP, and attack. |
| Finger 2 | Steel Ring of the Foot Soldier | Secondary ring that still contributes to all-around sheet quality. |
| Ear 1 | Clawed Earthcrafter's Hoop | Best ear by a wide margin: strong DEX and broad stat support. |
| Ear 2 | Dark Hoop of the Necromancer | Weak second ear slot in this pool, but still better than pure filler. |
| Primary | Ivory Hilted Cleaver | Excellent ratio and strong stat package. |
| Secondary | Reinforced Mephit Talon | Nearly equal ratio to primary and a broad stat spread. |
| Range | Shortbow of the Fighter | Best bow ratio in the Classic ranger-usable pool while still carrying stats. |

## Base Items And Legendary Snapshots

| Slot | Item | Base Snapshot | Legendary Snapshot |
|---|---|---|---|
| Head | Helm of the Untamed | DEX 5, STR 4, WIS 4, AC 16, HP 35 | DEX 10, STR 8, WIS 8, AC 42, HP 91, Heal 8 |
| Face | Abalone Engraved Tribal Mask | DEX 20, STA 15, AGI 12, INT 12, CHA 25, AC 20, HP 150 | DEX 40, STA 30, AGI 24, INT 24, CHA 50, AC 52, HP 390, SD 24 |
| Neck | Bile Etched Obsidian Choker | DEX 10, STR 10, STA 10, AC 10, HP 100 | DEX 20, STR 20, STA 20, AC 26, HP 260 |
| Shoulders | Fetid Shiverback Hide Mantle | DEX 5, STA 5, WIS 4, AC 10, HP 35 | DEX 10, STA 10, WIS 8, AC 26, HP 91, Heal 8 |
| Back | Cloak of Combustion | DEX 30, STR 15, STA 25, INT 20, WIS 20, AC 23, HP 140 | DEX 60, STR 30, STA 50, INT 40, WIS 40, AC 60, HP 364, SD 40, Heal 40 |
| Chest | Breastplate of the Untamed | DEX 12, STR 8, STA 10, AGI 4, WIS 12, AC 37, HP 25 | DEX 24, STR 16, STA 20, AGI 8, WIS 24, AC 96, HP 65, Heal 24 |
| Arms | Thorny Vine Vambraces | DEX 9, INT 9, AC 13 | DEX 18, INT 18, AC 34, SD 18 |
| Wrist 1 | Oozing Bracer | DEX 5, STA 5, WIS 5, AC 13, HP 30 | DEX 10, STA 10, WIS 10, AC 34, HP 78, Heal 10 |
| Wrist 2 | Hotof's Bracer | STR 10, WIS 10, CHA 10, AC 10 | STR 20, WIS 20, CHA 20, AC 26, Heal 20 |
| Hands | Thorny Vine Gauntlets | DEX 9, STR 9, AC 11 | DEX 18, STR 18, AC 29 |
| Waist | Runed Bolster Belt | DEX 10, STR 10, STA 10, AC 5, Haste 31 | DEX 20, STR 20, STA 20, AC 13, Haste 36 |
| Legs | Greaves of the Untamed | DEX 5, STR 5, AGI 5, WIS 5, AC 22, HP 30 | DEX 10, STR 10, AGI 10, WIS 10, AC 57, HP 78, Heal 10 |
| Feet | Leatherfoot Sandals | STR 10, AGI 10, WIS 10, AC 11 | STR 20, AGI 20, WIS 20, AC 29, Heal 20 |
| Finger 1 | Moss Encrusted Band | DEX 15, STR 30, STA 15, INT 20, WIS 20, AC 18, HP 145, ATK 25 | DEX 30, STR 60, STA 30, INT 40, WIS 40, AC 47, HP 377, ATK 65, SD 40, Heal 40 |
| Finger 2 | Steel Ring of the Foot Soldier | DEX 3, STR 3, STA 3, AGI 3, INT 3, WIS 3, AC 4, HP 25 | DEX 6, STR 6, STA 6, AGI 6, INT 6, WIS 6, AC 10, HP 65, SD 6, Heal 6 |
| Ear 1 | Clawed Earthcrafter's Hoop | DEX 15, STR 20, STA 15, INT 12, WIS 12, AC 15, HP 140, ATK 20 | DEX 30, STR 40, STA 30, INT 24, WIS 24, AC 39, HP 364, ATK 52, SD 24, Heal 24 |
| Ear 2 | Dark Hoop of the Necromancer | DEX 3, STA 3, AGI 3, INT 3, WIS 3, CHA 3, AC 3, HP 15 | DEX 6, STA 6, AGI 6, INT 6, WIS 6, CHA 6, AC 8, HP 39, SD 6, Heal 6 |
| Primary | Ivory Hilted Cleaver | DEX 15, STR 20, STA 20, AGI 15, WIS 15, HP 140, DMG 17, Delay 19 | DEX 30, STR 40, STA 40, AGI 30, WIS 30, HP 364, ATK 34, Heal 30, DMG 44, Delay 19 |
| Secondary | Reinforced Mephit Talon | DEX 10, STR 20, STA 20, INT 15, CHA 15, AC 10, HP 140, DMG 17, Delay 19 | DEX 20, STR 40, STA 40, INT 30, CHA 30, AC 26, HP 364, ATK 34, SD 30, DMG 44, Delay 19 |
| Range | Shortbow of the Fighter | DEX 2, STR 3, AGI 3, AC 5, HP 20, DMG 25, Delay 34 | DEX 4, STR 6, AGI 6, AC 13, HP 52, ATK 50, DMG 65, Delay 34 |

## Legendary Gear Totals

Gear-only totals:
- STR: 354
- STA: 312
- AGI: 110
- DEX: 366
- INT: 188
- WIS: 246
- CHA: 106
- AC: 667
- HP: 3042
- Mana: 2704
- Attack: 235
- Haste: 36
- Spell Damage: 188
- Heal Power: 246
- Resists: `MR 286 / FR 184 / CR 198 / DR 170 / PR 148`

Projected level 50 sheet totals with default base stats:
- STR: 429
- STA: 387
- AGI: 185
- DEX: 441
- INT: 268
- WIS: 321
- CHA: 181

## Design Read

What this says about your current Classic curve:
- A DEX-focused Legendary ranger in the live Classic pool can already break `400 DEX` on-sheet.
- The build does **not** get there by stacking pure DEX-only items. It gets there by combining strong DEX pieces with very broad stat packages.
- The second ear, second wrist, and second ring are where the pool thins out. That is useful for tuning because those are natural places to add progression without breaking the whole curve.
- Bow quality is much flatter than worn gear quality. The best ratio bow here is good, but the biggest power jump in this build still comes from worn stats rather than the bow itself.

Recommended use:
- Treat this as a concrete Classic benchmark for ranger combat simulations.
- If your combat model says `~440 DEX` should make a ranger overperform, your current Classic item pool is already there.
- If you want a stricter target like `~400 primary / ~200 secondary`, you likely need either:
  - lighter custom stat packages in Classic, or
  - a stricter "era-authentic only" curated item pool for the balance model.
