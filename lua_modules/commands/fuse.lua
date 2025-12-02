-- #fuse command - Transfer levels from cursor item to worn/inventory item
-- Usage: #fuse [slot_name|slot_number]
-- Example: #fuse chest  (fuse cursor item into chest slot)

function command_fuse(e)
    local client = e.self
    local args = e.args
    local target_slot_name = args[1] or "chest"

    -- Slot name to number mapping (RoF2 slots)
    local slot_map = {
        charm = 0,
        ear1 = 1,
        head = 2,
        face = 3,
        ear2 = 4,
        neck = 5,
        shoulder = 6,
        arms = 7,
        back = 8,
        wrist1 = 9,
        wrist2 = 10,
        range = 11,
        hands = 12,
        primary = 13,
        secondary = 14,
        finger1 = 15,
        finger2 = 16,
        chest = 17,
        legs = 18,
        feet = 19,
        waist = 20,
        ammo = 21,
        powersource = 9999
    }

    -- Get cursor item (donor)
    local cursor_slot = 33  -- RoF2
    if client:GetClientVersion() ~= 7 then
        cursor_slot = 30
    end

    local donor = client:GetInventory():GetItem(cursor_slot)
    if not donor then
        client:Message(13, "[Fuse] No donor item on cursor")
        return
    end

    -- Determine receiver slot
    local receiver_slot = tonumber(target_slot_name) or slot_map[target_slot_name:lower()]
    if not receiver_slot then
        client:Message(13, "[Fuse] Invalid slot: " .. target_slot_name)
        client:Message(13, "[Fuse] Use: chest, head, arms, etc. or slot number")
        return
    end

    local receiver = client:GetInventory():GetItem(receiver_slot)
    if not receiver then
        client:Message(13, "[Fuse] No item in slot: " .. target_slot_name)
        return
    end

    -- Extract levels
    local donor_id = donor:GetID()
    local donor_level = 0
    if donor_id >= 500000000 then
        donor_level = math.floor((donor_id / 1000) % 100000)
    end

    local receiver_id = receiver:GetID()
    local receiver_base_id = receiver_id
    if receiver_id >= 500000000 then
        receiver_base_id = receiver_id % 1000
    end

    if donor_level == 0 then
        client:Message(13, "[Fuse] Donor item has no levels to transfer")
        return
    end

    client:Message(15, "=== FUSION PREVIEW ===")
    client:Message(15, "Donor: " .. donor:GetItem():Name() .. " (+" .. donor_level .. " levels)")
    client:Message(15, "Receiver: " .. receiver:GetItem():Name())
    client:Message(15, "Result: " .. receiver:GetItem():Name() .. " +" .. donor_level)
    client:Message(15, "")
    client:Message(13, "[Fuse] Fusion system not yet implemented")
    client:Message(13, "[Fuse] Donor would be destroyed, receiver would gain all levels")

    -- TODO: Once DynamicItemManager is exposed:
    -- local fused = eq.DynamicItemManager.CreateDynamicInstance(receiver_base_id, donor_level)
    -- client:DeleteItemInInventory(cursor_slot)  -- Destroy donor
    -- client:DeleteItemInInventory(receiver_slot)  -- Remove old receiver
    -- client:PutItemInInventory(receiver_slot, fused)
end

return command_fuse
