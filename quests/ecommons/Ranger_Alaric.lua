function event_say(e)
	if(e.message:findi("hail")) then
		e.self:Say("Greetings, " .. e.other:GetCleanName() .. ". The balance of nature in these woods is disturbed. A dark corruption has taken hold of the wolves. Will you help me [cleanse] the woods?");
	elseif(e.message:findi("cleanse")) then
		e.self:Say("Excellent. Seek out the Corrupted Wolves wandering nearby and bring me a Corrupted Wolf Pelt as proof of your deed. I shall reward you.");
	end
end

function event_trade(e)
	local item_lib = require("items");

	if(item_lib.check_turn_in(e.trade, {item1 = 200000})) then
		e.self:Say("You have done well. The spirits of the wood thank you. Take this bow, may it serve you well.");
		e.other:QuestReward(e.self, 0, 0, 0, 0, 200001, 1000); -- Item 200001, 1000 XP
	end
	item_lib.return_items(e.self, e.other, e.trade)
end
