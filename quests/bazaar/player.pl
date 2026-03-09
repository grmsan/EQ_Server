sub EVENT_CLICKDOOR {
    if ($doorid == 146) { # Magic Map
        $client->Message(335, "The map glows faintly. Speak to Tearel nearby to travel.");
    }    
}