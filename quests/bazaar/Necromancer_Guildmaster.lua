local onboarding = require("bazaar_onboarding")

function event_say(e)
    onboarding.guildmaster_say(e, 11)
end

function event_trade(e)
    onboarding.return_items(e)
end
