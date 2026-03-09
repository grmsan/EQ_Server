# ============================================================
# a_faded_runestone — Global proximity waypoint attunement
# NPC ID: 999300  (invisible, placed near dungeon entrances)
#
# When a player walks within range, the runestone checks if
# the current zone has a matching waypoint. If so, it unlocks
# that waypoint on the player's account via UnlockWaypoint().
#
# Place one per dungeon zone using GM commands:
#   #npcspawn create 999300
#   (move to desired location, then #npcedit setloc)
# ============================================================

my $PROX_RANGE = 200;   # generous — ~400 unit square

sub EVENT_SPAWN {
    my $cx = $npc->GetX();
    my $cy = $npc->GetY();
    quest::set_proximity($cx - $PROX_RANGE, $cx + $PROX_RANGE,
                         $cy - $PROX_RANGE, $cy + $PROX_RANGE);
}

sub EVENT_ENTER {
    # Only fire for players (not bots/mercs)
    return if !$client;

    my $zone = $zonesn;

    # Check if this zone has a waypoint to unlock
    if ($client->IsWaypointUnlocked($zone)) {
        # Already attuned — do nothing (silent)
        return;
    }

    # Attempt to unlock
    $client->UnlockWaypoint($zone);

    # Verify it took (the zone might not have a matching waypoint row)
    if ($client->IsWaypointUnlocked($zone)) {
        # Success — give the player feedback
        $client->Message(335, "A faint hum rises from the stones beneath your feet.");
        $client->Message(258, "You have attuned yourself to this location. Tearel can now transport you here.");

        # Visual feedback: brief spell effect on pc (2644 = shimmering portal effect)
        $client->SpellEffect(43, 10);
    }
}
