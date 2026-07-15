local M = {}
local bit = require("bit")

local TASK_ID = 600100
local GREETER_NPC_ID = 12000189

local CLASS_NAMES = {
    [1] = "Warrior", [2] = "Cleric", [3] = "Paladin", [4] = "Ranger",
    [5] = "Shadow Knight", [6] = "Druid", [7] = "Monk", [8] = "Bard",
    [9] = "Rogue", [10] = "Shaman", [11] = "Necromancer", [12] = "Wizard",
    [13] = "Magician", [14] = "Enchanter", [15] = "Beastlord", [16] = "Berserker"
}

local function has_class(bits, class_id)
    return bit.band(bits, bit.lshift(1, class_id - 1)) ~= 0
end

local function class_count(client)
    if client.GetClassesCount then
        return client:GetClassesCount()
    end

    local bits = client:GetClassesBitmask()
    local count = 0
    for class_id = 1, 16 do
        if has_class(bits, class_id) then
            count = count + 1
        end
    end
    return count
end

local function class_summary(client)
    local bits = client:GetClassesBitmask()
    local names = {}
    for class_id = 1, 16 do
        if has_class(bits, class_id) then
            table.insert(names, CLASS_NAMES[class_id])
        end
    end
    if #names == 0 then
        return "your first calling"
    end
    return table.concat(names, "/")
end

local function ensure_task(client)
    if not client:IsTaskActive(TASK_ID) and not client:IsTaskCompleted(TASK_ID) then
        client:AssignTask(TASK_ID, GREETER_NPC_ID)
    end
end

local function update_if_active(client, activity_id)
    if client:IsTaskActivityActive(TASK_ID, activity_id) then
        client:UpdateTaskActivity(TASK_ID, activity_id, 1)
    end
end

function M.guildmaster_say(e, class_id)
    local client = e.other
    local count = class_count(client)
    local class_name = CLASS_NAMES[class_id]
    local bits = client:GetClassesBitmask()
    local confirm_phrase = "confirm_class_" .. tostring(class_id)

    if e.message:findi("hail") then
        ensure_task(client)

        if count >= 3 then
            e.self:Say("Your trio is already complete. Return to the Bazaar Greeter when you are ready to leave.")
            return
        end

        if has_class(bits, class_id) then
            e.self:Say("You already walk the path of the " .. class_name .. ". Choose another guild master if you still need more classes.")
            return
        end

        e.self:Say(
            "The path of the " .. class_name .. " will be added permanently to your trio if you accept it. " ..
            "If you are certain, tell me [" .. eq.say_link(confirm_phrase, true, "I choose the " .. class_name .. " path") .. "]."
        )
        return
    end

    if e.message:findi(confirm_phrase) then
        ensure_task(client)

        if count >= 3 then
            e.self:Say("Your trio is already complete. Return to the Bazaar Greeter when you are ready to leave.")
            return
        end

        if has_class(bits, class_id) then
            e.self:Say("You already walk the path of the " .. class_name .. ".")
            return
        end

        if client:AddExtraClass(class_id) then
            local new_count = class_count(client)
            client:Message(15, "You have permanently gained access to the " .. class_name .. " class.")

            if new_count >= 2 then
                update_if_active(client, 1)
            end
            if new_count >= 3 then
                update_if_active(client, 2)
            end

            if new_count >= 3 then
                e.self:Say("It is done. Your trio is now " .. class_summary(client) .. ". Return to the Bazaar Greeter.")
            else
                e.self:Say("A wise choice. You now walk as " .. class_summary(client) .. ". Speak with one more guild master to finish your trio.")
            end
        else
            e.self:Say("Something resisted the change. Return to the Bazaar Greeter and try again.")
        end
    end
end

function M.return_items(e)
    local item_lib = require("items")
    item_lib.return_items(e.self, e.other, e.trade)
end

return M
