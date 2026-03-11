# Waypoint Travel System — Job Aid

## Quick Summary

Players travel by hailing **Tearel** (NPC 990100) in the Bazaar.
He shows a chat menu of destinations grouped by continent.
Unlocked destinations are clickable saylinks; locked ones show as "undiscovered."

**Two types of waypoints:**

| Type | Default state | How unlocked |
|------|---------------|--------------|
| City/overworld | Unlocked for everyone | Rows in `thj_waypoints_default` |
| Dungeon | Locked | Player walks near an invisible **Faded Runestone** (NPC 999300) in the zone |

Unlocks are **account-wide** (stored in `thj_waypoints_account`).

## Key Files

| File | Purpose |
|------|---------|
| `quests/bazaar/Tearel.pl` | Travel menu — `@waypoints` array is the master destination list |
| `quests/global/a_faded_runestone.pl` | Proximity quest — auto-unlocks zone waypoint on approach |
| `quests/bazaar/player.pl` | Door 146 (Magic Map) — points player to Tearel |
| `utils/sql/install_waypoint_runestones.sql` | DB bootstrap for dungeon waypoints + NPC template |

## Database Tables

| Table | What it stores |
|-------|----------------|
| `thj_waypoints` | All destinations (id, shortname, long_name, category, x/y/z/heading) |
| `thj_waypoints_categories` | Category labels (0=Antonica, 1=Faydwer, 2=Odus, 3=Kunark, 4=Velious, 5=Luclin, 6=Planes, 7=Dungeons, 9=Utility) |
| `thj_waypoints_default` | Which waypoints are unlocked for all players (race_id=65535, class_mask=65535 = everyone) |
| `thj_waypoints_account` | Per-account unlocks (written by `UnlockWaypoint()`) |

## How To: Add a New Waypoint

### 1. Insert the waypoint row

```sql
INSERT INTO thj_waypoints (id, shortname, long_name, category, x, y, z, heading)
VALUES (36, 'zoneshort', 'Display Name', 7, 100.0, 200.0, 5.0, 0);
```

- `shortname` must match the zone's `short_name` in the `zone` table
- `category` must be a valid id from `thj_waypoints_categories`
- Coordinates = where Tearel teleports the player (usually zone safe point)
- Find a zone's safe coords: `SELECT safe_x, safe_y, safe_z FROM zone WHERE short_name = 'zoneshort';`

### 2. Add to Tearel's menu

Open `quests/bazaar/Tearel.pl` and add an entry to the `@waypoints` array under the correct category:

```perl
{ cat => "Dungeons", short => "zoneshort", long => "Display Name", zid => 99, x => 100, y => 200, z => 5, h => 0 },
```

- `zid` = zone id number from the `zone` table
- Keep entries sorted by category, then alphabetically

### 3. If discoverable (dungeon-style): place a runestone

In-game as a GM:

```
#zone zoneshort
```
Walk to the desired trigger location (near zone entrance).
```
#npcspawn create 999300
```
Done. The invisible NPC saves at your location. Proximity range is 200 units in each direction (generous).

### 4. If default-unlocked (city-style): add to defaults table

```sql
INSERT INTO thj_waypoints_default (waypoint_id, race_id, class_mask)
VALUES (36, 65535, 65535);
```

### 5. Reload quests

```
#reloadquests
```

## How To: Move a Runestone

In-game in the zone:

```
#npcspawn list                     -- find the spawn
```
Walk to the new position.
```
#npcedit setloc                    -- updates spawn2 coordinates
#reloadquests                      -- re-register proximity at new location
```

## How To: Change Teleport Landing Coordinates

Two places to update:

1. **Database:** `UPDATE thj_waypoints SET x=?, y=?, z=? WHERE shortname='zoneshort';`
2. **Tearel.pl:** Update the matching entry in `@waypoints`

The DB row is used by the C++ `PromptWaypointTransport` path; Tearel.pl uses its own hardcoded coords for `quest::movepc()`.

