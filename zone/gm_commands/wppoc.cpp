#include "../client.h"
#include "../../common/strings.h"

void command_wppoc(Client *c, const Seperator *sep)
{
	if (!c || !sep) {
		return;
	}

	const std::string sub = Strings::ToLower(sep->arg[1]);

	if (sub.empty() || sub == "list" || sub == "refresh") {
		c->SendWaypointList(true);
		c->Message(Chat::White, "Waypoint POC: sent waypoint list.");
		return;
	}

	if (sub == "travel") {
		const int32 waypoint_id = Strings::ToInt(sep->arg[2]);
		if (waypoint_id <= 0) {
			c->Message(Chat::White, "Usage: #wppoc travel [waypoint_id]");
			return;
		}

		c->TransportToWaypoint(static_cast<uint32>(waypoint_id));
		return;
	}

	if (sub == "expedition") {
		c->TransportToWaypoint(0);
		return;
	}

	c->Message(Chat::White, "Usage: #wppoc [list|travel <waypoint_id>|expedition]");
}
