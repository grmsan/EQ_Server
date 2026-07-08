-- Emissary of the Guilds (NPC 990201)
-- Adds the two extra classes used by the Bazaar new-player intro.

local TASK_ID = 600100

local CLASS_LIST = {
    { id = 1, name = "Warrior" },
    { id = 2, name = "Cleric" },
    { id = 3, name = "Paladin" },
    { id = 4, name = "Ranger" },
    { id = 5, name = "Shadow Knight" },
    { id = 6, name = "Druid" },
    { id = 7, name = "Monk" },
    { id = 8, name = "Bard" },
    { id = 9, name = "Rogue" },
    { id = 10, name = "Shaman" },
    { id = 11, name = "Necromancer" },
    { id = 12, name = "Wizard" },
    { id = 13, name = "Magician" },
    { id = 14, name = "Enchanter" },
    { id = 15, name = "Beastlord" },
    { id = 16, name = "Berserker" }
}

local CLASS_BY_ID = {}
for _, class_info in ipairs(CLASS_LIST) do
    CLASS_BY_ID[class_info.id] = class_info.name
end

local function has_class(bits, class_id)
    return (bits & (1 << (class_id - 1))) ~= 0
end

local function class_count(e)
    if e.other.GetClassesCount then
        return e.other:GetClassesCount()
    end

    local bits = e.other:GetClassesBitmask()
    local count = 0

    for class_id = 1, 16 do
        if has_class(bits, class_id) then
            count = count + 1
        end
    end

    return count
end

local function greeter_link()
    return eq.say_link("return to the Bazaar Greeter", true, "Bazaar Greeter")
end

local function show_class_menu(e)
    local bits = e.other:GetClassesBitmask()

    e.self:Say(
        "Greetings, " .. e.other:GetName() .. ". I speak for the guild masters gathered in this Bazaar. " ..
        "You may carry three callings in all. Choose the next class for your trio."
    )

    for _, class_info in ipairs(CLASS_LIST) do
        if has_class(bits, class_info.id) then
            e.other:Message(271, class_info.name .. " (already chosen)")
        else
            e.other:Message(258, "[" .. eq.say_link("choose_class_" .. class_info.id, true, class_info.name) .. "]")
        end
    end

    e.other:Message(335, "Class choices are permanent. Choose carefully.")
end

local function update_intro_task(e, count)
    if not e.other:IsTaskActive(TASK_ID) then
        return
    end

    if count >= 2 and e.other:IsTaskActivityActive(TASK_ID, 1) then
        e.other:UpdateTaskActivity(TASK_ID, 1, 1)
    end

    if count >= 3 and e.other:IsTaskActivityActive(TASK_ID, 2) then
        e.other:UpdateTaskActivity(TASK_ID, 2, 1)
    end
end

local function choose_class(e, class_id)
    if class_id < 1 or class_id > 16 then
        e.self:Say("I do not recognize that calling.")
        return
    end

    local count = class_count(e)
    if count >= 3 then
        e.self:Say("Your trio is already complete. Return to the [" .. greeter_link() .. "] and begin your work in the world.")
        return
    end

    local bits = e.other:GetClassesBitmask()
    local class_name = CLASS_BY_ID[class_id]

    if has_class(bits, class_id) then
        e.self:Say("You already walk the path of the " .. class_name .. ". Choose another calling.")
        show_class_menu(e)
        return
    end

    if e.other:AddExtraClass(class_id) then
        local new_count = class_count(e)
        e.other:Message(15, "You have permanently gained access to the " .. class_name .. " class.")
        update_intro_task(e, new_count)

        if new_count >= 3 then
            e.self:Say("It is done. Your trio is complete. Hurry back to the [" .. greeter_link() .. "]; you are needed beyond these walls.")
        else
            e.self:Say("A strong choice. You need one more class to complete your trio. Choose again when you are ready.")
            show_class_menu(e)
        end
    else
        e.self:Say("Something resisted the change. Speak with the Bazaar Greeter, then try again.")
    end
end

function event_say(e)
    if e.message:findi("hail") then
        if class_count(e) >= 3 then
            e.self:Say("Your trio is already complete. Return to the [" .. greeter_link() .. "] for your next step.")
            return
        end

        show_class_menu(e)
        return
    end

    local class_id = tonumber(e.message:match("^choose_class_(%d+)$"))
    if class_id then
        choose_class(e, class_id)
    end
end

function event_trade(e)
    local item_lib = require("items")
    item_lib.return_items(e.self, e.other, e.trade)
end

