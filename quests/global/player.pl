# Global Player Script
# Handles first-login welcome whisper and Heroic Throw AA grant.

my $BAZAAR_INTRO_TASK_ID = 600100;

sub maybe_assign_bazaar_intro_task {
    if (!$client->IsTaskCompleted($BAZAAR_INTRO_TASK_ID) && !$client->IsTaskActive($BAZAAR_INTRO_TASK_ID)) {
        $client->AssignTask($BAZAAR_INTRO_TASK_ID);
    }
}

sub EVENT_SAY {
    if ($text =~ /heroic throw/i && $class eq "Warrior") {
        if ($client->GrantAlternateAdvancementAbility(10000, 1, 1)) {
            $client->Message(15, "You have been granted Heroic Throw ability!");
            quest::ding();
        } else {
            $client->Message(13, "Failed to grant Heroic Throw. You may already have it.");
        }
    }
}

# Fire once per character on their first zone-in.
sub EVENT_ENTERZONE {
    my $seen = $client->GetBucket("welcome_whisper_sent");
    if (!$seen || $seen eq "0") {
        maybe_assign_bazaar_intro_task();
        $client->SetBucket("welcome_whisper_sent", "1");
        quest::settimer("welcome_whisper", 3);
    }
}

sub EVENT_TIMER {
    if ($timer eq "welcome_whisper") {
        my $name = $client->GetCleanName();
        quest::stoptimer("welcome_whisper");
        $client->Message(
            261,
            "A distant whisper reaches your ears: " .
            "'Welcome to Norrath, " . $name . ". " .
            "A task has been added to your journal. Seek the Bazaar Greeter in the Bazaar. " .
            "She will explain our three-class path and send you to the Emissary of the Guilds " .
            "to choose two more permanent classes.' " .
            "Use the teleport button in your inventory to travel there."
        );
    }
}
