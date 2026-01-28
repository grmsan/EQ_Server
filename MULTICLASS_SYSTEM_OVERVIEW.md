# Multiclass System Overview (RoF2)

> **📋 Master Technical Document:** [game_design/multiclass/IMPLEMENTATION_PLAN.md](game_design/multiclass/IMPLEMENTATION_PLAN.md)
> **🚀 Quick Start:** [game_design/multiclass/QUICK_START.md](game_design/multiclass/QUICK_START.md)

This document provides a high-level and technical overview of the multiclass (gestalt) system, including its goals, architecture, and how it interacts with the scaling item system. It is intended for developers and technical leads joining the project.

## High-level goals
- Allow a character to own multiple classes at once (e.g., Warrior + Ranger + Magician).
- Treat all owned classes equally (no permanent "base" class advantages).
- Keep the server authoritative for gameplay rules while the RoF2 client remains stock and stable.
- Support a scaling item system where items can level up and are persisted as real DB items (not client hacks).

## Key design principles
- Server authoritative: gameplay effects must be validated on the server using class bitmasks, not `GetClass()`.
- Union-of-classes: any system that checks class eligibility should consider all owned classes.
- Soft-lock persistence: learned spells, AAs, and skills are retained even if a class is removed; use-time checks gate them.
- Minimal client changes: prefer DLL hooks for UI visibility and quality-of-life only.

## Core architecture (server)
- **Class ownership**
  - Stored as a classes bitmask in `data_buckets` (canonical key: `GestaltClasses`).
  - Base class in `character_data.class` remains for compatibility, but is not exclusive.
  - Helper API:
    - `Client::GetClassesBits()` returns union-of-classes bitmask.
    - `Client::HasClass(class_id)` checks membership.

- **Rules**
  - `Custom:MulticlassingEnabled` (master toggle)
  - `Custom:MulticlassMaxClasses` (cap)
  - `Custom:MulticlassBucketKey` (back-compat/override)
  - `Custom:UseDynamicAATimers` and `Custom:BypassMulticlassStackConflict` (THJ parity)

- **Visibility and gating**
  - Spells, AAs, skills, disciplines, items, and other class-limited features must use union-of-classes checks.
  - Use-time checks are authoritative (cast, activate, equip, skill use, AA activation).

## Client integration (RoF2 + DLL)
- The RoF2 client calculates many usability filters locally.
- A custom `dinput8.dll` adds hooks to align UI visibility with the multiclass server rules:
  - Override spell level checks to consider owned classes.
  - Override usable class masks for UI filters.
  - Expose skills with non-zero server values even if base class would hide them.
  - Rewrite `/who` and class labels for multiclass display.
- The client DLL should be the primary place for custom UI behavior and stat presentation updates.
  - Use it to keep the stock client stable while making multiclass stats, labels, and filters match server truth.
  - See `/docs/` for stat update notes and packet-level references (RoF2 specifics).

## Scaling item system
- Items that level up are created and stored as real DB items (IDs can exceed 1B).
- Client sees items as standard DB items with normal stats.
- Multiclass must not corrupt item `Classes` masks or merchant lists; scaling items must remain standard DB items.

## Current pain points (active debugging)
- Spell vendor "Show usable items" still shows unusable scrolls (e.g., `Spell: Column of Frost`, item id `15380`).
- Spell vendor UI shows base-class labels for aggregated spells (`RNG(2)` vs `MAG(2)`).
- AA window missing AAs for added classes (and sometimes base-class AAs).
- Skills for added classes not consistently visible in the skills window.

## Recommended tooling
- Server GM inspectors:
  - `#mc spell <spell_id>`, `#mc aa <aa_id>`, `#mc item <item_id>`, `#mc merchant <slot|item_id>`
  - Print class masks, gating logic, and the class that satisfies each check.
- Client DLL probes:
  - On-demand logging for merchant selection, AA list refresh, skill list refresh.
  - Dump return-address RVAs to pinpoint call sites.

## Roadmap (near-term)
- Finish THJServer parity for: spells, AAs, skills, and merchant filters.
- Establish repeatable smoke tests in `TODO_THJSERVER_MULTICLASS_TESTS.md`.
- Reduce reliance on rebuilds by expanding server and DLL diagnostics.

## Docs and references
- `/docs/` contains packet notes and stat update references used by the DLL and server.
- Use `/docs/` as the first stop for context before altering packet formats or client hooks.
