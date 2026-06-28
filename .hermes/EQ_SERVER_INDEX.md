# EQ Server Repository Index

## Core Mechanics & Items
- **Files:** `common/item_instance.cpp`, `common/item_data.h`
- **Keywords:** Damage scaling, Item proportions, Aspect ratios, Base Damage
- **Caution:** Scaling relies on a per-shot "balance" ratio; modifying base values affects all items using that scale logic.

## Configuration & Systems
- **Files:** `common/item_scaling_config.cpp`, `common/item_scaling_config.h`
- **Keywords:** Ability curves, Tier scaling, Slot multipliers
- **Caution:** Global modification to configuration constants can impact unintended item types.

## Infrastructure & Networking
- **Folders:** `network/`, `gateway/`
- **Keywords:** Packet handling, Protocol definition
- **Caution:** Structure changes in these files may break packet serialization across client/server.

## World & Zone Logic
- **Files:** `world_server.cpp`, `zone_server.cpp` (or under `src/`)
- **Keywords:** Entity logic, Region bounds, NPC AI
- **Caution:** Core state management is sensitive to changes in object lifecycle timing.

## Hardware & Hook Extensions
- **Folders:** `dl/`, `hook/`
- **Keywords:** Buffer overrides, Pointer mapping, External hooks
- **Caution:** These areas often contain memory-unsafe code or low-level optimizations that bypass standard safety checks.
