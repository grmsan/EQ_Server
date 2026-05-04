-- ============================================================
-- Emissary of the Guilds (NPC 990201)
-- Allows players to add extra classes (multiclass system).
-- Shows the class selection menu on hail.
-- Players choose a class via saylinks; the class is added
-- immediately and they are told to return to the Greeter.
-- ============================================================

local CLASS_LIST = {
    { id = 1,  name = "Warrior"       },
    { id = 2,  name = "Cleric"        },
    { id = 3,  name = "Paladin"       },
    { id = 4,  name = "Ranger"        },
    { id = 5,  name = "Shadow Knight" },
    { id = 6,  name = "Druid"         },
    { id = 7,  name = "Monk"          },
    { id = 8,  name = "Bard"          },
    { id = 9,  name = "Rogue"         },
    { id = 10, name = "Shaman"        },
    { id = 11, name = "Necromancer"   },
    { id = 12, name = "Wizard"        },
    { id = 13, name = "Magician"      },
    { id = 14, name = "Enchanter"     },
    { id = 15, name = "Beastlord"     },
    { id = 16, name = "Berserker"     },
}

-- Helper: check if player already has this class bit
local function has_class(bits, class_id)
    return (bits & (1 << (class_id - 1))) ~= 0
end

-- Build and send the class selection menu
local function show_class_menu(e)
    local bits   = e.other:GetClassesBitmask()
    local pname  = e.other:GetName()

    e.self:Say(
        "Greetings, " .. pname .. ". " ..
        "I am the Emissary of the Guilds. " ..
        "Through my arts you may walk any path you choose. " ..
        "Which calling would you like to embrace?"
    )

    for _, c in ipairs(CLASS_LIST) do
        if has_class(bits, c.id) then
            e.other:Message(271, "  " .. c.name .. " (already mastered)")
        else
            local link = eq.saylink("choose_class_" .. c.id, true, c.name)
            e.other:Message(258, "  [" .. link .. "]")
        end
    end

    e.other:Message(
        335,
        "Your choice is permanent. Choose wisely, " .. pname .. "."
    )
end

function event_say(e)
    -- Hail: show the class selection menu
    if e.message:findi("hail") then
        show_class_menu(e)
        return
    end

    -- Handle class selection: "choose_class_N"
    local class_id = e.message:match("^choose_class_(%d+)$")
    if class_id then
        class_id = tonumber(class_id)
        if class_id < 1 or class_id > 16 then
            e.self:Say("I do not recognize that calling.")
            return
        end

        -- Validate they don't already have it
        local bits = e.other:GetClassesBitmask()
        if has_class(bits, class_id) then
            local class_name = CLASS_LIST[class_id] and CLASS_LIST[class_id].name or "that class"
            e.self:Say(
                "You already walk the path of the " .. class_name .. ". " ..
                "Choose another calling."
            )
            show_class_menu(e)
            return
        end

        -- Add the class
        local success = e.other:AddExtraClass(class_id)
        if success then
            local class_name = CLASS_LIST[class_id] and CLASS_LIST[class_id].name or "Unknown"
            e.other:Message(15, "The power of the " .. class_name .. " now flows through you!")
            e.self:Say(
                "The calling of the " .. class_name .. " is yours, " ..
                e.other:GetName() .. ". " ..
                "Return to the Bazaar Greeter nearby to continue your journey."
            )
        else
            e.self:Say(
                "The spirits resist this change. Please try again or speak to a GM for assistance."
            )
        end
        return
    end
end
