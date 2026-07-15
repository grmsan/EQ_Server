my $TASK_ID = 600100;

sub EVENT_SPAWN {
    quest::settimer("appearance", 1);
}

sub EVENT_TIMER {
    if ($timer eq "appearance") {
        quest::stoptimer("appearance");
        $npc->SetAppearance(1);
        return;
    }
}

sub maybe_assign_task {
    if (!$client->IsTaskCompleted($TASK_ID) && !$client->IsTaskActive($TASK_ID)) {
        $client->AssignTask($TASK_ID, $npc->GetNPCTypeID());
    }
}

sub class_count {
    return plugin::GetClassesCount($client);
}

sub update_task_activity {
    my ($activity_id) = @_;
    if ($client->IsTaskActivityActive($TASK_ID, $activity_id)) {
        $client->UpdateTaskActivity($TASK_ID, $activity_id, 1);
    }
}

sub send_home {
    my %start_points = (
        1   => { zone => 2, x => -51, y => 428, z => 3, h => 384, name => "North Qeynos" },
        2   => { zone => 30, x => -682, y => 3139, z => -60, h => 384, name => "Everfrost Peaks" },
        3   => { zone => 38, x => -916, y => -1510, z => -33, h => 0, name => "Toxxulia Forest" },
        4   => { zone => 54, x => 10, y => -20, z => 0, h => 128, name => "Greater Faydark" },
        5   => { zone => 54, x => 10, y => -20, z => 0, h => 128, name => "Greater Faydark" },
        6   => { zone => 25, x => -965, y => 1838, z => 20, h => 128, name => "Nektulos Forest" },
        7   => { zone => 2, x => -51, y => 428, z => 3, h => 384, name => "North Qeynos" },
        8   => { zone => 68, x => -213, y => 2795, z => 3, h => 128, name => "Butcherblock Mountains" },
        9   => { zone => 46, x => -588, y => -2192, z => -25, h => 128, name => "Innothule Swamp" },
        10  => { zone => 47, x => 906, y => 1043, z => 25, h => 128, name => "The Feerrott" },
        11  => { zone => 33, x => -770, y => 226, z => 4, h => 128, name => "Misty Thicket" },
        12  => { zone => 56, x => -272, y => 159, z => -21, h => 128, name => "Steamfont Mountains" },
        128 => { zone => 78, x => 1617, y => -1684, z => -54, h => 128, name => "Field of Bone" },
        130 => { zone => 165, x => -301, y => -1822, z => -27, h => 128, name => "Shadeweaver Thicket" },
        330 => { zone => 50, x => 421, y => 1185, z => 4, h => 128, name => "Rathe Mountains" },
        522 => { zone => 394, x => -1520, y => -1470, z => -86, h => 128, name => "Crescent Reach" },
    );

    my $race = $client->GetRace();
    my $start = $start_points{$race} || $start_points{1};
    quest::say("Good. Your trio is set. I will send you to " . $start->{name} . " now. Use Bazaar and Back whenever you need to return here.");
    $client->MovePC($start->{zone}, $start->{x}, $start->{y}, $start->{z}, $start->{h});
}

sub EVENT_SAY {
    if ($text =~ /hail/i) {
        maybe_assign_task();
        update_task_activity(0);

        if (class_count() >= 3) {
            update_task_activity(3);
            quest::say("Your trio is " . plugin::GetPrettyClassString($client) . ". If you are ready to leave the Bazaar, tell me [send me home].");
            return;
        }

        if (class_count() == 2) {
            quest::say("You have chosen " . plugin::GetPrettyClassString($client) . ". Good. Speak with one more guild master around the Bazaar to complete your trio, then return to me.");
            return;
        }

        quest::say("Welcome, $name. Walk around the Bazaar and speak with the guild masters to choose two more permanent classes. If you ever drop the task before finishing it, hail me again and I will reissue it.");
        return;
    }

    if ($text =~ /send me home/i) {
        maybe_assign_task();
        update_task_activity(3);

        if (class_count() < 3) {
            quest::say("Not yet. Walk the Bazaar and speak with the guild masters until your three-class trio is complete.");
            return;
        }

        send_home();
    }
}
