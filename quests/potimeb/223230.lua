-- a_summoned_boar_ (223230) part of Rallos Zek p5 adds
function event_signal(e)

	if e.signal == 1 then -- Attack!
		local rz = eq.get_entity_list():GetMobByNpcTypeID(223168);
		if rz.valid then
			rz:CopyHateList(e.self);
		end
	end

end
