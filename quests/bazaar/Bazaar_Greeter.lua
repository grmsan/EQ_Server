-- Bazaar Greeter (NPC 990200)
-- New-player onboarding for the multiclass start in Bazaar.

local TASK_ID = 600100
local GREETER_NPC_ID = 990200

local CLASS_NAMES = {
    [1] = "Warrior", [2] = "Cleric", [3] = "Paladin", [4] = "Ranger",
    [5] = "Shadow Knight", [6] = "Druid", [7] = "Monk", [8] = "Bard",
    [9] = "Rogue", [10] = "Shaman", [11] = "Necromancer", [12] = "Wizard",
    [13] = "Magician", [14] = "Enchanter", [15] = "Beastlord", [16] = "Berserker"
}

local PET_CLASS_BAGS = {
    [5] = 899980, [6] = 899981, [8] = 899983, [10] = 899984,
    [11] = 899985, [13] = 899986, [14] = 899987, [15] = 899988
}

local START_POINTS = {
    [1] = { zone = 2, x = -51, y = 428, z = 3, h = 384, name = "North Qeynos" },
    [2] = { zone = 30, x = -682, y = 3139, z = -60, h = 384, name = "Everfrost Peaks" },
    [3] = { zone = 38, x = -916, y = -1510, z = -33, h = 0, name = "Toxxulia Forest" },
    [4] = { zone = 54, x = 10, y = -20, z = 0, h = 128, name = "Greater Faydark" },
    [5] = { zone = 54, x = 10, y = -20, z = 0, h = 128, name = "Greater Faydark" },
    [6] = { zone = 25, x = -965, y = 1838, z = 20, h = 128, name = "Nektulos Forest" },
    [7] = { zone = 2, x = -51, y = 428, z = 3, h = 384, name = "North Qeynos" },
    [8] = { zone = 68, x = -213, y = 2795, z = 3, h = 128, name = "Butcherblock Mountains" },
    [9] = { zone = 46, x = -588, y = -2192, z = -25, h = 128, name = "Innothule Swamp" },
    [10] = { zone = 47, x = 906, y = 1043, z = 25, h = 128, name = "The Feerrott" },
    [11] = { zone = 33, x = -770, y = 226, z = 4, h = 128, name = "Misty Thicket" },
    [12] = { zone = 56, x = -272, y = 159, z = -21, h = 128, name = "Steamfont Mountains" },
    [128] = { zone = 78, x = 1617, y = -1684, z = -54, h = 128, name = "Field of Bone" },
    [130] = { zone = 165, x = -301, y = -1822, z = -27, h = 128, name = "Shadeweaver Thicket" },
    [330] = { zone = 50, x = 421, y = 1185, z = 4, h = 128, name = "Rathe Mountains" },
    [522] = { zone = 394, x = -1520, y = -1470, z = -86, h = 128, name = "Crescent Reach" }
}

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

local function class_summary(e)
    local bits = e.other:GetClassesBitmask()
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

local function emissary_link()
    return eq.say_link("find Emissary of the Guilds", true, "Emissary of the Guilds")
end

local function return_link()
    return eq.say_link("send me home", true, "send me home")
end

local function ensure_task(e)
    if not e.other:IsTaskActive(TASK_ID) and not e.other:IsTaskCompleted(TASK_ID) then
        e.other:AssignTask(TASK_ID, GREETER_NPC_ID)
    end
end

local function update_if_active(e, activity_id)
    if e.other:IsTaskActivityActive(TASK_ID, activity_id) then
        e.other:UpdateTaskActivity(TASK_ID, activity_id, 1)
    end
end

local function grant_intro_rewards(e)
    if e.other:GetBucket("intro_quest") == "done" then
        return
    end

    e.other:SetBucket("intro_quest", "done")
    e.other:SummonFixedItem(22292)

    local bits = e.other:GetClassesBitmask()
    local pet_bag_count = 0

    for class_id, bag_id in pairs(PET_CLASS_BAGS) do
        if has_class(bits, class_id) then
            e.other:SummonFixedItem(bag_id)
            pet_bag_count = pet_bag_count + 1
        end
    end

    e.other:Message(15, "You receive an Adventurers Pack and 10 platinum.")

    if pet_bag_count > 0 then
        e.other:Message(15, "You also receive pet armory supplies for your pet-capable classes.")
    end
end

local function send_home(e)
    local start = START_POINTS[e.other:GetRace()] or START_POINTS[1]
    e.self:Say("Good. Back to " .. start.name .. " with you. Remember, Bazaar Gate brings you back here when you need trainers, vendors, or the way home again.")
    e.other:MovePC(start.zone, start.x, start.y, start.z, start.h)
end

local function handle_hail(e)
    local name = e.other:GetName()
    local count = class_count(e)

    ensure_task(e)
    update_if_active(e, 0)

    if count >= 2 then
        update_if_active(e, 1)
    end

    if count >= 3 then
        update_if_active(e, 2)
        update_if_active(e, 3)
        grant_intro_rewards(e)

        e.self:Say(
            "There you are, " .. name .. ". I have been waiting for you. " ..
            "Your trio is set: " .. class_summary(e) .. ". Hurry now, you are needed out in the world. " ..
            "Use your Bazaar Gate ability whenever you need to return here, then speak to me again when you want help returning home. " ..
            "If you are ready, I can [" .. return_link() .. "]."
        )
        return
    end

    if count == 2 then
        e.self:Say(
            "Good, " .. name .. ", you have taken a second path: " .. class_summary(e) .. ". " ..
            "You still need one more class to complete your trio. Speak with the [" .. emissary_link() .. "] again, choose carefully, then report back to me."
        )
        return
    end

    e.other:SetBucket("intro_quest", "1")
    e.self:Say(
        "Oh hey, there you are, " .. name .. ". I have been waiting for you. " ..
        "This Bazaar is your staging ground, but you are not meant to stand here lost. " ..
        "In this world you build a trio of classes. Start by speaking with the [" .. emissary_link() .. "] and choose two more callings to stand beside your first. " ..
        "Hurry up and choose; you are needed out in the world. " ..
        "You also have a Bazaar Gate ability. Use it any time you need to return to the Bazaar for trainers, vendors, or travel."
    )
end

function event_say(e)
    if e.message:findi("hail") then
        handle_hail(e)
        return
    end

    if e.message:findi("send me home") then
        if class_count(e) < 3 then
            e.self:Say("Not yet. Finish your three-class trio with the [" .. emissary_link() .. "] first, then I will send you out properly.")
            return
        end

        send_home(e)
    end
end

function event_trade(e)
    local item_lib = require("items")
    item_lib.return_items(e.self, e.other, e.trade)
end

