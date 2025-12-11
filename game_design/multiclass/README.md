# Multiclass (3-at-once) – Technical Exploration

Goal: allow a single character to be simultaneously three classes (shared level) with access to all gear/spells/AAs/skills of any of the three, and have the client display all three classes. This is an exploration doc to map work needed on server + eq-core DLL.

## Core Constraints & Targets
- Character keeps one level, one inventory, one AA pool, one spellbook, one skill table.
- Each of the 3 chosen classes contributes:
  - Equipment use flags (deity/race remain).
  - Spell gems/book access and casting rules.
  - AA availability and activation.
  - Skill caps and innate class bonuses (pets, disciplines).
  - UI display of all three class names in client.
- Server authoritative; DLL only for client presentation or UI unlocks.

## Server-Side Work (high level)
- Data model
  - Extend character record to store `class_primary`, `class_secondary`, `class_tertiary` (or array of 3 class IDs).
  - Extend rule set to gate the feature (enable/disable multiclass).
- Capability resolution
  - Skills: merge per-class caps; likely max(cap) or sum? Decide per-skill strategy; store merged caps per level.
  - Spells: union of class spell lists; maintain memorization limits; add validation in `CastSpell`, `Client::MemorizeSpell`.
  - AAs: union of class AA tables; gate purchase by level and choose how AA bonuses stack (watch duplicates).
  - Innates/disciplines: allow activation of disciplines from any of the three classes; ensure reuse timers shared.
  - Pets: allow pet families for pet classes; avoid double-adding passive pet bonuses.
  - Item class checks: relax `Classes` mask to permit any of the 3 classes.
- Networking
  - Pack all 3 class IDs in player profile (the client only knows one; DLL will read the extra payload).
  - Option: embed in an unused section of `OP_PlayerProfile` or a custom opcode to be consumed by the DLL (patterned after classless DLL’s custom opcodes).

## Client/DLL Work
- Hook incoming profile: capture the extra class IDs carried in the server packet/custom opcode.
- UI display:
  - Override class labels to show “Class1 / Class2 / Class3”.
  - Optionally show combined skill caps or per-class caps in alternate UI panels.
- Equipment gating:
  - If client refuses equipping based on single-class mask, bypass via DLL (similar to classless DLL hook).
- Spell/AAs:
  - UI should allow viewing and memorizing spells from all three classes; may need DLL to relax client-side filters.
- Logging: add debug breadcrumb when multiclass payload is received and parsed.

## Proposed Payload Strategy (server → client)
- Reuse classless pattern: add a custom opcode (e.g., `OP_MultiClassInfo`) with a small struct:
  ```
  struct MultiClassInfo { uint8 count; uint8 class_ids[3]; }
  ```
  - Send on zone-in and on class change.
  - DLL caches and updates UI labels.
- For the base profile (`OP_PlayerProfile`), optionally stash the extra classes in unused bytes at a known offset; DLL can read them if present for redundancy.

## Testing Plan (incremental)
1) Server only: add 3-class fields; unit test merge rules for skills/spells/AA eligibility.
2) Server packets: add `OP_MultiClassInfo`; log send/receive counts.
3) DLL: hook packet, log cached classes, override class label text; verify in-game display.
4) Equip tests: items restricted to any of the 3 classes equip successfully.
5) Spell/AAs: ensure UI shows and allows casting/activation of abilities from all three classes.
6) Regression: normal single-class players unaffected when rule disabled.

## Open Design Questions
- Skill cap merge policy (max vs weighted vs per-tree choice).
- Duplicate AA effects stacking rules.
- Discipline reuse timers shared or per-class?
- Pet conflicts (multiple pet classes at once).
- Balance rule: optional “main class” focus vs full parity.

## Next Steps
- Decide merge policies (skills/AA/pets) and document per-system.
- Add server fields + rule flag; create packet scaffolding for `OP_MultiClassInfo`.
- Implement DLL packet hook (reuse classless `HandleWorldMessage` detour pattern) to cache/display the three classes.
- Build QA matrix to cover gear, spells, AAs, skills, pets, and UI. 
