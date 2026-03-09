# ============================================================
# Tearel — Keeper of the Map (Bazaar, NPC 990100)
# Chat-based waypoint travel menu.
# Hail shows unlocked destinations grouped by continent.
# Player clicks a saylink to teleport (with confirmation).
# Uses $client->IsWaypointUnlocked() — account-based.
# ============================================================

# Waypoint definitions: [shortname, long_name, zone_id, x, y, z, heading]
# These must match thj_waypoints table rows.
my @waypoints = (
    # -- Antonica --
    { cat => "Antonica",  short => "freportw",    long => "West Freeport",       zid => 9,   x => -116, y => -142, z => -6,   h => 140 },
    { cat => "Antonica",  short => "qeynos2",     long => "North Qeynos",        zid => 2,   x => 370,  y => 160,  z => 5,    h => 180 },
    { cat => "Antonica",  short => "halas",        long => "Halas",               zid => 29,  x => 0,    y => 0,    z => 3,    h => 0   },
    { cat => "Antonica",  short => "neriakb",      long => "Neriak Commons",      zid => 41,  x => -42,  y => 210,  z => -3,   h => 384 },
    { cat => "Antonica",  short => "grobb",        long => "Grobb",               zid => 52,  x => -450, y => 100,  z => 4,    h => 256 },
    { cat => "Antonica",  short => "oggok",        long => "Oggok",               zid => 49,  x => -99,  y => -22,  z => 3,    h => 128 },
    { cat => "Antonica",  short => "rivervale",    long => "Rivervale",           zid => 19,  x => -180, y => -220, z => 4,    h => 256 },
    { cat => "Antonica",  short => "ecommons",     long => "East Commonlands",    zid => 22,  x => -1700,y => -300, z => 3,    h => 128 },
    # -- Faydwer --
    { cat => "Faydwer",   short => "gfaydark",     long => "Greater Faydark",     zid => 54,  x => -511, y => 55,   z => 0,    h => 128 },
    { cat => "Faydwer",   short => "felwithea",    long => "Northern Felwithe",   zid => 61,  x => 120,  y => 280,  z => 13,   h => 128 },
    { cat => "Faydwer",   short => "kaladima",     long => "South Kaladim",       zid => 60,  x => 10,   y => -20,  z => 5,    h => 128 },
    { cat => "Faydwer",   short => "akanon",       long => "Ak'anon",             zid => 55,  x => -761, y => 1279, z => -24,  h => 182 },
    # -- Odus --
    { cat => "Odus",      short => "erudnext",     long => "Erudin",              zid => 24,  x => -240, y => -1216,z => 52,   h => 510 },
    # -- Kunark --
    { cat => "Kunark",    short => "cabeast",      long => "East Cabilis",        zid => 106, x => 10,   y => 10,   z => 3,    h => 256 },
    { cat => "Kunark",    short => "fieldofbone",  long => "The Field of Bone",   zid => 78,  x => 1617, y => -1691,z => -45,  h => 10  },
    { cat => "Kunark",    short => "dreadlands",   long => "Dreadlands",          zid => 86,  x => 9722, y => 1136, z => 2626, h => 0   },
    # -- Velious --
    { cat => "Velious",   short => "iceclad",      long => "Iceclad Ocean",       zid => 110, x => 350,  y => 5300, z => -5,   h => 190 },
    # -- Luclin --
    { cat => "Luclin",    short => "sharvahl",     long => "Shar Vahl",           zid => 155, x => 240,  y => 35,   z => 3,    h => 256 },
    # -- Planes --
    { cat => "Planes",    short => "poknowledge",  long => "Plane of Knowledge",  zid => 202, x => 830,  y => 575,  z => -64,  h => 128 },
    # -- Dungeons (unlocked by discovering Faded Runestones in each zone) --
    { cat => "Dungeons",  short => "befallen",     long => "Befallen",            zid => 36,  x => 35,   y => -82,  z => 3,    h => 0   },
    { cat => "Dungeons",  short => "najena",       long => "Najena",              zid => 44,  x => 858,  y => -76,  z => 4,    h => 0   },
    { cat => "Dungeons",  short => "permafrost",   long => "Permafrost Caverns",  zid => 73,  x => 61,   y => -121, z => 2,    h => 0   },
    { cat => "Dungeons",  short => "soldunga",     long => "Solusek's Eye",       zid => 31,  x => -486, y => -476, z => 73,   h => 0   },
    { cat => "Dungeons",  short => "soldungb",     long => "Nagafen's Lair",      zid => 32,  x => -263, y => -424, z => -108, h => 0   },
    { cat => "Dungeons",  short => "gukbottom",    long => "The Ruins of Old Guk",zid => 66,  x => -217, y => 1197, z => -78,  h => 0   },
    { cat => "Dungeons",  short => "unrest",       long => "Estate of Unrest",    zid => 63,  x => 52,   y => -38,  z => 3,    h => 0   },
    { cat => "Dungeons",  short => "hole",         long => "The Hole",            zid => 39,  x => -1050,y => 640,  z => -80,  h => 0   },
    { cat => "Dungeons",  short => "crushbone",    long => "Crushbone",           zid => 58,  x => 158,  y => -644, z => 4,    h => 0   },
    { cat => "Dungeons",  short => "mistmoore",    long => "Castle Mistmoore",    zid => 59,  x => 120,  y => -330, z => -178, h => 0   },
    { cat => "Dungeons",  short => "kedge",        long => "Kedge Keep",          zid => 64,  x => 14,   y => 100,  z => 302,  h => 0   },
    { cat => "Dungeons",  short => "sebilis",      long => "Ruins of Sebilis",    zid => 89,  x => 0,    y => 250,  z => 44,   h => 0   },
    { cat => "Dungeons",  short => "chardok",      long => "Chardok",             zid => 103, x => 859,  y => 119,  z => 106,  h => 0   },
    { cat => "Dungeons",  short => "karnor",       long => "Karnor's Castle",     zid => 102, x => 302,  y => 18,   z => 6,    h => 0   },
    { cat => "Dungeons",  short => "kael",         long => "Kael Drakkel",        zid => 113, x => -633, y => -47,  z => 128,  h => 0   },
    # -- Utility --
    { cat => "Utility",   short => "bazaar",       long => "The Bazaar",          zid => 151, x => 20,   y => -15,  z => 0.72, h => 256 },
);

