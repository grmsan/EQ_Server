#include "../client.h"
#include "../../common/rulesys.h"

void command_itemdiag(Client *c, const Seperator *sep)
{
	Mob *t = c;
	if (c->GetTarget()) {
		t = c->GetTarget();
	}

	if (!t->IsClient()) {
		c->Message(Chat::White, "ITEMDIAG | target must be a client");
		return;
	}

	auto tc = t->CastToClient();

	c->Message(
		Chat::White,
		fmt::format(
			"ITEMDIAG | target={} ({}) enable_custom_item_stats={} send_custom_item_stats_to_client={}",
			tc->GetCleanName(),
			tc->GetID(),
			RuleB(Items, EnableCustomItemStats) ? "true" : "false",
			RuleB(Items, SendCustomItemStatsToClient) ? "true" : "false"
		).c_str()
	);

	int shown = 0;
	for (int16 slot_id = EQ::invslot::EQUIPMENT_BEGIN; slot_id <= EQ::invslot::EQUIPMENT_END; ++slot_id) {
		const auto *inst = tc->GetInv().GetItem(slot_id);
		if (!inst) {
			continue;
		}

		const auto *base = inst->GetUnscaledItem();
		const auto *scaled = inst->GetItem();
		if (!base || !scaled) {
			continue;
		}

		const bool scaled_diff =
			(base->HP != scaled->HP) ||
			(base->Mana != scaled->Mana) ||
			(base->Endur != scaled->Endur) ||
			(base->ASta != scaled->ASta) ||
			(base->AStr != scaled->AStr) ||
			(base->ADex != scaled->ADex) ||
			(base->AAgi != scaled->AAgi) ||
			(base->AInt != scaled->AInt) ||
			(base->AWis != scaled->AWis) ||
			(base->ACha != scaled->ACha);

		const auto custom = inst->GetCustomDataString();
		const bool interesting = scaled_diff || inst->IsScaling() || !custom.empty();
		if (!interesting) {
			continue;
		}

		++shown;
		c->Message(
			Chat::White,
			fmt::format(
				"ITEMDIAG | slot={} id={} name='{}' scaling={} custom_len={} HP {}->{} STA {}->{}",
				slot_id,
				base->ID,
				base->Name,
				inst->IsScaling() ? "true" : "false",
				static_cast<int>(custom.size()),
				base->HP,
				scaled->HP,
				static_cast<int>(base->ASta),
				static_cast<int>(scaled->ASta)
			).c_str()
		);
	}

	c->Message(
		Chat::White,
		fmt::format("ITEMDIAG | interesting_equipped_items={}", shown).c_str()
	);
}

