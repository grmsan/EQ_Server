# Gear Stat Budgets (Expansions 0-4)

Scope:
- This uses the live DB expansion IDs, not a 1-based design shorthand.
- `0 = Classic`, `1 = Kunark`, `2 = Velious`, `3 = Luclin`, `4 = PoP`.
- Data source is droppable equippable gear from zone loot tables, grouped by NPC level in 10-level bands.
- The companion script is [tools/stat_budget_report.py](C:/Users/marsh/OneDrive/Documents/GitHub/EQ_Server/tools/stat_budget_report.py).

Important limitation:
- This document measures **raw primary stat budgets**, not total item quality.
- It should not be used by itself to claim "Kunark gear is better than Velious gear" or similar cross-expansion quality rankings.
- For that question, total power metrics like iLevel/AC/HP/Mana/resists matter more than raw best-primary alone.

## Core Purpose

This work is trying to solve a real balance problem: the stat redesign wants large, meaningful stat curves, but encounter tuning needs a believable answer to "what does a normal player actually have at this level and expansion?"

That means we need a stat-budget language that is:
- simple enough to tune around
- grounded in the real item DB
- clear about starter vs mid vs endgame
- honest about where power comes from: raw stats, heroics, AC/HP/Mana, augments, or tier bonuses

## Player Experience

For a new or average player, the current system produces one strong feeling and one weak one.

The strong feeling:
- `Enchanted` gear is immediately legible because doubling raw attributes is huge on the character sheet.

The weak feeling:
- once the player has mostly `Enchanted` gear, raw STR/STA/DEX/AGI/INT/WIS/CHA growth flattens hard, because `Legendary` does not increase raw attributes above the `Enchanted` ceiling.

What that means by audience:
- New player: sees clear wins from getting stat-bearing gear at all.
- Average player: feels a big jump at `Enchanted`, then a muddier progression afterward.
- Optimizer: learns quickly that raw-attribute progression is mostly a "get to Enchanted" game, then pivots to heroics, AC/HP/Mana, haste, attack, and augments.
- Casual player: may feel that higher-tier upgrades are less exciting than the color/name implies if they mostly look at sheet stats.
- Veteran: will absolutely target the systems that still scale after `Enchanted`, which means your balance model needs to treat augments/heroics as part of endgame, not an optional side bonus.

## Incentives And Exploits

Current incentives:
- Players are pushed toward items with the highest single primary stat because `Enchanted` doubles that value cleanly.
- If balance is tuned around raw primary stats, players will undervalue pure-defense or utility pieces unless those pieces are compensated somewhere else.
- If endgame challenge expects "500 to all stats" from raw gear alone, players will miss that target and feel weak unless augments or heroics secretly carry the gap.

Likely player behaviors:
- Min-maxers will solve for "best raw primary at base tier, then double it."
- Players will compare `Enchanted` favorably and `Legendary` unfavorably if they only see raw stats moving at the first step.
- If augs are required to make the final budget work, they stop being optional customization and become mandatory tax.

## Friction Vs Depth

The current model has good clarity but uneven depth.

Good friction:
- `Base -> Enchanted` is easy to understand.
- Expansion and level-band budgeting is easy to communicate.

Bad friction:
- `Legendary` and `Mythic` add complexity, but not equivalent raw-stat clarity.
- If encounter tuning is keyed to raw stats, the tier system asks players to engage with multiple progression systems without paying off in the metric they are taught to care about.

Design read:
- This is not overcomplicated yet.
- But it is split-brain: raw attributes tell one progression story, while heroics/combat stats tell another.

## Interaction With Other Systems

This matters a lot because the stat docs expect large values:
- STR wants meaningful gear-driven growth.
- STA wants predictable HP and sustain bands.
- INT/WIS want mana and spell-output curves that do not guess blindly.
- DEX/AGI/CHA can spike hard if their effective totals are underestimated.

The main interaction problem is this:
- the current item-tier system is good at scaling total item power
- but it is weaker at scaling raw primary stats deep into an expansion

So if the combat formulas are balanced around raw sheet stats, your upper-end tuning may be wrong.
If they are balanced around total item power, heroics, and augments, the current tier model makes more sense.

## What Is Good

The strongest design seed is this:
- a 20-slot stat-budget model is absolutely the right way to tune the new stat formulas

