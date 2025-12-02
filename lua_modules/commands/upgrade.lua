-- #upgrade command - Level up item on cursor
-- Usage: #upgrade [levels]
-- Example: #upgrade 5  (adds 5 levels to cursor item)

function command_upgrade(e)
    local client = e.self
    local args = e.args
    local levels_to_add = tonumber(args[1]) or 1

    if levels_to_add < 1 then
        client:Message(13, "Usage: #upgrade [levels] - Must be at least 1")
        return
    end

    -- Get cursor item
    local cursor_slot = 33  -- RoF2
    if client:GetClientVersion() ~= 7 then
        cursor_slot = 30  -- Titanium/older
    end

    local inst = client:GetInventory():GetItem(cursor_slot)
    if not inst then
        client:Message(13, "[Upgrade] No item on cursor")
        return
    end

    local item_id = inst:GetID()
    local item_name = inst:GetItem():Name()

    -- Use inf.* functions to extract level and base ID
    local current_level = inf.get_item_level(item_id)
    local base_id = inf.get_base_item_id(item_id)
    local new_level = current_level + levels_to_add

    client:Message(15, "[Upgrade] " .. item_name .. " +" .. current_level .. " -> +" .. new_level)
    client:Message(15, "[Upgrade] Base ID: " .. base_id)

    -- Generate new dynamic ID
    local new_id = inf.generate_dynamic_id(base_id, new_level)

    client:Message(15, "[Upgrade] New dynamic ID: " .. new_id)
    client:Message(15, "[Upgrade] Attempting to create upgraded item...")

    -- Summon the new dynamic item
    client:SummonItem(new_id)

    client:Message(15, "[Upgrade] Check cursor for upgraded item")
    client:Message(15, "[Upgrade] Original item remains - delete manually if successful")
end

return command_upgrade
