-- ============================================================
-- Essence Artificer (181202) — Rare Essence vendor
-- Member of the Grand Order of Augmenteers.
-- Sells proc augments, high-tier catalysts for Rare Essence (currency 101).
-- Aloof, somewhat mystical, considers her work an art form.
-- Dialogue focuses on MECHANICS, not specific item names/prices.
-- ============================================================

-- Randomized denial responses when asked about Fizzwick/salvage
local denials = {
    "I haven't the faintest idea what you're talking about. " ..
    "My work concerns augmentation, not... disassembly.",

    "A gnome? I see no gnome. I have never seen a gnome. " ..
    "Are we finished with this line of questioning?",

    "That word has no meaning within these walls. " ..
    "If you wish to discuss something of substance, " ..
    "I deal in [augments] and [catalysts].",

    "I'm afraid you have me at a complete loss. " ..
    "I suggest you direct such inquiries elsewhere. Preferably far from here.",

    "*stares blankly* " ..
    "No. I am not aware of any such person or... practice. " ..
    "Now then. [Augments]?",
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
            "Ah. Another aspirant. " ..
            "I am the Artificer of the Grand Order of Augmenteers. " ..
            "My work deals in the rarefied — [augments] of extraordinary " ..
            "potency, and the [catalysts] required for the highest tiers " ..
            "of augment merging. " ..
            "I accept only Rare [Essence] as payment. If you do not know " ..
            "what that is, I suggest you educate yourself before wasting " ..
            "my time. Or speak with the Provisioner. She enjoys... explaining things."
        )

    -- AUGMENTS (general)
    elseif e.message:findi("augment") then
        e.self:Say(
            "I carry weapon augments that trigger spell effects each time " ..
            "your weapon connects with a foe. Proc augments, as they are " ..
            "commonly known. " ..
            "Browse my inventory — you will find a variety of elemental " ..
            "and restorative effects, each available in multiple tiers. " ..
            "All of them can be [merged] to reach greater potency. " ..
            "Do not waste my time asking about stat stones or leveling trinkets. " ..
            "Those are the Provisioner's and Weaponsmith's domains."
        )

    -- ESSENCE (rare)
    elseif e.message:findi("essence") then
        e.self:Say(
            "Rare Essence is distilled from items of exceptional provenance — " ..
            "equipment that once belonged to named creatures, raid bosses, " ..
            "beings of genuine power. " ..
            "When such items are broken down, there is a chance that Rare Essence " ..
            "crystallizes alongside the common variety. " ..
            "It cannot be manufactured. It cannot be duplicated. " ..
            "It can only be earned through the defeat of worthy adversaries " ..
            "and the reclamation of their artifacts."
        )

    -- CATALYSTS
    elseif e.message:findi("catalyst") then
        e.self:Say(
            "I carry the advanced catalysts — the ones required for " ..
            "the upper tiers of augment merging. " ..
            "The lower-tier catalysts are available from the Weaponsmith " ..
            "or the Forgemaster. I do not deal in pedestrian materials. " ..
            "If you intend to push an augment to its maximum potential, " ..
            "you will need what I offer. Plan accordingly."
        )

    -- MERGING
    elseif e.message:findi("merg") then
        e.self:Say(
            "Place three identical augments and the appropriate catalyst " ..
            "into an Augment Forge. Combine. One augment of the next tier emerges. " ..
            "The Provisioner or the Weaponsmith can explain the fundamentals " ..
            "if you require further hand-holding. " ..
            "I concern myself only with the upper echelons of the craft."
        )

    -- THE ORDER
    elseif e.message:findi("order") or e.message:findi("augmenteer") then
        e.self:Say(
            "The Grand Order of Augmenteers represents the pinnacle of " ..
            "applied crystalline enchantment. We are four in number, " ..
            "each a specialist in our domain. " ..
            "The Weaponsmith forges the leveling stones — solid work, if basic. " ..
            "The Provisioner curates the stat matrices — thorough, if predictable. " ..
            "The Forgemaster tends the merge apparatus — reliable, if uninspired. " ..
            "And I? I work with the rare. The exceptional. The extraordinary."
        )
    end
end