That gives you something operational:
- "Level 50 Classic/Kunark player should have about X raw STR before buffs"
- "Fresh level-cap Luclin player should have about Y STA"
- "Best-in-era PoP player should only reach Z with aug investment"

That is the right abstraction. Keep it.

## What Is Weak

The weak point is not the reporting idea. The weak point is the current raw-stat growth model.

Main issues:
- `Enchanted` doubles raw attributes, but `Legendary` does not move them higher.
- That makes the raw-stat endgame band too compressed unless augments or heroics do the real work.
- Early-era bands are noisy because old loot tables include many off-stat and low-stat pieces.
- A "20 slots x average item" model is directionally useful, but it still needs starter/mid/end assumptions, not one flat number.

Most important practical problem:
- If you want "starter 200 to all, end 500 to all" as raw stat targets in expansions 0-4, the current tier rules do not naturally support that.

## Measured Snapshot

Method:
- Unique droppable equippable item IDs per `(expansion, npc level band)`
- Only items with at least one positive primary stat
- Default model: `20 slots`, `75 baseline raw stat`, `Enchanted = x2 raw attributes`, `Legendary = same raw attribute ceiling as Enchanted`

### Per-Band Raw Primary Stat Snapshot

`MedPri` = median best primary stat on an item in that band.
`P90Pri` = strong upper-end stat item in that band.

| Exp | Era | Level Band | Items | AvgPri | MedPri | P90Pri | AvgAll |
|---|---|---:|---:|---:|---:|---:|---:|
| 0 | Classic | 01-10 | 27 | 2.00 | 2.00 | 2.40 | 2.30 |
| 0 | Classic | 11-20 | 42 | 3.81 | 3.00 | 5.00 | 4.00 |
| 0 | Classic | 21-30 | 75 | 4.21 | 4.00 | 6.00 | 5.60 |
| 0 | Classic | 31-40 | 119 | 5.45 | 5.00 | 9.00 | 8.41 |
| 0 | Classic | 41-50 | 256 | 7.07 | 5.00 | 13.00 | 13.53 |
| 0 | Classic | 51-60 | 369 | 7.30 | 6.00 | 13.00 | 14.48 |
| 0 | Classic | 61-70 | 58 | 7.00 | 7.00 | 10.60 | 15.10 |
| 1 | Kunark | 01-10 | 4 | 2.00 | 2.00 | 2.70 | -0.50 |
| 1 | Kunark | 11-20 | 24 | 2.46 | 2.00 | 4.00 | 3.62 |
| 1 | Kunark | 21-30 | 40 | 4.53 | 4.00 | 8.00 | 6.58 |
| 1 | Kunark | 31-40 | 90 | 4.33 | 4.00 | 6.00 | 7.07 |
| 1 | Kunark | 41-50 | 111 | 5.37 | 5.00 | 10.00 | 9.16 |
| 1 | Kunark | 51-60 | 183 | 6.68 | 6.00 | 10.00 | 12.99 |
| 1 | Kunark | 61-70 | 89 | 10.74 | 10.00 | 16.00 | 28.45 |
| 2 | Velious | 11-20 | 10 | 2.20 | 2.00 | 3.10 | 0.90 |
| 2 | Velious | 21-30 | 51 | 3.10 | 3.00 | 5.00 | 4.84 |
| 2 | Velious | 31-40 | 117 | 4.56 | 4.00 | 8.00 | 8.35 |
| 2 | Velious | 41-50 | 185 | 5.37 | 5.00 | 10.00 | 9.64 |
| 2 | Velious | 51-60 | 384 | 7.20 | 6.00 | 12.00 | 15.37 |
| 2 | Velious | 61-70 | 109 | 8.12 | 8.00 | 12.20 | 23.39 |
| 3 | Luclin | 01-10 | 7 | 5.86 | 2.00 | 12.00 | 7.29 |
| 3 | Luclin | 11-20 | 22 | 2.05 | 2.00 | 3.00 | 3.68 |
| 3 | Luclin | 21-30 | 58 | 3.00 | 3.00 | 5.00 | 4.69 |
| 3 | Luclin | 31-40 | 131 | 4.95 | 4.00 | 10.00 | 10.31 |
| 3 | Luclin | 41-50 | 142 | 5.80 | 5.00 | 10.00 | 12.99 |
| 3 | Luclin | 51-60 | 258 | 8.07 | 6.00 | 15.00 | 18.76 |
| 3 | Luclin | 61-70 | 78 | 12.35 | 10.00 | 20.00 | 28.24 |
| 4 | PoP | 41-50 | 23 | 6.74 | 5.00 | 10.00 | 12.22 |
| 4 | PoP | 51-60 | 62 | 7.92 | 6.00 | 14.70 | 17.76 |
| 4 | PoP | 61-70 | 95 | 9.13 | 10.00 | 12.60 | 21.71 |

