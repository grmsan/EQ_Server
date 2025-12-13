#include "../client.h"

void command_damage(Client *c, const Seperator *sep)
{
	int arguments = sep->argnum;
	if (!arguments || !sep->IsNumber(1)) {
		c->Message(Chat::White, "Usage: #damage [Amount]");
		return;
	}

	Mob* target = c->GetTarget();
	if (!target) {
		c->Message(Chat::White, "You must have a target to use #damage.");
		return;
	}

	const auto damage = Strings::ToBigInt(sep->arg[1]);
	const auto before_hp = target->GetHP();
	target->Damage(c, damage, SPELL_UNKNOWN, EQ::skills::SkillHandtoHand, false);
	const auto after_hp = target->GetHP();

	c->Message(
		Chat::White,
		fmt::format(
			"DAMAGE | target={} ({}) requested={} hp={}/{} -> {}/{} invul={} divine_aura={}",
			target->GetCleanName(),
			target->GetID(),
			damage,
			before_hp,
			target->GetMaxHP(),
			after_hp,
			target->GetMaxHP(),
			target->GetInvul() ? "On" : "Off",
			target->DivineAura() ? "On" : "Off"
		).c_str()
	);
}
