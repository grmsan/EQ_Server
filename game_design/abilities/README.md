# EQEmulator Abilities Development Guide

This folder documents how this repo implements custom spells, disciplines, and alternate advancements (AAs), including the client sync steps (`spells_us.txt` / `dbstr_us.txt`) used by `server_manager.py`.

## Docs In This Folder

### 1. `SPELLS_GUIDE.md`
- Spell schema and effect fields
- C++ extension points for custom SPAs
- Client synchronization behavior

### 2. `DISCIPLINES_GUIDE.md`
- Discipline spell setup (`IsDiscipline=1`)
- Tome/item wiring
- Endurance and timer handling

### 3. `AA_GUIDE.md`
- AA schema (`aa_ability`, `aa_ranks`, `aa_rank_effects`, `aa_rank_prereqs`)
- Active vs passive AA setup
- AA visibility and reload troubleshooting

### 4. `IMPLEMENTATION_CHECKLIST.md`
- Build/deploy/test checklist
- Common failure modes and recovery steps

## Repo-Specific Workflow (Authoritative)

1. Apply SQL changes (typically in `utils/sql/custom/...`).
2. Reload server data:
   - AA table changes: `#reload aa_data`
   - Spell table changes: run `shared_memory.exe` (or Server Manager "Shared Memory")
3. Export client files with Server Manager:
   - `Export spells_us`
   - `Export dbstr_us`
4. Verify Server Manager "Client Asset Status" shows up to date.
5. Fully restart client (`eqgame`) before validating in-game UI/AA/spell text.

## Important Notes

- `spells_new.id` must stay `< 65535` for client compatibility.
- `db_str` entries are required for correct AA names/descriptions in the client window.
- In this branch, AA class masks are evaluated with a left shift (`1 << class_id`) in `zone/aa.cpp`.
  - Example: Warrior = `2`, Cleric = `4`, ... all classes = `131070`.
- Reserved IDs currently in active custom use:
  - Spells: `5824` (Origin alias path), `65000` (Heroic Throw), `65010` (Colossal Smash)
  - AA/Ranks: `331` / `1000` (Origin/Bazaar-and-Back alias), `10000` and `12000` bands (custom warrior AAs)
  - DB strings: `1000`, `10000`, `10010`, `10011`

## Useful In-Game Commands

```text
#castspell [spell_id]              # Cast spell on yourself
#reload aa_data                    # Reload AA definitions
#set aa_points aa [amount]         # Set unspent AA points on target
#showstats                         # Show current stat state
```

`#grantaa` in this codebase grants all AAs up to a level on the target; it is not "grant by AA id".

## Where Code Lives

- `zone/spell_effects.cpp` - Spell effect execution
- `zone/spells.cpp` - Casting/recast/core spell flow
- `zone/aa.cpp` - AA purchase/activation/send table logic
- `zone/bonuses.cpp` - Passive bonuses from spells/items/AAs
- `server_manager.py` - Export of `spells_us.txt` / `dbstr_us.txt`

## Next

- For spells: see `SPELLS_GUIDE.md`
- For AAs: see `AA_GUIDE.md`
- For full rollout checklist: see `IMPLEMENTATION_CHECKLIST.md`
