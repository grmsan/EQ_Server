-- #iteminfo command - Display detailed item level and stats
-- Usage: #iteminfo [cursor|slot_name]
-- Example: #iteminfo cursor

function command_iteminfo(e)
    local client = e.self
    local args = e.args
    local target = args[1] or "cursor"

    local slot_map = {
        cursor = 33,  -- RoF2 default
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
        ammo = 21
    }

    if client:GetClientVersion() ~= 7 then
        slot_map.cursor = 30
    end

    local slot_id = tonumber(target) or slot_map[target:lower()]
    if not slot_id then
        client:Message(13, "[ItemInfo] Invalid target: " .. target)
        return
    end

    local inst = client:GetInventory():GetItem(slot_id)
    if not inst then
        client:Message(13, "[ItemInfo] No item found")
        return
    end

    local item_id = inst:GetID()
    local item = inst:GetItem()

    client:Message(15, "=== ITEM INFO ===")
    client:Message(15, "Name: " .. item:Name())
    client:Message(15, "ID: " .. item_id)

    -- Check if dynamic item
    if item_id >= 500000000 then
        local level = math.floor((item_id / 1000) % 100000)
        local base_id = item_id % 1000

        client:Message(15, "Type: Dynamic Item")
        client:Message(15, "Level: +" .. level)
        client:Message(15, "Base ID: " .. base_id)
    else
        client:Message(15, "Type: Base Item")
        client:Message(15, "Level: +0")
    end

    client:Message(15, "")
    client:Message(15, "Stats:")
    client:Message(15, "  AC: " .. item:AC())
    client:Message(15, "  HP: " .. item:HP())
    client:Message(15, "  Mana: " .. item:Mana())

    if item:AStr() > 0 then client:Message(15, "  STR: +" .. item:AStr()) end
    if item:ASta() > 0 then client:Message(15, "  STA: +" .. item:ASta()) end
    if item:AAgi() > 0 then client:Message(15, "  AGI: +" .. item:AAgi()) end
    if item:ADex() > 0 then client:Message(15, "  DEX: +" .. item:ADex()) end
    if item:AInt() > 0 then client:Message(15, "  INT: +" .. item:AInt()) end
    if item:AWis() > 0 then client:Message(15, "  WIS: +" .. item:AWis()) end
    if item:ACha() > 0 then client:Message(15, "  CHA: +" .. item:ACha()) end

    if item:HeroicStr() > 0 then client:Message(15, "  H.STR: +" .. item:HeroicStr()) end
    if item:HeroicSta() > 0 then client:Message(15, "  H.STA: +" .. item:HeroicSta()) end
    if item:HeroicAgi() > 0 then client:Message(15, "  H.AGI: +" .. item:HeroicAgi()) end
    if item:HeroicDex() > 0 then client:Message(15, "  H.DEX: +" .. item:HeroicDex()) end
    if item:HeroicInt() > 0 then client:Message(15, "  H.INT: +" .. item:HeroicInt()) end
    if item:HeroicWis() > 0 then client:Message(15, "  H.WIS: +" .. item:HeroicWis()) end
    if item:HeroicCha() > 0 then client:Message(15, "  H.CHA: +" .. item:HeroicCha()) end

    if item:Haste() > 0 then client:Message(15, "  Haste: " .. item:Haste() .. "%") end
    if item:Regen() > 0 then client:Message(15, "  Regen: +" .. item:Regen()) end
end

return command_iteminfo
