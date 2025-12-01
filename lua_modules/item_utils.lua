local item_utils = {}

-- Helper function to intelligently set item stats, handling the 127 cap for base stats
-- by overflowing into Heroic stats automatically.
function item_utils.SetSmartStat(item, slot, statName, value)
    local cap = 127
    local val = tonumber(value)
    if not val then return end -- Handle nil/invalid input gracefully

    -- List of stats that have Heroic counterparts
    local heroicMap = {
        ["STR"] = "HEROIC_STR",
        ["STA"] = "HEROIC_STA",
        ["DEX"] = "HEROIC_DEX",
        ["AGI"] = "HEROIC_AGI",
        ["INT"] = "HEROIC_INT",
        ["WIS"] = "HEROIC_WIS",
        ["CHA"] = "HEROIC_CHA",
        ["MR"]  = "HEROIC_MR",
        ["FR"]  = "HEROIC_FR",
        ["CR"]  = "HEROIC_CR",
        ["DR"]  = "HEROIC_DR",
        ["PR"]  = "HEROIC_PR",
        ["SV_CORRUP"] = "HEROIC_SV_CORRUP"
    }

    if heroicMap[statName] then
        if val > cap then
            -- Max out the base stat
            item:SetItemStat(slot, statName, tostring(cap))
            -- Put the rest in Heroic
            local heroicVal = val - cap
            item:SetItemStat(slot, heroicMap[statName], tostring(heroicVal))
        else
            -- Fits in base stat
            item:SetItemStat(slot, statName, tostring(val))
        end
    else
        -- Stat doesn't have a heroic equivalent (like HP, Mana, AC), just set it directly
        -- Note: HP/Mana/AC are 32-bit and don't have the 127 limit.
        item:SetItemStat(slot, statName, tostring(val))
    end
end

return item_utils
