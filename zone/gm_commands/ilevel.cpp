#include "../client.h"
#include "../../common/item_ilevel.h"
#include "../../common/rulesys.h"

void command_ilevel(Client *c, const Seperator *sep)
{
	// #ilevel            — show iLevel for all equipped items on target (or self)
	// #ilevel <item_id>  — show iLevel for a specific item by ID
	// #ilevel all        — batch-calculate iLevel for every item in the DB (GM only)

	if (sep->arg[1][0] != '\0' && strcasecmp(sep->arg[1], "all") == 0) {
		// ---- batch mode: calculate iLevel for all items ----
		c->Message(Chat::White, "ILEVEL | Batch calculating iLevel for all items...");

		auto results = database.QueryDatabase(
			"SELECT id FROM items WHERE id < 1000000000 ORDER BY id"
		);
		if (!results.Success() || !results.RowCount()) {
			c->Message(Chat::Red, "ILEVEL | Failed to query items table.");
			return;
		}

		int count = 0;
		int updated = 0;
		for (auto row = results.begin(); row != results.end(); ++row) {
			uint32 item_id = Strings::ToUnsignedInt(row[0]);
			const auto *item = database.GetItem(item_id);
			if (!item) {
				continue;
			}

			int ilevel = ItemProgression::CalculateILevel(item);
			++count;

			auto upd = database.QueryDatabase(
				fmt::format(
					"UPDATE items SET calculated_ilevel = {} WHERE id = {}",
					ilevel,
					item_id
				)
			);
			if (upd.Success()) {
				++updated;
			}
		}

		c->Message(
			Chat::White,
			fmt::format(
				"ILEVEL | Done. Processed {} items, updated {} rows.",
				count,
				updated
			).c_str()
		);
		return;
	}

	if (sep->arg[1][0] != '\0') {
		// ---- single item by ID ----
		uint32 item_id = Strings::ToUnsignedInt(sep->arg[1]);
		if (item_id == 0) {
			c->Message(Chat::Red, "ILEVEL | Usage: #ilevel [item_id|all]");
			return;
		}

		const auto *item = database.GetItem(item_id);
		if (!item) {
			c->Message(Chat::Red, fmt::format("ILEVEL | Item {} not found.", item_id).c_str());
			return;
		}

		int ilevel = ItemProgression::CalculateILevel(item);
		bool is_weapon = ItemProgression::IsWeaponType(item->ItemType);

		float dps = 0.0f;
		if (is_weapon && item->Delay > 0) {
			dps = static_cast<float>(item->Damage) / static_cast<float>(item->Delay);
		}

		c->Message(
			Chat::White,
			fmt::format(
				"ILEVEL | id={} name='{}' type={} iLevel={} weapon={} magic={}",
				item->ID,
				item->Name,
				item->ItemType,
				ilevel,
				is_weapon ? "true" : "false",
				item->Magic ? "true" : "false"
			).c_str()
		);

		if (is_weapon) {
			c->Message(
				Chat::White,
				fmt::format(
					"ILEVEL |   Dmg={} Delay={} DPS={:.2f} AC={} HP={} Mana={} Haste={} Attack={}",
					item->Damage,
					item->Delay,
					dps,
					item->AC,
					item->HP,
					item->Mana,
					item->Haste,
					item->Attack
				).c_str()
			);
		}
		else {
			c->Message(
				Chat::White,
				fmt::format(
					"ILEVEL |   AC={} HP={} Mana={} MR={} FR={} CR={} DR={} PR={}",
					item->AC,
					item->HP,
					item->Mana,
					static_cast<int>(item->MR),
					static_cast<int>(item->FR),
					static_cast<int>(item->CR),
					static_cast<int>(item->DR),
					static_cast<int>(item->PR)
				).c_str()
			);
		}

		// Show attributes + heroics
		int attrs = static_cast<int>(item->AStr) + static_cast<int>(item->ASta) +
		            static_cast<int>(item->AAgi) + static_cast<int>(item->ADex) +
		            static_cast<int>(item->AWis) + static_cast<int>(item->AInt) +
		            static_cast<int>(item->ACha);
		int heroics = item->HeroicStr + item->HeroicSta + item->HeroicAgi +
		              item->HeroicDex + item->HeroicWis + item->HeroicInt + item->HeroicCha;

		c->Message(
			Chat::White,
			fmt::format(
				"ILEVEL |   Attrs={} Heroics={} Essence={} TierCost(B>E)={} (E>L)={} (L>M)={}",
				attrs,
				heroics,
				ItemProgression::CalculateEssenceYield(item, 0),
				ItemProgression::CalculateTierCost(ilevel, 1),
				ItemProgression::CalculateTierCost(ilevel, 2),
				ItemProgression::CalculateTierCost(ilevel, 3)
			).c_str()
		);
		return;
	}

	// ---- no args: show equipped items on target ----
	Mob *t = c;
	if (c->GetTarget()) {
		t = c->GetTarget();
	}

	if (!t->IsClient()) {
		c->Message(Chat::White, "ILEVEL | Target must be a client.");
		return;
	}

	auto tc = t->CastToClient();
	c->Message(
		Chat::White,
		fmt::format(
			"ILEVEL | Equipped items for {} ({})",
			tc->GetCleanName(),
			tc->GetID()
		).c_str()
	);

	int total_ilevel = 0;
	int slot_count = 0;

	for (int16 slot_id = EQ::invslot::EQUIPMENT_BEGIN; slot_id <= EQ::invslot::EQUIPMENT_END; ++slot_id) {
		const auto *inst = tc->GetInv().GetItem(slot_id);
		if (!inst) {
			continue;
		}

		const auto *item = inst->GetUnscaledItem();
		if (!item) {
			continue;
		}

		int ilevel = ItemProgression::CalculateILevel(item);
		total_ilevel += ilevel;
		++slot_count;

		c->Message(
			Chat::White,
			fmt::format(
				"ILEVEL | slot={:2d} id={:6d} iLevel={:4d} '{}'",
				slot_id,
				item->ID,
				ilevel,
				item->Name
			).c_str()
		);
	}

	c->Message(
		Chat::White,
		fmt::format(
			"ILEVEL | {} slots, total iLevel={}, avg={:.1f}",
			slot_count,
			total_ilevel,
			slot_count > 0 ? static_cast<float>(total_ilevel) / slot_count : 0.0f
		).c_str()
	);
}
