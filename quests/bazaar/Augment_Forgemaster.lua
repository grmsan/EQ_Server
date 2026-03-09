-- ============================================================
-- Augment Forgemaster (181203) — Augment Merging vendor
-- Member of the Grand Order of Augmenteers.
-- Sells merge catalysts, Augment Forge, Infusion Pool for Common Essence.
-- Gruff, practical, hands-on craftsman. The "blue collar" member.
-- Dialogue focuses on MECHANICS, not specific item names/prices.
-- ============================================================

-- Randomized denial responses when asked about Fizzwick/salvage
local denials = {
    "Who? Never heard of 'em. " ..
    "Look, I keep my head down and tend the forges. " ..
    "I don't keep track of every person in the Bazaar.",

    "Salvage? Is that some kind of cooking technique? " ..
    "I only know augments. You want augment help, I'm your guy. " ..
    "Otherwise, I can't help you.",

    "A gnome? Doing what to items? " ..
    "Sounds made up. You feeling alright? " ..
    "Maybe sit down for a minute. Want to talk about [merging] instead?",

    "Nope. Doesn't ring a bell. Not even a little one. " ..
    "Now, did you need a forge or a catalyst? " ..
    "Because THAT I can help with.",

    "*scratches head* " ..
    "Can't say the name means anything to me. " ..
    "I mostly just stare at forges all day. Don't get out much.",
}

function event_say(e)
    -- GNOME / SALVAGE / FIZZWICK DENIAL
    if e.message:findi("gnome") or e.message:findi("fizzwick") or
       e.message:findi("salvage") or e.message:findi("satchel") or
       e.message:findi("boltsprocket") then
        e.self:Say(denials[math.random(#denials)])
        return
    end

    -- HAIL
    if e.message:findi("hail") then
        e.self:Say(
            "Greetings. I'm the Forgemaster. I deal in the practical side " ..
            "of augmentation — the [merging], the [infusion], the actual " ..
            "getting-things-done part while the others write treatises about it. " ..
            "Need an Augment Forge? Need [catalysts]? " ..
            "Want to know about [infusion]? I'm your guy. " ..
            "Want a lecture? Go talk to the Provisioner."
        )

    -- MERGING (detailed how-to)
    elseif e.message:findi("merg") then
        e.self:Say(
            "Right, here's how merging works. No fancy words, just the steps. " ..
            "First: buy an Augment Forge from me. It's a combine container. " ..
            "Second: get three augments of the SAME type and SAME level. " ..
            "Whatever you've got, as long as they match exactly. " ..
            "Third: grab the right [catalyst] for the level you're merging FROM. " ..
            "Fourth: put all four items in the Forge — three augs plus one " ..
            "catalyst — and hit Combine. " ..
            "Out comes one augment of the next tier. Done. " ..
            "Simple enough even for an Artificer to understand."
        )

    -- AUGMENT FORGE
    elseif e.message:findi("forge") and not e.message:findi("forgemaster") then
        e.self:Say(
            "The Augment Forge is the tool of the trade. " ..
            "You put things in, better things come out. " ..
            "The Weaponsmith also carries them if you prefer. " ..
            "Keep one in your inventory at all times. You never know when " ..
            "you'll have three matching augments burning a hole in your bags."
        )

    -- CATALYSTS
    elseif e.message:findi("catalyst") then
        e.self:Say(
            "I carry catalysts for the lower merging tiers. " ..
            "Each tier of merge requires its own specific catalyst — " ..
            "use the wrong one and the Forge won't accept the combination. " ..
            "For the higher-tier catalysts, you'll need to talk to the Artificer. " ..
            "She deals in the rare stuff. " ..
            "Check my inventory and hers, between us we've got the full range."
        )

    -- INFUSION
    elseif e.message:findi("infusion") or e.message:findi("infuse") then
        e.self:Say(
            "Infusion is an advanced technique. Once you've got a " ..
            "max-level augment — the highest you can merge to — you can " ..
            "push it even further through infusion. " ..
            "Buy an Infusion Pool from me. Place your max-level augment " ..
            "inside along with an Infusion Catalyst and combine. " ..
            "The augment absorbs the catalyst's energy and gains a permanent " ..
            "stat increase. Small gains each time, but they add up. " ..
            "It's expensive and slow, but for someone who's already at the top? " ..
            "It's the only way forward. That's where the real progression lives."
        )

    -- INFUSION POOL
    elseif e.message:findi("pool") then
        e.self:Say(
            "The Infusion Pool is the container used for the infusion process. " ..
            "Similar to the Augment Forge but designed for a different reaction. " ..
            "One max-level augment plus one Infusion Catalyst — combine " ..
            "and the augment comes out a little bit stronger. " ..
            "I sell the Pool right here. Cheap investment for long-term power."
        )

    -- THE ORDER
    elseif e.message:findi("order") or e.message:findi("augmenteer") then
        e.self:Say(
            "The Grand Order of Augmenteers. Sounds fancier than it is. " ..
            "It's four people and a shared interest in making gear better. " ..
            "The Weaponsmith handles weapon augments — good solid work. " ..
            "The Provisioner manages the stat augments — thorough woman. " ..
            "The Artificer deals in rare materials — brilliant, if you can " ..
            "get past the attitude. " ..
            "And me? I keep the forges running. Someone has to."
        )
    end
end
