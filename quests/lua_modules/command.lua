--copy or symbolic link this file to /server/lua_modules/, it will not work in /server/quests/lua_modules/

local commands_path = "lua_modules/commands/";
local commands      = { };
local dispatch_forwarding = false

commands["endurance"] = { 50,  require(commands_path .. "endurance") };
commands["lockouts"]  = { 0,   require(commands_path .. "lockouts") };
commands["timeleft"]  = { 0,   require(commands_path .. "time_left") };

function eq.DispatchCommands(e)
	if dispatch_forwarding then
		return 0;
	end

	local command = commands[e.command];

	if(command) then
		local access = command[1];
		if(access > e.self:Admin()) then
			e.self:Message(13, "Access level not high enough.");
			return 1;
		end

		local func = command[2];
		func(e);
		return 1;
	end

	-- Forward #test to server command handler so this module does not shadow it.
	-- If the running process does not have #test loaded yet, emit a clear message.
	if e.command == "test" and e.self and e.self.SendGMCommand then
		local command_line = "#" .. e.command;
		local args = e.args or {};
		for i = 1, #args do
			command_line = command_line .. " " .. tostring(args[i]);
		end

		dispatch_forwarding = true;
		local ok, handled = pcall(function()
			return e.self:SendGMCommand(command_line, true);
		end);
		dispatch_forwarding = false;

		if ok and handled then
			return 1;
		end

		e.self:Message(13, "#test is not loaded in this zone process. Restart world/zone to load the new command.");
		return 1;
	end

	return 0;
end
