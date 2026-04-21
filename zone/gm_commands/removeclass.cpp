#include "../client.h"
#include "../../common/classes.h"
#include "../../common/rulesys.h"

void command_removeclass(Client *c, const Seperator *sep)
{
	const int arguments = sep->argnum;
	if (arguments < 1 || !sep->IsNumber(1)) {
		c->Message(Chat::White, "Usage: #removeclass [class_id] - Removes a class from your or your targeted player's multiclass bitmask (must leave at least one class).");
		return;
	}

	if (!RuleB(Custom, MulticlassingEnabled)) {
		c->Message(Chat::White, "Multiclassing is disabled. Enable rule Custom:MulticlassingEnabled first.");
		return;
	}

	Client *t = c;
	if (c->GetTarget() && c->GetTarget()->IsClient() && c->GetGM()) {
		t = c->GetTarget()->CastToClient();
	}

	const uint8 class_id = static_cast<uint8>(Strings::ToUnsignedInt(sep->arg[1]));
	if (!EQ::ValueWithin(class_id, 1, 16)) {
		c->Message(Chat::White, "Class ID must be between 1 and 16.");
		return;
	}

	if (!t->HasClass(class_id)) {
		c->Message(
			Chat::White,
			fmt::format(
				"{} does not have class {} ({}). classes_bitmask={} count={}",
				c->GetTargetDescription(t),
				static_cast<int>(class_id),
				GetClassIDName(class_id, t->GetLevel()),
				t->GetClassesBitmask(),
				static_cast<int>(t->GetClassesCount())
			).c_str()
		);
		return;
	}

	const uint8 before_count = t->GetClassesCount();
	const uint16 before_bits = t->GetClassesBitmask();
	if (before_count <= 1) {
		c->Message(Chat::White, "Cannot remove the last remaining class.");
		return;
	}

	if (!t->RemoveExtraClass(class_id)) {
		c->Message(
			Chat::White,
			fmt::format(
				"Failed to remove class {} ({}) from {}. classes_bitmask={} count={}",
				static_cast<int>(class_id),
				GetClassIDName(class_id, t->GetLevel()),
				c->GetTargetDescription(t),
				before_bits,
				static_cast<int>(before_count)
			).c_str()
		);
		return;
	}

	c->Message(
		Chat::White,
		fmt::format(
			"Removed class {} ({}) from {}. classes_bitmask {}->{} count {}->{}",
			static_cast<int>(class_id),
			GetClassIDName(class_id, t->GetLevel()),
			c->GetTargetDescription(t),
			before_bits,
			t->GetClassesBitmask(),
			static_cast<int>(before_count),
			static_cast<int>(t->GetClassesCount())
		).c_str()
	);
}

