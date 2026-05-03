# THJ Gap Register

**Last Updated:** 2026-04-25
**Assignment Board:** [PROJECT_WORKSTREAMS.md](PROJECT_WORKSTREAMS.md)
**Primary Reference:** `extras/THJServer/`
**Reference Report:** `tools/output/multiclass_references.csv`

This register tracks known and suspected gaps between this project and THJ. It is a triage document, not a mandate to port every diff.

---

## Gap Classification

| Class | Meaning | Default Action |
| --- | --- | --- |
| Must Port | Required for intended gameplay, parity, stability, or data safety | Create implementation packet |
| Validate First | Code appears present or intentionally different; needs runtime proof | Create validation packet |
| Design Decision | THJ behavior exists, but local policy may differ | Bring to manager |
| Intentional Divergence | Difference is accepted for this project | Document rationale |
| Ignore | Upstream-only, obsolete, unrelated, or low-value | Leave unassigned |

---

## Gap Review Workflow

1. Pick a bounded packet from [PROJECT_WORKSTREAMS.md](PROJECT_WORKSTREAMS.md).
2. Compare only the files and systems named in that packet.
3. Categorize each meaningful THJ difference using the classes above.
4. Add rows to this register.
5. Create focused implementation, validation, or decision packets only for meaningful gaps.

Do not mark a diff as a gap just because files differ. The question is whether behavior needed by this project is missing, unverified, or intentionally different.

---

## Active Gap Summary

| Area | Status | Current Finding | Next Packet |
| --- | --- | --- | --- |
| Multiclass core | Open | THJ-GAP-01 found five fix/decision deltas plus one validation delta: `/who` payload policy, stale base-class reintroduction in world helper, bard item-click cast guards, AA-table timer preload parity, toggle-AA disabled-state parity, and a remaining memorize-time refresh check | `MC-DEC-01`, `MC-FIX-01`, `MC-FIX-02`, `MC-FIX-03A`, `MC-FIX-03B`, `MC-VAL-03` |
| Combat / pets / mechanics | Open | Known open pet-assist UX delta; recent pet fixes need source-level comparison | `THJ-GAP-02` |
| Bazaar / quests / waypoints | Open | Large THJ script/API surface partially ported; need missing API/data gap list | `THJ-GAP-03` |
| DB / rules / tooling | Open | Reference report shows DB/rule/build diffs; need classify must-port vs irrelevant | `THJ-GAP-04` |
| Bots / mercs / scripting lower-priority diffs | Open | Many checklist diffs exist but likely not all relevant | `THJ-GAP-05` |

---

## Gap Register

