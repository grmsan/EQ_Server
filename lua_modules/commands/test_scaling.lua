local function command_testscaling(e)
    local client = e.self
    local args = e.args
    local action = args[1] or "apply"

    local cursor_slot = 30 -- Default Titanium/Standard
    if client:GetClientVersion() == 7 then -- RoF2
        cursor_slot = 33
    end

    local inst = client:GetInventory():GetItem(cursor_slot)

    if inst == nil then
        client:Message(13, "No item on cursor.")
        return
    end

    if action == "check" then
        client:Message(13, "Item: " .. inst:GetItem():Name())
        client:Message(13, "Scaling: " .. tostring(inst:GetScaling()))
        client:Message(13, "Exp: " .. inst:GetExp())
    else
        local current_hp = tonumber(inst:GetCustomData("HP")) or 0
        local new_hp = current_hp + 50

        client:Message(13, "Applying Scaling to item: " .. inst:GetItem():Name())
        client:Message(13, "Old HP: " .. current_hp .. " -> New HP: " .. new_hp)

        inst:SetScaling(true)
        inst:SetCustomData("HP", tostring(new_hp))

        client:SendItemScale(inst)
        client:Message(13, "Done. Item stats should update immediately.")
    end
end

return command_testscaling
