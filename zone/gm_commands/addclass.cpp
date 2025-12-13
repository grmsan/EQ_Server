#include "../client.h"
#include "../../common/classes.h"
#include "../../common/rulesys.h"
#include "../../common/strings.h"

void command_addclass(Client *c, const Seperator *sep)
{
	const int arguments = sep->argnum;
	if (arguments < 1) {
		c->Message(Chat::White, "Usage: #addclass [class_id] | list - Adds a class, or lists class IDs (X = enabled).");
		return;
	}

	Client *t = c;
	if (c->GetTarget() && c->GetTarget()->IsClient() && c->GetGM()) {
		t = c->GetTarget()->CastToClient();
	}

	const std::string arg1 = sep->arg[1] ? sep->arg[1] : "";
	if (!arg1.empty() && Strings::ToLower(arg1) == "list") {
		const uint16 bits = t->GetClassesBitmask();
		c->Message(
			Chat::White,
			fmt::format(
				"Class IDs for {} (multiclass={} classes_bitmask=0x{:04X} ({}) count={} max_classes={})",
				c->GetTargetDescription(t),
				RuleB(Custom, MulticlassingEnabled) ? "On" : "Off",
				bits,
				bits,
				static_cast<int>(t->GetClassesCount()),
				RuleI(Custom, MulticlassMaxClasses)
			).c_str()
		);

		for (uint8 class_id = 1; class_id <= 16; ++class_id) {
			const bool enabled = t->HasClass(class_id);
			const bool is_base = (class_id == t->GetClass());
			c->Message(
				Chat::White,
				fmt::format(
					"{}: {}{}{}",
					static_cast<int>(class_id),
					GetClassIDName(class_id, t->GetLevel()),
					is_base ? " (base)" : "",
					enabled ? " X" : ""
				).c_str()
			);
		}

		return;
	}

	if (!sep->IsNumber(1)) {
		c->Message(Chat::White, "Usage: #addclass [class_id] | list - Adds a class, or lists class IDs (X = enabled).");
		return;
	}

	if (!RuleB(Custom, MulticlassingEnabled)) {
		c->Message(Chat::White, "Multiclassing is disabled. Enable rule Custom:MulticlassingEnabled first.");
		return;
	}

	const uint8 class_id = static_cast<uint8>(Strings::ToUnsignedInt(sep->arg[1]));
	if (!EQ::ValueWithin(class_id, 1, 16)) {
		c->Message(Chat::White, "Class ID must be between 1 and 16.");
		return;
	}

	if (t->HasClass(class_id)) {
		c->Message(
			Chat::White,
			fmt::format(
				"{} already has class {} ({}). classes_bitmask={} count={}",
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

	if (!t->AddExtraClass(class_id)) {
		c->Message(
			Chat::White,
			fmt::format(
				"Failed to add class {} ({}) to {}. classes_bitmask={} count={} max_classes={}",
				static_cast<int>(class_id),
				GetClassIDName(class_id, t->GetLevel()),
				c->GetTargetDescription(t),
				before_bits,
				static_cast<int>(before_count),
				RuleI(Custom, MulticlassMaxClasses)
			).c_str()
		);
		return;
	}

	c->Message(
		Chat::White,
		fmt::format(
			"Added class {} ({}) to {}. classes_bitmask {}->{} count {}->{}",
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
