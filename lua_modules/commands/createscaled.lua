-- #createscaled command - Create a scaled item for testing
-- Usage: #createscaled <base_id> <level>
-- Example: #createscaled 1001 50

function command_createscaled(e)
    local client = e.self
    local args = e.args

    local base_id = tonumber(args[1])
    local level = tonumber(args[2]) or 1

    if not base_id then
        client:Message(13, "Usage: #createscaled <base_id> <level>")
        client:Message(13, "Example: #createscaled 1001 50  (creates Cloth Cap +50)")
        return
    end

    if level < 0 or level > 99999 then
        client:Message(13, "[CreateScaled] Level must be between 0 and 99,999")
        return
    end

    -- Generate dynamic ID using inf.* function
    local dynamic_id = inf.generate_dynamic_id(base_id, level)

    client:Message(15, "[CreateScaled] Creating scaled item:")
    client:Message(15, "  Base ID: " .. base_id)
    client:Message(15, "  Level: +" .. level)
    client:Message(15, "  Dynamic ID: " .. dynamic_id)
    client:Message(15, "  Is Dynamic: " .. tostring(inf.is_dynamic_item(dynamic_id)))

    -- Summon the dynamic item
    client:SummonItem(dynamic_id)

    client:Message(15, "[CreateScaled] Item summoned - check your cursor!")
    client:Message(15, "[CreateScaled] Use #iteminfo cursor to see stats")
end

return command_createscaled