### Expansion-Cap Full-Set Estimate

Interpretation:
- `Starter` = `75 + (20 x MedPri x 1.0)`
- `Mid` = `75 + (20 x MedPri x 2.0)` using current `Enchanted`
- `End` = `75 + (20 x P90Pri x 2.0)` with no separate raw-stat increase from `Legendary`

| Exp | Era | Cap Band | Starter | Mid | End | Read |
|---|---|---:|---:|---:|---:|---|
| 0 | Classic | 61-70 | 215 | 355 | 499 | Raw cap band is already pretty compressed after `Enchanted` |
| 1 | Kunark | 61-70 | 275 | 475 | 715 | High because Kunark top-end itemization has some very fat stat pieces |
| 2 | Velious | 61-70 | 235 | 395 | 563 | Reasonable if endgame also leans on heroics and AC/HP |
| 3 | Luclin | 61-70 | 275 | 475 | 875 | Vex Thal-era outliers spike hard |
| 4 | PoP | 61-70 | 275 | 475 | 579 | Smaller end spread than Luclin because raw stats are flatter than total power |

### Cross-Expansion Reality Check

If you compare Kunark and Velious by this document's raw-primary lens alone, Kunark can look stronger in some top-end buckets. That is misleading.

Using a rough iLevel-style total power metric on level 61-65 droppable stat gear:
- Kunark (`expansion = 1`) average total item power is about `286.7`
- Velious (`expansion = 2`) average total item power is about `370.4`

So the earlier "Kunark 715 vs Velious 563" style inversion is **not** evidence that Velious gear is weaker overall. It is evidence that:
- raw best-primary stat is an incomplete metric
- VP-era loot has some unusually fat raw-stat items
- Velious expresses more of its superiority through broader total power, not just one raw primary number

## Better Versions

### Safe Version

Keep the current raw-stat rules, but be explicit about what they mean:
- Balance raw-primary stat formulas around `Starter` and `Mid`, not mythical fully-maxed raw-stat numbers.
- Treat `End` as a combined budget from raw stats + heroics + augments + HP/AC/haste, not raw stats alone.
- Publish target bands like:
  - fresh cap
  - mostly enchanted
  - fully augmented / endgame optimized

This is the least risky path.

### Ambitious Version

Let raw attributes keep growing after `Enchanted`.

Example:
- `Enchanted = x2.0 raw attrs`
- `Legendary = x2.4 raw attrs`
- `Mythic = x2.8 raw attrs`

Or:
- `Enchanted = x2.0`
- `Legendary = x2.0 + flat per-slot bonus`
- `Mythic = x2.0 + larger flat per-slot bonus`

This makes sheet stats continue to tell the same story the tier colors are telling.

Risk:
- easy to blow up STR/STA formulas if softcaps are not ready

### Weird Version

Stop using raw attributes as the main balance anchor and move to a formal per-slot "stat budget rating."

Example:
- each expansion/level band has a target budget
- each item spends that budget across raw stats, heroics, HP/AC/Mana, haste, attack, utility
- combat tuning uses "effective budget" rather than only raw STR/STA/etc.

This is cleaner long-term, but it is a more abstract design language and harder to communicate to players.

## Decision

**Iterate**

Keep the reporting idea and keep the 20-slot budget model.

Do not keep the current design assumptions unexamined if your real target is "starter 200 to all, end 500 to all" as raw-stat expectations. The current tier model can support that only inconsistently across expansions 0-4, and mostly by leaning on item outliers or external systems.

Practical recommendation:
- Use the script to lock a real target table for each expansion cap.
- Decide whether endgame power is supposed to live in raw stats or in heroics/augments/combat stats.
- If the answer is raw stats, change `Legendary` and possibly `Mythic` so they continue raw-attribute growth.
- If the answer is not raw stats, document that clearly and tune combat formulas around `starter raw`, `mid raw`, and `end effective` budgets instead of one vague number.
