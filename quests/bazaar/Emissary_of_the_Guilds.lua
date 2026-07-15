function event_say(e)
    if e.message:findi("hail") then
        e.self:Say("Class selection is handled by the guild masters around the Bazaar now. Speak with them, then return to the Bazaar Greeter.")
    end
end

function event_trade(e)
    local item_lib = require("items")
    item_lib.return_items(e.self, e.other, e.trade)
end
