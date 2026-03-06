-- Step 10: Daily/Weekly Essence Quest completion rewards
-- Place in quests/global/player.lua (or append to existing)

function event_task_complete(e)
    if e.task_id == 600000 then
        e.self:AddAlternateCurrencyValue(100, 500)
        e.self:AddAlternateCurrencyValue(101, 20)
        e.self:Message(15, "You receive 500 Common Essence and 20 Rare Essence!")
    elseif e.task_id == 600001 then
        e.self:AddAlternateCurrencyValue(100, 2000)
        e.self:AddAlternateCurrencyValue(101, 150)
        e.self:Message(15, "You receive 2000 Common Essence and 150 Rare Essence!")
    end
end
