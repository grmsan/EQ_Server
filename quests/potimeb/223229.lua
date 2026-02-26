-- a_summoned_minion_ (223229) part of Rallos Zek p5 adds
function event_signal(e)

        if e.signal == 1 then -- Attack!
                local rz = eq.get_entity_list():GetMobByNpcTypeID(223168);
                if rz.valid then
                        rz:CopyHateList(e.self);
                end
        end

end
