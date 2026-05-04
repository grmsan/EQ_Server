# Global Player Script
# Handles first-login welcome whisper and the Heroic Throw AA grant.

sub EVENT_SAY {
    if ($text =~/heroic throw/i && $class eq "Warrior") {
        if ($client->GrantAlternateAdvancementAbility(10000, 1, 1)) {
            $client->Message(15, "You have been granted the Heroic Throw ability!");
            quest::ding();
        } else {
            $client->Message(13, "Failed to grant Heroic Throw. You may already have it.");
        }
    }
}

# Fire once per character when they zone in for the very first time.
sub EVENT_ENTERZONE {
    my $seen = $client->GetBucket("welcome_whisper_sent");
    if (!$seen || $seen eq "0") {
        $client->SetBucket("welcome_whisper_sent", "1");
        quest::settimer("welcome_whisper", 3);
    }
}

sub EVENT_TIMER {
    if ($timer eq "welcome_whisper") {
        quest::stoptimer("welcome_whisper");
        $client->Message(
            261,
            "A distant whisper reaches your ears: " .
            "'Welcome to Norrath, " . $name . ". " .
            "The Bazaar awaits you. Seek the Bazaar Greeter there — " .
            "she will set your feet on the proper path.' " .
            "Use the teleport button in your inventory to travel there."
        );
    }
}
