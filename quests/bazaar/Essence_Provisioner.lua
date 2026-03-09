-- ============================================================
-- Essence Provisioner (181201) — Common Essence vendor
-- Member of the Grand Order of Augmenteers.
-- Sells stat augments, solvents for Common Essence (currency 100).
-- Scholarly, precise, the "professor" of the group.
-- Dialogue focuses on MECHANICS, not specific item names/prices.
-- ============================================================

-- Randomized denial responses when asked about Fizzwick/salvage
local denials = {
    "I beg your pardon? I'm not familiar with that term. " ..
    "The Order's curriculum is quite extensive — perhaps you're " ..
    "confusing it with something from another discipline entirely.",

    "A gnome? Doing what now? I'm afraid I don't follow. " ..
    "My area of expertise is augmentation, not... whatever you're describing.",

    "I'm sure I have no idea what you mean. " ..
    "Now, shall we discuss something within my purview? " ..
    "Perhaps [augments] or [merging]?",

    "That word is not in the Grand Order's lexicon. " ..
    "I would suggest consulting a dictionary. Or perhaps a therapist.",

    "Hmm? No, that doesn't ring any bells. Not a single one. " ..
    "Were you looking for information about [augments]?",
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
            "Welcome. I am the Provisioner of the Grand Order of Augmenteers. " ..
            "I curate a selection of enhancement stones that grant heroic " ..
            "attributes to any who can afford the investment. " ..
            "I also carry Purified Solvent for safe augment removal. " ..
            "If you are unfamiliar with [augments], [essence], " ..
            "or the [merging] process, I would be happy to educate you."
        )

    -- AUGMENTS (general explanation)
    elseif e.message:findi("augment") then
        e.self:Say(
            "Augments are crystallized enhancement matrices — small stones that " ..
            "can be socketed into equipment with open augment slots. " ..
            "Once socketed, the augment's properties merge with the item, " ..
            "granting additional stats or other enhancements. " ..
            "Browse my inventory to see the stat augments I carry. " ..
            "The Weaponsmith offers weapon augments, and the " ..
            "Artificer deals in the rarest varieties. " ..
            "All augments can be [merged] to increase their power."
        )

    -- ESSENCE (how to get it)
    elseif e.message:findi("essence") then
        e.self:Say(
            "Essence is the currency of enhancement. " ..
            "Common Essence is earned by breaking down unwanted magic items. " ..
            "The process involves placing magic items into a specialized container " ..
            "and combining them — the essence is extracted and added to your " ..
            "Alternate Currency balance automatically. " ..
            "Check the Alternate Currency tab of your inventory window " ..
            "to see your current balance. " ..
            "The more powerful the item, the more essence you receive."
        )

    -- SOLVENT
    elseif e.message:findi("solvent") then
        e.self:Say(
            "The Purified Solvent is an essential tool for any augment user. " ..
            "If you wish to remove an augment from a piece of equipment — " ..
            "perhaps to upgrade it or move it to a better item — simply " ..
            "use the solvent. It safely extracts the augment without " ..
            "damaging either the stone or the equipment. " ..
            "A small price for the flexibility it provides."
        )

    -- MERGING
    elseif e.message:findi("merg") then
        e.self:Say(
            "The merging process is the crown jewel of augmentation science. " ..
            "Obtain an Augment Forge from the Weaponsmith or the Forgemaster. " ..
            "Place three augments of the SAME type and level into the Forge, " ..
            "along with the correct catalyst. Click Combine. " ..
            "The result: one augment of the next tier — " ..
            "significantly more powerful than any of the three inputs. " ..
            "Higher-tier catalysts are required as you advance. " ..
            "Plan your investments wisely."
        )

    -- CATALYSTS
    elseif e.message:findi("catalyst") then
        e.self:Say(
            "Catalysts are the binding agents in the merging process. " ..
            "Each tier of augment requires a specific catalyst to merge — " ..
            "the Forge simply will not accept an incorrect pairing. " ..
            "Basic catalysts are available from the Weaponsmith and Forgemaster. " ..
            "The rarer varieties come from the Artificer. " ..
            "Consult the Forgemaster if you need guidance on which catalyst " ..
            "corresponds to which tier."
        )

    -- THE ORDER
    elseif e.message:findi("order") or e.message:findi("augmenteer") then
        e.self:Say(
            "The Grand Order of Augmenteers exists to elevate the art of " ..
            "equipment enhancement beyond mere tinkering. " ..
            "We are researchers. Craftsmen. Scholars. " ..
            "Each of us has dedicated years to understanding the crystalline " ..
            "matrices that make augmentation possible. " ..
            "Our work is constructive. Additive. Superior."
        )
    end
end