sub EVENT_SAY {
    # --- HAIL: show travel menu ---
    if ($text =~ /hail/i) {
        plugin::NPCTell(
            "Greetings, $name. I am Tearel, the Keeper of the Map. " .
            "I can transport you to any location you have discovered. " .
            "Simply choose a destination from the list below."
        );

        my $current_cat = "";
        my $has_entries = 0;

        foreach my $wp (@waypoints) {
            # Print category header when it changes
            if ($wp->{cat} ne $current_cat) {
                $current_cat = $wp->{cat};
                $client->Message(335, "--- $current_cat ---");
            }

            my $is_here = ($wp->{short} eq $zonesn);
            my $unlocked = $client->IsWaypointUnlocked($wp->{short});

            if ($is_here) {
                $client->Message(271, "  " . $wp->{long} . " [You Are Here]");
            }
            elsif ($unlocked) {
                my $link = quest::saylink("travel_" . $wp->{short}, 1, $wp->{long});
                $client->Message(258, "  $link");
                $has_entries = 1;
            }
            else {
                $client->Message(271, "  " . $wp->{long} . " (undiscovered)");
            }
        }

        if (!$has_entries) {
            $client->Message(335, "You have not yet discovered any travel destinations.");
        }

        $client->Message(335, "---");
        $client->Message(271, "Explore the world to discover new destinations.");
        return;
    }

    # --- TRAVEL: handle destination clicks ---
    if ($text =~ /^travel_(\w+)$/i) {
        my $dest_short = $1;

        # Find the waypoint
        my $wp = undef;
        foreach my $w (@waypoints) {
            if ($w->{short} eq $dest_short) {
                $wp = $w;
                last;
            }
        }

        if (!$wp) {
            plugin::NPCTell("I don't recognize that destination.");
            return;
        }

        if (!$client->IsWaypointUnlocked($wp->{short})) {
            plugin::NPCTell("You have not yet discovered that location. Explore the world to attune yourself to new destinations.");
            return;
        }

        if ($wp->{short} eq $zonesn) {
            plugin::NPCTell("You are already here, $name.");
            return;
        }

        plugin::NPCTell("Safe travels, $name. Transporting you to " . $wp->{long} . ".");
        quest::movepc($wp->{zid}, $wp->{x}, $wp->{y}, $wp->{z}, $wp->{h});
        return;
    }
}

sub EVENT_ITEM {
    plugin::return_items(\%itemcount);
}