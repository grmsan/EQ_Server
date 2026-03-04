# tools/

Developer utilities for the EQ_Server project.  
All scripts read DB credentials from `../eqemu_config.json`.

## Scripts

| Script | Purpose |
|--------|---------|
| `db_viewer.py` | General-purpose DB query CLI. `python tools/db_viewer.py --query "SELECT ..."` |
| `model_raid_economy.py` | Queries live loot tables to validate the Essence economy (iLevel 1:1 model, tier costs, per-zone totals). |
| `validate_ilevel.py` | Tests the power-score → iLevel algorithm against real DB items. Shows distribution histogram and economy spot-check. |
| `item_scale_preview.py` | Tkinter GUI for previewing `item_scaling.json` tier scaling values. Separate from iLevel/Essence system. |

## Quick Start

```bash
# From repo root
python tools/validate_ilevel.py          # iLevel algorithm against live items
python tools/model_raid_economy.py       # Full raid economy analysis
python tools/db_viewer.py --query "SELECT name FROM items LIMIT 5"
```

## Design References

- [ITEM_LEVEL_SYSTEM.md](../game_design/infinite_progression/ITEM_LEVEL_SYSTEM.md) — iLevel algorithm spec
- [INFINITE_ITEM_PROGRESSION_DESIGN.md](../game_design/infinite_progression/INFINITE_ITEM_PROGRESSION_DESIGN.md) — Master design doc
- [AUGMENT_SYSTEM.md](../game_design/infinite_progression/AUGMENT_SYSTEM.md) — Augment subsystem deep-dive
