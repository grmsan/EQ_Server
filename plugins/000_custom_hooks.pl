sub CustomEventSayEntry {
    return 0;
}

sub CustomEventNPCDeathEntry {
    return 0;
}

sub CustomEventHandinEntry {
    return 0;
}

sub CustomEventNPCSpawnEntry {
    return 0;
}

sub IsTHJ {
    return 0;
}

sub MultiClassingEnabled {
    return 0;
}

sub GetClassBitmask {
    return 0;
}

sub GetClassesCount {
    return 1;
}

sub AddClass {
    # No-op
}

sub NPCTell {
    my $text = shift;
    quest::say($text);
}

sub ScaleInstanceNPC {
    # No-op
}

sub LootEOM {
    # No-op
}

sub ProcessSlayerCredit {
    # No-op
}

sub CustomEventExpGainEntry { return 0; }
sub CustomEventAAExpGainEntry { return 0; }
sub CustomEventItemEquipEntry { return 0; }
sub CustomEventItemUnequipEntry { return 0; }
sub CustomEventDestroyEntry { return 0; }
sub CustomEventItemClickCastEntry { return 0; }

# The error was &main::eval_file, so we define it in the main package
package main;

open(my $fh, '>', 'plugin_debug.txt');
print $fh "Plugin loaded\n";
close $fh;

sub eval_file {
    my($package, $filename) = @_;
    $filename =~ s/\'//g;

    if(! -r $filename) {
        # quest::debug("Unable to read perl file '$filename'");
        return;
    }

    # Standard require to load the file into the specified package
    eval "package $package; require './$filename';";

    if ($@) {
        # quest::debug("Error loading $filename: $@");
    }
    return 1;
}

package plugin;

return 1;
