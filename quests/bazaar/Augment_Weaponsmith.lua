-- ============================================================
-- Augment Weaponsmith (181200) — Leveling Augments vendor
-- Member of the Grand Order of Augmenteers.
-- Sells platinum-based leveling weapon augments.
-- Stoic, militaristic, considers augments the "warrior's edge."
-- Dialogue focuses on MECHANICS, not specific item names/prices.
-- ============================================================

-- Randomized denial responses when asked about Fizzwick/salvage
local denials = {
    "I'm sorry, I don't know what you're referring to. " ..
    "The Grand Order does not concern itself with... whatever that is.",

    "A gnome? In the Bazaar? I see dozens of gnomes every day. " ..
    "None of them are of any particular note.",

    "Salvage? I'm afraid I deal exclusively in augmentation. " ..
    "Perhaps you have me confused with someone else.",

    "I have absolutely no idea what you're talking about. " ..
    "Now, did you have a question about [augments]?",

    "That name means nothing to me. " ..
    "Is there something I can help you with regarding the Order's services?",
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
            "Greetings, adventurer. I am the Weaponsmith of the Grand Order " ..
            "of Augmenteers — the foremost authority on equipment enhancement " ..
            "in all of Norrath. " ..
            "If you seek to improve your combat prowess, you have come " ..
            "to the right place. " ..
            "I can tell you about [augments], the [merging] process, " ..
            "or the [Order] itself."
        )

    -- AUGMENTS (general)
    elseif e.message:findi("augment") then
        e.self:Say(
            "Augments are enhancement stones that can be socketed into " ..
            "any equipment with an open augment slot. Once socketed, " ..
            "the augment's properties merge with the item — granting " ..
            "additional stats, damage effects, or other benefits. " ..
            "Browse my inventory to see what I carry. Each augment " ..
            "is calibrated for a specific level range, so find the one " ..
            "appropriate for your current strength. " ..
            "And remember — any augment can be [merged] to become " ..
            "even more powerful."
        )

    -- MERGING
    elseif e.message:findi("merg") then
        e.self:Say(
            "Augment merging is the pinnacle of our Order's research. " ..
            "The process is straightforward: obtain an Augment Forge, " ..
            "place three augments of the same type and level inside " ..
            "along with an appropriate catalyst, and combine. " ..
            "The result is one augment of the next tier — superior " ..
            "in every measurable way. " ..
            "The Forgemaster can explain the specifics of [catalysts] " ..
            "and the forge itself if you require further instruction."
        )

    -- CATALYSTS
    elseif e.message:findi("catalyst") then
        e.self:Say(
            "Catalysts are required components for the merging process. " ..
            "Each tier of merge requires a specific catalyst — use the " ..
            "wrong one and the Forge will reject the combination. " ..
            "I carry some of the basic catalysts. The Forgemaster stocks " ..
            "a broader selection, and the Artificer handles the rarest varieties. " ..
            "Precision in all things."
        )

    -- THE ORDER / AUGMENTEERS
    elseif e.message:findi("order") or e.message:findi("augmenteer") then
        e.self:Say(
            "The Grand Order of Augmenteers is the premier institution " ..
            "dedicated to the science of augmentation. We have studied the " ..
            "crystalline matrices of enhancement stones for generations. " ..
            "Our Provisioner handles common currency transactions. " ..
            "Our Artificer manages the rarest materials. " ..
            "And our Forgemaster oversees the merging apparatus. " ..
            "Together, we offer a complete path to equipment perfection."
        )
    end
end