## How To: Add a New Category

```sql
INSERT INTO thj_waypoints_categories (id, name) VALUES (8, 'New Category');
```

Then use `cat => "New Category"` in Tearel.pl entries. Categories display in the order they appear in the `@waypoints` array.

## How To: GM-Unlock a Waypoint for Testing

```
#wp unlock zoneshort               -- unlocks for your account
#wp lock zoneshort                  -- re-locks it
```

## DLL Waypoint UI POC Commands

If using the custom `eq-core-dll-main` waypoint POCs:

```
#wppoc list
#wppoc travel <waypoint_id>
#wppoc expedition
```

- `#wppoc list` sends `OP_WaypointList` to client.
- `#wppoc travel` routes directly to `TransportToWaypoint()` and still enforces unlock checks.
- Client DLL command `/waypointpoc` opens the SIDL waypoint window and uses these server commands under the hood.
- Client DLL command `/waypointoverlay` drives a HUD overlay POC (no SIDL, no ImGui) using the same `#wppoc` bridge.
- Client DLL command `/waypointimgui` drives the Lua/ImGui scaffold POC command surface using the same `#wppoc` bridge.

Or via Perl in a quest script:

```perl
$client->UnlockWaypoint("zoneshort");
```

---

## Architecture Details

### Player Flow

```
Player hails Tearel
  → Tearel.pl EVENT_SAY iterates @waypoints
  → For each: calls $client->IsWaypointUnlocked(shortname)
  → Unlocked = clickable saylink | Locked = grey "(undiscovered)"
  → Player clicks a saylink (e.g. "travel_gukbottom")
  → EVENT_SAY matches /^travel_(\w+)$/
  → Validates unlock status server-side
  → quest::movepc() teleports player
```

### Discovery Flow

```
Player enters dungeon zone
  → Walks within 200 units of Faded Runestone NPC
  → a_faded_runestone.pl EVENT_ENTER fires
  → Calls $client->IsWaypointUnlocked($zonesn)
  → If not unlocked: $client->UnlockWaypoint($zonesn)
  → Verifies unlock succeeded (zone must have a thj_waypoints row)
  → Shows attunement message + spell visual effect
  → Subsequent visits: silent (already unlocked)
```

### C++ Backing (zone/client.cpp)

The Perl methods call into C++:

| Perl Call | C++ Method | What It Does |
|-----------|-----------|--------------|
| `$client->IsWaypointUnlocked("x")` | `Client::IsWaypointUnlocked()` | Checks `thj_waypoints_default` then `thj_waypoints_account` |
| `$client->UnlockWaypoint("x")` | `Client::UnlockWaypoint()` | Inserts into `thj_waypoints_account` for the account |

Unlock checks cascade: default table first (everyone gets cities), then account table (per-account dungeon discoveries).

### NPC Template: a_faded_runestone (999300)

| Property | Value | Why |
|----------|-------|-----|
| Race | 127 | Invisible Man — no model needed, works in all zones |
| Bodytype | 11 | Untargetable intractable object |
| Level | 1 | Minimal |
| Name | a_faded_runestone | Shows on target (barely matters since invisible + untargetable) |
| Lastname | Remnant of the Combine | Flavor |

### Current Waypoint List

**Default-unlocked (cities):** West Freeport, North Qeynos, Halas, Neriak, Grobb, Oggok, Rivervale, East Commonlands, Greater Faydark, N. Felwithe, S. Kaladim, Ak'anon, Erudin, East Cabilis, Shar Vahl, Bazaar, Plane of Knowledge

**Default-locked (overworld):** Field of Bone, Dreadlands, Iceclad Ocean

**Discoverable (dungeons):** Befallen, Najena, Permafrost, Sol A, Sol B, Lower Guk, Unrest, The Hole, Crushbone, Mistmoore, Kedge Keep, Sebilis, Chardok, Karnor's Castle, Kael Drakkel
