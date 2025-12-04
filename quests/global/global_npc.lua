function event_spawn(e)
    -- peq_halloween
    if (eq.is_content_flag_enabled("peq_halloween")) then
        -- exclude mounts and pets
        if (e.self:GetCleanName():findi("mount") or e.self:IsPet()) then
            return;
        end

        -- soulbinders
        -- priest of discord
        if (e.self:GetCleanName():findi("soulbinder") or e.self:GetCleanName():findi("priest of discord")) then
            e.self:ChangeRace(eq.ChooseRandom(14,60,82,85));
            e.self:ChangeSize(6);
            e.self:ChangeTexture(1);
            e.self:ChangeGender(2);
        end

        -- Shadow Haven
        -- The Bazaar
        -- The Plane of Knowledge
        -- Guild Lobby
        local halloween_zones = eq.Set { 202, 150, 151, 344 }
        local not_allowed_bodytypes = eq.Set { 11, 60, 66, 67 }
        if (halloween_zones[eq.get_zone_id()] and not_allowed_bodytypes[e.self:GetBodyType()] == nil) then
            e.self:ChangeRace(eq.ChooseRandom(14,60,82,85));
            e.self:ChangeSize(6);
            e.self:ChangeTexture(1);
            e.self:ChangeGender(2);
        end
    end
end

local function safe_tostring(value)
    local ok, result = pcall(function() return tostring(value) end)
    if ok and result then
        return result
    end
    return '<unprintable>'
end

-- Toggleable debug output for the upgrade script
-- Set to true to enable debug logging; false to disable all debug lines
local UPGRADE_DEBUG = false
local function debug_print(...)
    if not UPGRADE_DEBUG then
        return
    end
    eq.debug(...)
end

