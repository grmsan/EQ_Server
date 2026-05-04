-- ============================================================
-- Bazaar Greeter (NPC 990200)
-- Guides new players through the multiclass intro quest chain.
--
-- Quest steps tracked via data bucket "intro_quest":
--   ""       = first time (show welcome and send to Emissary)
--   "1"      = waiting for first extra class (classes >= 2)
--   "2"      = waiting for second extra class (classes >= 3)
--   "done"   = already rewarded
-- ============================================================

local CLASS_NAMES = {
    [1]="Warrior",[2]="Cleric",[3]="Paladin",[4]="Ranger",
    [5]="Shadow Knight",[6]="Druid",[7]="Monk",[8]="Bard",
    [9]="Rogue",[10]="Shaman",[11]="Necromancer",[12]="Wizard",
    [13]="Magician",[14]="Enchanter",[15]="Beastlord",[16]="Berserker"
}

-- Pet-summoning classes → their class-specific Pet Armory bag item ID
-- These bags were created in utils/sql/custom/2026_02_26_thj_pet_bags.sql
local PET_CLASS_BAGS = {
    [5]  = 899980,  -- Shadowknight Pet Armory
    [6]  = 899981,  -- Druid Pet Armory
    [8]  = 899983,  -- Bard Pet Armory
    [10] = 899984,  -- Shaman Pet Armory
    [11] = 899985,  -- Necromancer Pet Armory
    [13] = 899986,  -- Magician Pet Armory
    [14] = 899987,  -- Enchanter Pet Armory
    [15] = 899988,  -- Beastlord Pet Armory
}

-- Mirrors the has_class() logic in Emissary_of_the_Guilds.lua
local function has_class(bits, class_id)
    return (bits & (1 << (class_id - 1))) ~= 0
end

-- Helper: return the display name for the player's primary class
local function primary_class_name(e)
    local id = e.other:GetClass()
    return CLASS_NAMES[id] or "Adventurer"
end

-- Friendly saylink pointing to the Emissary
local function emissary_link()
    return eq.saylink("find the Emissary of the Guilds", true, "Emissary of the Guilds")
end

function event_say(e)
    if not e.message:findi("hail") then return end

    local step  = e.other:GetBucket("intro_quest")
    local count = e.other:GetClassesCount()
    local pname = e.other:GetName()

    -- Already completed the intro chain
    if step == "done" then
        e.self:Say(
            "Welcome back, " .. pname .. "! " ..
            "The Emissary of the Guilds can expand your callings further whenever you are ready. " ..
            "Safe adventuring out there."
        )
        return
    end

    -- Step 2: waiting on second extra class
    if step == "2" then
        if count >= 3 then
            -- Reward!
            e.other:SetBucket("intro_quest", "done")
            e.self:Say(
                "You have mastered three callings, " .. pname .. "! " ..
                "Norrath holds no greater student of the versatile path. " ..
                "Accept this Adventurer's Pack for your travels."
            )
            -- Give starter bag (10-slot backpack) and some coin
            e.other:SummonFixedItem(22292)  -- Backpack (10-slot)
            e.other:AddMoneyToPP(0, 0, 0, 50, false)  -- 50 plat starter coin

            -- Give pet class bags for each summoner calling the player has chosen
            local bits = e.other:GetClassesBitmask()
            local pet_bag_count = 0
            for class_id, bag_id in pairs(PET_CLASS_BAGS) do
                if has_class(bits, class_id) then
                    e.other:SummonFixedItem(bag_id)
                    pet_bag_count = pet_bag_count + 1
                end
            end
            if pet_bag_count > 0 then
                e.other:Message(
                    15,
                    "You also receive a pet equipment bag for each of your summoner callings. " ..
                    "Use them to organize gear for your companions!"
                )
            end

            e.other:Message(
                15,
                "Tip: Ask Tearel nearby to travel to the classic lands of Norrath. " ..
                "Visit East Commonlands for a good starting adventure — merchants, " ..
                "bandits, and plenty of experience await a fresh adventurer."
            )
        else
            -- Not yet added second class
            e.self:Say(
                "You carry the power of two callings, " .. pname .. ". " ..
                "Return to the [" .. emissary_link() .. "] nearby and choose one more path. " ..
                "When you bear three callings, come back to me for your reward."
            )
        end
        return
    end

    -- Step 1: waiting on first extra class
    if step == "1" then
        if count >= 2 then
            -- First class added — advance to step 2
            e.other:SetBucket("intro_quest", "2")
            e.self:Say(
                "You have grown, " .. pname .. "! " ..
                "A single calling is strong, but two are stronger still. " ..
                "Visit the [" .. emissary_link() .. "] once more and embrace a third path. " ..
                "When you return to me with three callings, I will have a gift for you."
            )
        else
            -- Haven't visited the Emissary yet
            e.self:Say(
                "Your adventure begins as a " .. primary_class_name(e) .. ", " .. pname .. ". " ..
                "But here in this realm, a single calling need not define you. " ..
                "Visit the [" .. emissary_link() .. "] nearby to add a second calling to your path."
            )
        end
        return
    end

    -- First time greeting (step == "" or nil)
    e.other:SetBucket("intro_quest", "1")
    e.self:Say(
        "Welcome to the Bazaar, " .. pname .. "! " ..
        "I am your guide to adventure in these classic lands of Norrath. " ..
        "You arrive as a " .. primary_class_name(e) .. ", but true adventurers are never so simple. " ..
        "Seek out the [" .. emissary_link() .. "] nearby to add a second calling. " ..
        "Once you have done so, return to me for the next step of your journey."
    )
end
