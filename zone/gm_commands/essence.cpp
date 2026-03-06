#include "../client.h"
#include "../../common/rulesys.h"

/*
 * #essence — GM command for Essence currency management.
 *
 * Players see their Essence balance in the Alternate Currency tab on the
 * character sheet.  This command is for admin testing and balance adjustment.
 *
 *   #essence           — show balance (self or target)
 *   #essence add N [R] — grant N Common Essence (and R Rare) to target
 *   #essence set N [R] — set exact balance
 */

void command_essence(Client *c, const Seperator *sep)
{
	Client *target = c;
	if (c->GetTarget() && c->GetTarget()->IsClient()) {
		target = c->GetTarget()->CastToClient();
	}

	const uint32 common_id = RuleI(ItemProgression, CommonEssenceCurrencyID);
	const uint32 rare_id   = RuleI(ItemProgression, RareEssenceCurrencyID);

	// Subcommand: add
	if (sep->arg[1][0] && !strcasecmp(sep->arg[1], "add") && sep->IsNumber(2)) {
		int common_amount = atoi(sep->arg[2]);
		int rare_amount   = sep->IsNumber(3) ? atoi(sep->arg[3]) : 0;

		if (common_amount != 0) {
			target->AddAlternateCurrencyValue(common_id, common_amount);
		}
		if (rare_amount != 0) {
			target->AddAlternateCurrencyValue(rare_id, rare_amount);
		}

		c->Message(Chat::Yellow, "Granted %d Common Essence and %d Rare Essence to %s.",
			common_amount, rare_amount, target->GetCleanName());
		return;
	}

	// Subcommand: set
	if (sep->arg[1][0] && !strcasecmp(sep->arg[1], "set") && sep->IsNumber(2)) {
		uint32 common_val = static_cast<uint32>(std::max(0, atoi(sep->arg[2])));
		uint32 rare_val   = sep->IsNumber(3) ? static_cast<uint32>(std::max(0, atoi(sep->arg[3]))) : 0;

		target->SetAlternateCurrencyValue(common_id, common_val);
		target->SetAlternateCurrencyValue(rare_id, rare_val);

		c->Message(Chat::Yellow, "Set %s's Essence to %u Common, %u Rare.",
			target->GetCleanName(), common_val, rare_val);
		return;
	}

	// Default: show balance
	uint32 common_bal = target->GetAlternateCurrencyValue(common_id);
	uint32 rare_bal   = target->GetAlternateCurrencyValue(rare_id);

	c->Message(Chat::Yellow, "--- Essence Balance for %s ---", target->GetCleanName());
	c->Message(Chat::Yellow, "  Common Essence: %u", common_bal);
	c->Message(Chat::Yellow, "  Rare Essence:   %u", rare_bal);
}
