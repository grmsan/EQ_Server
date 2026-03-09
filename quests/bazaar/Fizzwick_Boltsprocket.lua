-- ============================================================
-- Fizzwick Boltsprocket — Salvage Master (Bazaar)
-- A quirky gnome who teaches players the Salvage system.
-- He's an independent operator, eccentric and secretive,
-- slightly paranoid about the "Augmenteers" finding out his methods.
-- ============================================================

local bazaar_npcs = {
    "Gearo",
    "Faeroi",
    "Caerlyna",
    "Sateal Deirosap",
    "Eryke Stremstin",
    "Ward Jermain",
    "Jolum",
}

local function random_npc()
    return bazaar_npcs[math.random(#bazaar_npcs)]
end

-- Paranoid asides Fizzwick mutters between sentences
local asides = {
    "*glances over shoulder*",
    "*taps nose conspiratorially*",
    "*lowers voice to a whisper*",
    "*fidgets with a loose spring in his pocket*",
    "*peers around nervously*",
    "*adjusts goggles that aren't there*",
}

local function aside()
    return asides[math.random(#asides)]
end

function event_say(e)
    -- HAIL
    if e.message:findi("hail") then
        local npc = random_npc()
        e.self:Say(
            "Ah HA! A visitor! Come closer, come closer. " ..
            "The name's Fizzwick. Fizzwick Boltsprocket. " ..
            "You look like someone who accumulates a LOT of junk. " ..
            "Am I right? Of course I'm right. I can always tell. " ..
            aside() .. " " ..
            "I deal in the art of [salvage] — the REFINED art, mind you, " ..
            "not the brute-force nonsense those pompous [Augmenteers] peddle. " ..
            "But don't tell " .. npc .. " I said that. " ..
            "That one can NEVER know what I do here. NEVER."
        )

    -- SALVAGE (main topic)
    elseif e.message:findi("salvage") then
        e.self:Say(
            "Yes, yes, salvage! The most elegant discipline in all of Norrath, " ..
            "if you ask me — and you DID ask me, so there. " ..
            aside() .. " " ..
            "Here's the secret: every magic item in this world is held together " ..
            "by essence. Pure, crystallized potential! When you no longer need " ..
            "an item, you don't just THROW it away like some barbarian. " ..
            "You break it down. You extract the [essence]. " ..
            "Would you like to know [how it works]?"
        )

    -- HOW IT WORKS
    elseif e.message:findi("how it works") then
        e.self:Say(
            "Simple! Well, simple for a genius. Moderately complex for everyone else. " ..
            aside() .. " " ..
            "Step one: grab a Salvage Satchel from my inventory. " ..
            "Step two: stuff any MAGIC items you don't want inside. " ..
            "Step three: click the Combine button. " ..
            "WHOOSH! The items dissolve and you get essence deposited " ..
            "directly into your Alternate Currency balance. " ..
            "Check your inventory window — there's an Alternate Currency tab. " ..
            "The more powerful the item, the more [essence] you get. " ..
            "And sometimes — rarely, mind you — you might even get [Rare Essence]."
        )

    -- ESSENCE
    elseif e.message:findi("essence") and not e.message:findi("rare") then
        e.self:Say(
            "Common Essence is the lifeblood of progress around here. " ..
            "You can spend it at those snooty [Augmenteers] across the way — " ..
            "much as it pains me to admit they sell ANYTHING useful. " ..
            aside() .. " " ..
            "The stronger the magic item you salvage, the more essence you extract. " ..
            "Little rusty dagger? Pfah! Not even worth the satchel space. " ..
            "Has to be MAGIC. No magic, no essence. I don't make the rules. " ..
            "Well, actually, I sort of DO, but that's besides the point. " ..
            "There's also [Rare Essence] if you're lucky."
        )

    -- RARE ESSENCE
    elseif e.message:findi("rare essence") or e.message:findi("rare") then
        local npc = random_npc()
        e.self:Say(
            "Oho! Now you're asking the RIGHT questions! " ..
            aside() .. " " ..
            "Rare Essence is... special. You can't just grind it out of any old " ..
            "sword. It appears sometimes when you salvage particularly powerful " ..
            "items — gear from named creatures, raid bosses, that sort of thing. " ..
            "There's always a small chance, but the mightier the foe that " ..
            "dropped it, the better your odds. " ..
            "Some of the [Augmenteers] trade in the stuff — not that I'd " ..
            "give them the satisfaction of a referral. " ..
            "Don't tell " .. npc .. " I sent you. " ..
            "I have a reputation to maintain."
        )

    -- SATCHEL
    elseif e.message:findi("satchel") then
        e.self:Say(
            "Right here, right here! My pride and joy. " ..
            aside() .. " " ..
            "Open it up, stuff in all those magic items collecting dust " ..
            "in your bags, and give that Combine button a good hard click. " ..
            "I also sell a Purified Solvent — handy little thing. " ..
            "If you've got an augment stuck in a piece of gear and want it back, " ..
            "the solvent pops it right out, clean as a whistle. " ..
            "No damage, no fuss. Unlike SOME people's methods. " ..
            "I'm looking at you, Forgemaster."
        )

    -- AUGMENTEERS (the aug merchants)
    elseif e.message:findi("augmenteer") then
        local npc = random_npc()
        e.self:Say(
            "Oh, THEM. The 'Grand Order of Augmenteers.' " ..
            aside() .. " " ..
            "Big fancy title for four people who glue rocks to swords. " ..
            "Don't get me wrong — augments are useful. I SUPPOSE. " ..
            "But they act like they invented the concept of making things better. " ..
            "Last week the Provisioner told me my salvage work was " ..
            "'crude and unrefined.' UNREFINED! Me! A Boltsprocket! " ..
            "My family has been taking things apart since before their " ..
            "'Grand Order' could spell 'augmentation.' " ..
            "Anyway. If you DO need augments, they're right over there. " ..
            "But you didn't hear it from me. And DEFINITELY don't tell " ..
            npc .. "."
        )
    end
end

function event_spawn(e)
    -- Fizzwick occasionally mutters to himself
    eq.set_timer("mutter", 180000) -- every 3 minutes
end

function event_timer(e)
    if e.timer == "mutter" then
        local mutters = {
            "Hmm, now where did I put that spring...",
            "Essence extraction efficiency is up three percent this week...",
            "Those Augmenteers think they're SO clever with their fancy rocks...",
            "Note to self: do NOT mix fire essence with cold essence again.",
            "I wonder if " .. random_npc() .. " suspects anything...",
            "Salvage, salvage, salvage. It's a beautiful word, really.",
            "*tinkers with something that sparks*",
            "Patent pending, patent pending, ALL of this is patent pending...",
        }
        e.self:Say(mutters[math.random(#mutters)])
    end
end
