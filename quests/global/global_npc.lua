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
local TEST_BASE_CHANCE_FULL = 10.5
local TEST_UPGRADE_MODE = true       -- set to false when done testing

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

    -- Check if this is a rare or named mob
    local npc_name = e.self:GetCleanName()
    local npc_type_id = e.self:GetNPCTypeID()
    local is_rare_spawn = e.self:CastToNPC():IsRareSpawn()
    local is_named = is_rare_spawn

    debug_print(string.format(
        "[UPGRADE] Kill: %s (ID: %d, RareSpawn: %s)",
        npc_name, npc_type_id, tostring(is_rare_spawn)
    ))

    ----------------------------------------------------------------
    -- 1. Compute base upgrade chance from con color
    --    We now work directly in floating point probabilities.
    ----------------------------------------------------------------
    local con = ""
    local success_client, client_con_str = pcall(function()
        return client:GetConsiderColor(e.self:GetLevel())
    end)
    local success_mob, mob_con_str = pcall(function()
        return e.self:GetConsiderColor(client:GetLevel())
    end)

    if success_client and client_con_str then
        con = tostring(client_con_str):lower()
        debug_print(string.format("Client-con = %s", con))
    elseif success_mob and mob_con_str then
        con = tostring(mob_con_str):lower()
        debug_print(string.format("Mob-con = %s (fallback)", con))
    end

    -- Map con color to a base chance for a fully geared character
    -- These are "expected upgrades per kill" when wearing all slots
    local base_chance_full = 0.0

    if con == "gray" or con == "grey" then
        base_chance_full = 0.001    -- 0.1 percent
    elseif con == "green" then
        base_chance_full = 0.005    -- 0.5 percent
    elseif con == "light blue" then
        base_chance_full = 0.01     -- 1 percent
    elseif con == "dark blue" or con == "blue" then
        base_chance_full = 0.02     -- 2 percent
    elseif con == "white" then
        base_chance_full = 0.03     -- 3 percent
    elseif con == "yellow" then
        base_chance_full = 0.04     -- 4 percent
    elseif con == "red" then
        base_chance_full = 0.05     -- 5 percent
    else
        -- Treat unknown like a very weak mob
        base_chance_full = 0.001
    end

    -- Named mobs: we want them to be much juicier
    -- Interpret this as "expected upgrades per kill for a fully geared character"
    -- Old behavior was 1 to 3 upgrades, average 2, and one hundred percent of the time
    local NAMED_EXPECTED_UPGRADES_FULL = 2.0

    if is_named then
        base_chance_full = NAMED_EXPECTED_UPGRADES_FULL
    end

	if TEST_UPGRADE_MODE then
        base_chance_full = TEST_BASE_CHANCE_FULL
        debug_print(string.format(
            "[UPGRADE] TEST MODE: forcing base_chance_full to %.3f",
            base_chance_full
        ))
    end

    local mob_level = e.self:GetLevel()
    local player_level = client:GetLevel()
    debug_print(string.format(
        "[UPGRADE] MobLevel=%d ClientLevel=%d Con [%s], base_chance_full=%.4f (named=%s)",
        mob_level, player_level, con, base_chance_full, tostring(is_named)
    ))

    ----------------------------------------------------------------
    -- 2. Collect equipped items
    ----------------------------------------------------------------
    local equipment_slots = {
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
        20 -- Waist
    }

    -- This is the maximum number of slots that can ever be upgraded
    -- This is the value you use for "fully geared" scaling
    local MAX_UPGRADE_SLOTS = #equipment_slots

    -- Collect all equipped items with slot numbers
    local equipped_slots = {}

    -- Optional extra logging flag
    local VERBOSE_ITEM_DEBUG = false

    for _, slot_id in ipairs(equipment_slots) do
        local item_inst = client:GetInventory():GetItem(slot_id)
        if item_inst then
            -- Check that the instance has a valid instance id
            local inst_ok, inst_id = pcall(function() return item_inst:GetID() end)
            if not inst_ok or not inst_id or inst_id == 0 then
                -- Invalid or empty instance, skip quietly
                goto continue
            end

            -- We only care that there is a valid instance in this slot
            table.insert(equipped_slots, slot_id)

            if VERBOSE_ITEM_DEBUG then
                -- Try to get item template information, but never spam errors
                local base_id = nil
                local item_name = nil

                local item_ok, item = pcall(function() return item_inst:GetItem() end)
                if item_ok and item then
                    local ok_base, b_id = pcall(function() return item:GetID() end)
                    if ok_base then
                        base_id = b_id
                    end

                    local ok_name, n = pcall(function() return item:GetName() end)
                    if ok_name then
                        item_name = n
                    end
                end

                debug_print(string.format(
                    "[UPGRADE] Equipped slot %d (inst_id=%s%s%s)",
                    slot_id,
                    tostring(inst_id),
                    base_id and (", base_id=" .. tostring(base_id)) or "",
                    item_name and (", name=" .. tostring(item_name)) or ""
                ))
            else
                -- Clean simple log line
                debug_print(string.format(
                    "[UPGRADE] Equipped slot %d (inst_id=%s)",
                    slot_id,
                    tostring(inst_id)
                ))
            end
        end
        ::continue::
    end

    local equipped_count = #equipped_slots
    if equipped_count == 0 then
        debug_print("[UPGRADE] No equipped items found.")
        client:Message(15, "You have no equipped items to upgrade.")
        return
    end

    debug_print(string.format("[UPGRADE] Total equipped items: %d", equipped_count))


    ----------------------------------------------------------------
    -- 3. Per slot roll logic
    --
    -- Idea:
    --   base_chance_full is "expected upgrades per kill" when wearing MAX_UPGRADE_SLOTS items
    --
    --   per_slot_chance = base_chance_full / MAX_UPGRADE_SLOTS
    --
    --   then for each equipped slot we roll:
    --       if math.random() < per_slot_chance then upgrade that slot
    --
    -- Properties:
    --   expected total upgrades = equipped_count * per_slot_chance
    --                            = base_chance_full * (equipped_count / MAX_UPGRADE_SLOTS)
    --   so wearing more gear increases your total upgrade rate
    --
    --   for non named red:
    --      base_chance_full = 0.05
    --      per_slot_chance = 0.05 / MAX_UPGRADE_SLOTS
    --      a fully geared player (equipped_count = MAX_UPGRADE_SLOTS)
    --      still averages about one upgrade every twenty kills
    ----------------------------------------------------------------

    local per_slot_chance = base_chance_full / MAX_UPGRADE_SLOTS

    -- Safety clamp
    if per_slot_chance < 0 then
        per_slot_chance = 0
    elseif per_slot_chance > 1 then
        per_slot_chance = 1
    end

    debug_print(string.format(
        "[UPGRADE] per_slot_chance=%.6f, MAX_UPGRADE_SLOTS=%d",
        per_slot_chance, MAX_UPGRADE_SLOTS
    ))

    local total_upgrades = 0

    -- Roll once per equipped slot
    for _, slot_id in ipairs(equipped_slots) do
        local r = math.random()  -- 0 to 1
        if r < per_slot_chance then
            debug_print(string.format(
                "[UPGRADE] Upgrading slot %d (roll=%.6f < %.6f)",
                slot_id, r, per_slot_chance
            ))
            local success = client:SendGMCommand("#upgrade " .. slot_id, true)
            debug_print(string.format("[UPGRADE] SendGMCommand result: %s", tostring(success)))
            total_upgrades = total_upgrades + 1
        else
            debug_print(string.format(
                "[UPGRADE] No upgrade for slot %d (roll=%.6f >= %.6f)",
                slot_id, r, per_slot_chance
            ))
        end
    end

    ----------------------------------------------------------------
    -- 4. Named safety net
    -- For named, we expect plenty of upgrades, but because rolls are random
    -- it is still possible to whiff. If you want to guarantee that named
    -- always gives at least one upgrade, keep a small safety net here.
    ----------------------------------------------------------------
    if is_named and total_upgrades == 0 then
        debug_print("[UPGRADE] Named mob gave zero upgrades from per slot rolls, applying safety net.")

        -- Pick between one and three slots and force upgrades
        local num_guaranteed = math.min(equipped_count, math.random(1, 3))
        client:Message(15, "The power of the slain creature surges through your equipment!")

        -- Shuffle a copy of the slot list and take first num_guaranteed
        local slots_copy = {}
        for i, slot_id in ipairs(equipped_slots) do
            slots_copy[i] = slot_id
        end
        for i = #slots_copy, 2, -1 do
            local j = math.random(i)
            slots_copy[i], slots_copy[j] = slots_copy[j], slots_copy[i]
        end

        for i = 1, num_guaranteed do
            local slot_id = slots_copy[i]
            debug_print(string.format(
                "[UPGRADE] Named safety net upgrading slot %d (%d of %d)",
                slot_id, i, num_guaranteed
            ))
            local success = client:SendGMCommand("#upgrade " .. slot_id, true)
            debug_print(string.format("[UPGRADE] SendGMCommand result: %s", tostring(success)))
        end
    end

    debug_print(string.format("[UPGRADE] Total upgrades this kill: %d", total_upgrades))
end