| Gap ID | Area | THJ Source | Local Source | Classification | Impact | Recommended Packet | Status |
| --- | --- | --- | --- | --- | --- | --- | --- |
| GAP-001 | Pet assist UX | THJ pet command/UI paths | Live tree has server-side assist behavior but no player-facing `/pet assist` command/UI surface | Design Decision | Full THJ UX parity is incomplete unless we expose a toggle | `THJ-GAP-02` | Open |
| GAP-002 | World `/who` payload shape | `extras/THJServer/world/clientlist.cpp::ClientList::SendWhoAll`; `extras/THJServer/world/clientlist.cpp::ClientList::SendFriendsWho` | `world/clientlist.cpp::ClientList::SendWhoAll`; `world/clientlist.cpp::ClientList::SendFriendsWho` | Design Decision | THJ sends the multiclass bitmask in the class field so the DLL can rewrite `/who` and friends class labels; local world keeps the compatibility class in the payload, which preserves stock-client behavior but blocks THJ-style class-label parity | `MC-DEC-01` | Open |
| GAP-003 | World multiclass mask hydration | `extras/THJServer/world/clientlist.cpp::ClientList::SendWhoAll`; `extras/THJServer/world/clientlist.cpp::ClientList::SendFriendsWho` | `world/clientlist.cpp::GetMulticlassBitsOrBase` | Must Port | Implementation removed the world-side legacy base-class OR reintroduction, so persisted `GestaltClasses` ownership now stays authoritative once present; runtime still needs to prove `/who` filters and friends display no longer resurrect removed starting classes | `MC-VAL-03` | Review |
| GAP-004 | Bard item-click cast rules under multiclass | `extras/THJServer/zone/client_packet.cpp::Handle_OP_ItemVerifyRequest` | `zone/client_packet.cpp::Handle_OP_ItemVerifyRequest` | Must Port | THJ only applies bard song interruption and bard item-click casting shortcuts when the current cast really qualifies under multiclass rules; local logic still treats any bard-owned character as bard-special for these branches, which can mis-handle mixed-class cast replacement and nonzero item cast times | `MC-FIX-02` | Open |
| GAP-005 | AA-table dynamic timer preload parity | `extras/THJServer/zone/aa.cpp::Client::SendAlternateAdvancementTable`; `extras/THJServer/zone/aa.cpp::Client::GetDynamicAATimers` | `zone/aa.cpp::Client::SendAlternateAdvancementTable`; `zone/aa.cpp::{LoadDynamicAATimers, GetDynamicAATimer, SetDynamicAATimer}` | Must Port | `SendAlternateAdvancementTable()` now preloads the persisted dynamic AA timer cache before serializing ranks, matching the THJ refresh pattern without replacing the local bucket-backed timer store; runtime still needs to prove cooldown-family stability after zoning, relogging, or class mutation | `MC-VAL-02`, `AA-02` | Review |
| GAP-006 | Toggle-AA disabled-state cache parity | `extras/THJServer/zone/aa.cpp::Client::GetAllToggleAAStatus`; `extras/THJServer/zone/aa.cpp::{GetToggleAAStatus, SetToggleAAStatus}` | `zone/aa.cpp` (no equivalent toggle-cache or `CharacterAaDisabledRepository` path) | Must Port | THJ persists and hydrates per-AA disabled state before bonus and AA-table evaluation; the local tree lacks that cache/repository path, so THJ toggle-AA behavior cannot be matched yet | `MC-FIX-03B` | Open |
| GAP-007 | Memorize-time bulk stat refresh | `extras/THJServer/zone/client_packet.cpp::Client::Handle_OP_MemorizeSpell` | `zone/client_packet.cpp::Client::Handle_OP_MemorizeSpell` | Validate First | THJ emits `SendBulkStatsUpdate()` after successful memorization and the local tree does not; this may be harmless now, but it is the remaining packet-side multiclass refresh delta and should be proven against spellbook, skills-window, and DLL presentation cases before dismissal | `MC-VAL-03` | Open |

---

## THJ-GAP-01 Scoped Non-Gaps

- `zone/client.cpp::GetClassesBits`, `LoadPersistedClassesBits`, `SetClassesBits`, and compatibility-class sync are already stricter than THJServer and should not be re-ported just because the file still differs.
- `zone/client.cpp::MaxSkill` plus `zone/client_process.cpp::{OPGMTrain, OPGMEndTraining, OPGMTrainSkill}` already use owned-class checks in places where THJ still has older `GetClass()`-centric branches; treat trainer and skill-cap behavior as validation-first, not missing-core-port work.
- `zone/client_mods.cpp::CalcBaseMana` already computes best-of-owned-classes mana using per-class base data and is not a regression relative to THJ's older helper split.
- `world/worlddb.cpp` already reads `GestaltClasses` directly for char-select shaping and deterministically picks a compatibility class from the owned mask; char-select remains a validation item, not a source-level core gap.

## Implementation Notes

- `2026-04-26` - `GAP-005`: `zone/aa.cpp::SendAlternateAdvancementTable()` now calls `LoadDynamicAATimers()` before rebuilding the AA table when `Custom:UseDynamicAATimers` is enabled. Follow-up runtime proof should target `AA-02` in [game_design/multiclass/TEST_TRACKER.md](game_design/multiclass/TEST_TRACKER.md).
- `2026-04-26` - `GAP-003`: `world/clientlist.cpp::GetMulticlassBitsOrBase()` now returns the legacy base-class bit immediately when `Custom:MulticlassingEnabled` is off, uses persisted multiclass ownership as-is after bucket lookup, and falls back from invalid zero masks to the compatibility/base class. Follow-up runtime proof should cover `CORE-01` and `UI-01` in [game_design/multiclass/TEST_TRACKER.md](game_design/multiclass/TEST_TRACKER.md).

---

## Gap Entry Template

```md
| GAP-### | Area | THJ file/function | Local file/function | Must Port / Validate First / Design Decision / Intentional Divergence / Ignore | Player/dev impact | Packet ID | Open |
```

---

## Agent Evidence Template

```md
Gap ID: GAP-###
Area: Multiclass AA
THJ reference: extras/THJServer/path/file.cpp:function
Local reference: path/file.cpp:function
Difference: concise behavioral delta
Classification: Must Port | Validate First | Design Decision | Intentional Divergence | Ignore
Why it matters: gameplay, stability, data, UX, or not relevant
Recommended next packet: THJ-GAP-xx / MC-FIX-xx / MC-VAL-xx
```
<!-- End of THJ gap register -->
