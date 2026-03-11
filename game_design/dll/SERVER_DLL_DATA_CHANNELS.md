# Server <-> DLL Data Channels

Last updated: 2026-03-10

## Goal
Document the transport channels currently used between the EQEmu server and the custom client DLL so feature work can reuse stable paths.

## Channel Summary

| Channel | Direction | Wire ID | Format | Server Source | DLL Sink | Status |
|---|---|---|---|---|---|---|
| EdgeStatLabel | Server -> DLL | raw opcode `0x1338` | `uint32 count` + repeated `{uint32 key, uint64 value}` | `Client::SendEdgeStats()` / `Client::SendEdgeTestProbe()` in `zone/client.cpp` | `ApplyEdgeStatLabelPacket()` in `extras/eq-core-dll-main/src/eqgame.cpp` | Active |
| ServerStatsUpdate | Server -> DLL | `OP_ServerStatsUpdate` mapped to `0x7330` for RoF2 | fixed struct (`ServerStatsUpdate_Struct`) | `Client::SendServerStatsUpdate()` in `zone/client.cpp` | parsed in `HandleWorldMessage_Detour()` in `eqgame.cpp` | Active |
| Custom item tail | Server -> DLL | item packet append marker `0x1337C0DE` | marker + pair count + key/value tuples | append in `common/patches/rof2.cpp` | parse/strip in `HandleWorldMessage_Detour()` in `eqgame.cpp` | Active (apply logic pending) |
| Probe callback v1/v2 | DLL -> Server | chat command (`/say #test ...`) | command args | emitted in `eqgame.cpp` | parsed in `zone/gm_commands/test.cpp` | Diagnostics only |
| Waypoint list | Server -> DLL | `OP_WaypointList` (`0x1402` RoF2) | waypoint list struct + entries | `Client::SendWaypointList()` in `zone/client.cpp` | parsed by `WaypointPOCWnd_OnWaypointListPacket()` in `WaypointPOCWnd.cpp` | Active |
| Waypoint action (POC) | DLL -> Server | `#wppoc` say-command bridge | text command args | emitted by `WaypointPOCWnd`, `WaypointOverlayPOC`, and `WaypointLuaImGuiPOC` (`/say #wppoc ...`) | handled by `zone/gm_commands/wppoc.cpp` | Active POC |

## EdgeStatLabel (`0x1338`) Details

### Payload layout
- `uint32 count`
- repeat `count` times:
  - `uint32 key`
  - `uint64 value`

### Current key ranges
- Core stat keys:
  - `2..7` HP/Mana/End current+max
  - `24..30` STR/STA/DEX/AGI/INT/WIS/CHA
  - `200` multiclass bitmask
- PowerSlotWnd POC keys:
  - `300..343`
- Probe/test keys:
  - `900..908`

### Send timing notes
EdgeStat snapshots are intentionally re-sent after connect/zone to handle client timing while DLL detours arm:
- `zone/client_packet.cpp` (connect + zone complete)
- `zone/client_process.cpp` (retry timer)

## ServerStatsUpdate (`0x7330`) Details

### Struct
Defined in `common/eq_packet_structs.h` as `ServerStatsUpdate_Struct`, includes:
- spawn id
- STR/STA/AGI/DEX/INT/WIS/CHA
- current + max HP/Mana/Endurance

### Rule and mapping gates
- Rule gate: `Character:EnableServerStatsUpdate` in `common/ruletypes.h`.
- Opcode must exist in patch map:
  - `utils/patches/patch_RoF2.conf` has `OP_ServerStatsUpdate=0x7330`.

## Item Custom Data Marker (`0x1337C0DE`)

### Server side
- In `common/patches/rof2.cpp`, item packet builder appends custom per-instance pairs when `Items:SendCustomItemStatsToClient` is enabled.

### DLL side
- `eqgame.cpp` scans trailing bytes, parses marker block, logs pairs, strips appended tail before handing packet to the client.
- Current implementation is parse/log/strip; direct in-memory item scaling is still TODO.

## Lua-Based Packet Sending (Server Script Layer)

Server Lua already supports raw/custom opcode packet creation:
- `zone/lua_packet.cpp`:
  - `Packet(opcode, size, true)` creates raw packet and uses `SetOpcodeBypass(opcode)`.
- `zone/lua_client.cpp`:
  - `Client:QueuePacket(packet, ...)` sends packet to client.

This means scripted server features can send `0x1338` payloads without C++ changes.

## Recommended Channel Conventions (Going Forward)

### Keyspace governance for `0x1338`
- Reserve key blocks by feature to avoid collisions:
  - `0-255`: core stats/system
  - `256-511`: class/multiclass and profile metadata
  - `512-1023`: UI controls/state
  - `1024+`: feature modules (`bags`, `waypoints`, `travel`, etc.)

### Move production actions off chat callbacks
- Keep `/say #test ...` only for diagnostics.
- For production UI actions, define a dedicated custom action opcode with:
  - `feature_id`
  - `action_id`
  - `request_id`
  - payload bytes
- Require server-side validation for all action requests.

### Compatibility model
- If DLL is absent or older, server should continue normal legacy flow.
- Treat custom channels as optional overlays, not hard dependencies.