function event_death_complete(e)
    local client = e.other

	if not client then
        return
    end

    -- If the killer is a pet, get the owner
    if client:IsPet() then
        client = client:GetOwner()
    end

    -- Verify we have a client
    if not client or not client:IsClient() then
        return
    end

    client = client:CastToClient()

    -- Check if this is a rare/named mob
    -- The ONLY reliable indicator is the rare_spawn flag in the database
    -- This is set via npc_types.rare_spawn column or #npcedit rarespawn command
    local npc_name = e.self:GetCleanName()
    local npc_type_id = e.self:GetNPCTypeID()
    local is_rare_spawn = e.self:CastToNPC():IsRareSpawn()

    -- Use ONLY the rare_spawn flag for named detection
    local is_named = is_rare_spawn

    -- DEBUG: Log every kill for testing
    debug_print(string.format("[UPGRADE] Kill: %s (ID: %d, RareSpawn: %s)",
        npc_name, npc_type_id, tostring(is_rare_spawn)))

    -- Chance configuration: base chance depends on con color (mob vs player)
    -- Con categories mapped to chances (in per-1000 units for 0.1% granularity):
    -- Gray => 0.1% (1/1000)
    -- Light Blue => 1% (10/1000)
    -- Dark Blue => 2% (20/1000)
    -- White => 3% (30/1000)
    -- Yellow => 4% (40/1000)
    -- Red => 5% (50/1000)
    -- Named mobs continue to get 100% chance (1000/1000)
    local con = ""
    -- Obtain consider color from client perspective (matches client UI). Fallback to mob-perspective if needed.
    local success_client, client_con_str = pcall(function() return client:GetConsiderColor(e.self:GetLevel()) end)
    local success_mob, mob_con_str = pcall(function() return e.self:GetConsiderColor(client:GetLevel()) end)
    if success_client and client_con_str then
        con = tostring(client_con_str):lower()
        debug_print(string.format("Client-con = %s", con))
    elseif success_mob and mob_con_str then
        con = tostring(mob_con_str):lower()
        debug_print(string.format("Mob-con = %s (fallback)", con))
    end

    local threshold = 0 -- per 1000 scale
    if con == "gray" or con == "grey" then
        threshold = 1
	elseif con == "green" then
		threshold = 5
    elseif con == "light blue" then
        threshold = 10
    elseif con == "dark blue" or con == "blue" then
        threshold = 20
    elseif con == "white" then
        threshold = 30
    elseif con == "yellow" then
        threshold = 40
    elseif con == "red" then
        threshold = 50
    else
        -- fallback: treat unknown / green / others as very unlikely
        threshold = 1
    end

    if is_named then
        threshold = 1000 -- 100% chance
    end

    -- Use a 0..1000 roll so 0.1% is representable
    local roll = math.random(1000)
    local mob_level = e.self:GetLevel()
    local player_level = client:GetLevel()
    debug_print(string.format("[UPGRADE] MobLevel=%d ClientLevel=%d Con [%s], Roll: %d/%d (needed %d or less)",
        mob_level, player_level, con, roll, 1000, threshold))

    if roll > threshold then
        return
    end

    debug_print("[UPGRADE] Upgrade triggered! Collecting equipped items...")

    -- Equipment slots to check (EQ::invslot::slotCharm=0 through slotAmmo=21)
    local equipment_slots = {
        0,  -- Charm
        1,  -- Left Ear
        2,  -- Head
        3,  -- Face
        4,  -- Right Ear
        5,  -- Neck
        6,  -- Shoulders
        7,  -- Arms
        8,  -- Back
        9,  -- Left Wrist
        10, -- Right Wrist
        11, -- Range
        12, -- Hands
        13, -- Primary
        14, -- Secondary
        15, -- Left Finger
        16, -- Right Finger
        17, -- Chest
        18, -- Legs
        19, -- Feet
        20, -- Waist
        21, -- Power Source / Ammo (slotAmmo upper bound in command validation)
    }

    -- Collect all equipped items with slot numbers
    local equipped_items = {}
    for _, slot_id in ipairs(equipment_slots) do
        local item_inst = client:GetInventory():GetItem(slot_id)
        if item_inst then
            -- Validate the instance has a real ID
            local inst_ok, inst_id = pcall(function() return item_inst:GetID() end)
            if not inst_ok or not inst_id or inst_id == 0 then
                -- Empty or invalid placeholder, skip quietly
                goto continue
            end

            local item_ok, item = pcall(function() return item_inst:GetItem() end)
            if not item_ok or not item then
                debug_print(string.format("[UPGRADE] slot %d: item_inst present, but GetItem() returned nil. inst_id=%s, item_inst=%s",
                    slot_id, tostring(inst_ok and inst_id or 'unknown'), safe_tostring(item_inst)))
                goto continue
            end


            -- Attempt to extract more info from the item object safely
            local ok_name, item_name = pcall(function() return item:GetName() end)
            local ok_base, base_id = pcall(function() return item:GetID() end)
            if not ok_name or not item_name then
                item_name = 'UNKNOWN_ITEM'
            end
            table.insert(equipped_items, slot_id)
            debug_print(string.format("[UPGRADE] Found item in slot %d: %s (inst_id=%s, base_id=%s)",
                slot_id, tostring(item_name), tostring(inst_id), tostring(ok_base and base_id or 'unknown')))
            if not ok_name then
                debug_print(string.format("[UPGRADE] Warning: item:GetName() failed for slot %d; item_inst=%s, item=%s",
                    slot_id, safe_tostring(item_inst), safe_tostring(item)))
            end
        end
        ::continue::
    end


    -- If no equipped items, nothing to upgrade (defensive check)
    if not equipped_items or #equipped_items == 0 then
        debug_print("[UPGRADE] No equipped items found!")
        client:Message(15, "You have no equipped items to upgrade!")
        return
    end

    debug_print(string.format("[UPGRADE] Total equipped items: %d", #equipped_items))

    -- For named mobs, upgrade multiple items (1-3 random items)
    local num_upgrades = 1
    if is_named then
        num_upgrades = math.random(1, 3)
        client:Message(15, "The power of the slain creature surges through your equipment!")
    end

    -- Make sure we don't try to upgrade more items than equipped
    if equipped_items and #equipped_items > 0 then
        num_upgrades = math.min(num_upgrades, #equipped_items)
    else
        num_upgrades = 0
    end

    debug_print(string.format("[UPGRADE] Will upgrade %d items", num_upgrades))

    -- Pick random slots and upgrade them
    for i = 1, num_upgrades do
        local random_index = math.random(1, #equipped_items)
        local slot_id = table.remove(equipped_items, random_index)

        debug_print(string.format("[UPGRADE] Upgrading slot %d (attempt %d/%d)",
            slot_id, i, num_upgrades))

        -- Use the #upgrade command internally (requires GM status bypass)
        -- Format: #upgrade <slot_number>
        local success = client:SendGMCommand("#upgrade " .. slot_id, true)  -- true = ignore status check

        debug_print(string.format("[UPGRADE] SendGMCommand result: %s", tostring(success)))
    end
end
