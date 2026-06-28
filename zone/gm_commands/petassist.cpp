#include "../client.h"

void command_petassist(Client *c, const Seperator *sep)
{
	Mob *pet = c->GetActivePet();
	if (!pet && c->GetTarget() && c->GetTarget()->IsPet() && c->GetTarget()->GetOwnerID() == c->GetID()) {
		pet = c->GetTarget();
	}
	if (!pet) {
		pet = c->GetPet();
	}

	if (!pet || !pet->IsNPC() || pet->IsFamiliar()) {
		c->Message(Chat::White, "You do not have a pet that can assist you.");
		return;
	}

	bool enable = !pet->IsPetAssisting();
	if (sep->arg[1]) {
		const std::string command_arg = Strings::ToLower(sep->arg[1]);
		if (command_arg == "on" || command_arg == "true" || command_arg == "1") {
			enable = true;
		}
		else if (command_arg == "off" || command_arg == "false" || command_arg == "0") {
			enable = false;
		}
		else if (command_arg == "status") {
			c->Message(Chat::White, "Pet assist is currently %s.", pet->IsPetAssisting() ? "enabled" : "disabled");
			return;
		}
		else {
			c->Message(Chat::White, "Usage: #petassist [on|off|status]");
			return;
		}
	}

	pet->CastToNPC()->DoPetCommandAssist(enable);
	c->Message(Chat::White, "Pet assist is now %s.", enable ? "enabled" : "disabled");
}
