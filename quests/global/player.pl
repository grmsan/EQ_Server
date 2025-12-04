# Global Player Script - Grant Heroic Throw AA
# This script grants the Heroic Throw AA when a warrior says "heroic throw"

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
