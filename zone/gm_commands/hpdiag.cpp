#include "../client.h"
#include "../mob.h"
#include "../../common/opcodemgr.h"
#include "../../common/rulesys.h"

void command_hpdiag(Client *c, const Seperator *sep)
{
	Mob *t = c;
	if (c->GetTarget()) {
		t = c->GetTarget();
	}

	auto *eqs = c->Connection();
	auto *opm = eqs ? eqs->GetOpcodeManager() : nullptr;

	const uint16 hpupdate_eq = opm ? opm->EmuToEQ(OP_HPUpdate) : 0;
	const uint16 mobhealth_eq = opm ? opm->EmuToEQ(OP_MobHealth) : 0;
	const uint16 server_stats_eq = opm ? opm->EmuToEQ(OP_ServerStatsUpdate) : 0;

	c->Message(
		Chat::White,
		"%s",
		fmt::format(
			"HPDIAG | target={} ({}) server_hp={}/{} hp_ratio={:.2f} invul={} divine_aura={} gm_invul={}",
			t->GetCleanName(),
			t->GetID(),
			t->GetHP(),
			t->GetMaxHP(),
			static_cast<double>(t->GetHPRatio()),
			t->GetInvul() ? "On" : "Off",
			t->DivineAura() ? "On" : "Off",
			(t->IsClient() ? (t->CastToClient()->GetGMInvul() ? "On" : "Off") : "N/A")
		).c_str()
	);

	if (t->IsClient()) {
		auto tc = t->CastToClient();
		int custom_item_count = 0;
		for (int16 slot_id = EQ::invslot::EQUIPMENT_BEGIN; slot_id <= EQ::invslot::EQUIPMENT_END; ++slot_id) {
			const auto *inst = tc->GetInv().GetItem(slot_id);
			if (!inst) {
				continue;
			}
			if (!inst->GetCustomDataString().empty() || inst->IsScaling()) {
				++custom_item_count;
			}
		}

		c->Message(
			Chat::White,
			"%s",
			fmt::format(
				"HPDIAG | rules Combat:UseNewStaminaFormula={} Character:SoDClientUseSoDHPManaEnd={} Items:EnableCustomItemStats={} Items:SendCustomItemStatsToClient={} Character:EnableServerStatsUpdate={} Character:EnableEdgeStatLabel={} custom_equipped_items={}",
				RuleB(Combat, UseNewStaminaFormula) ? "true" : "false",
				RuleB(Character, SoDClientUseSoDHPManaEnd) ? "true" : "false",
				RuleB(Items, EnableCustomItemStats) ? "true" : "false",
				RuleB(Items, SendCustomItemStatsToClient) ? "true" : "false",
				RuleB(Character, EnableServerStatsUpdate) ? "true" : "false",
				RuleB(Character, EnableEdgeStatLabel) ? "true" : "false",
				custom_item_count
			).c_str()
		);

		const auto item = tc->GetItemBonuses();
		const auto spell = tc->GetSpellBonuses();
		const auto aa = tc->GetAABonuses();

		c->Message(
			Chat::White,
			"%s",
			fmt::format(
				"HPDIAG | breakdown level={} class={} sta={} itembonuses_hp={} flat_maxhp(aa/spell/item)={}/{}/{} pct_maxhp(aa/spell/item)={}/{}/{}",
				tc->GetLevel(),
				static_cast<int>(tc->GetClass()),
				tc->GetSTA(),
				item.HP,
				aa.FlatMaxHPChange,
				spell.FlatMaxHPChange,
				item.FlatMaxHPChange,
				aa.PercentMaxHPChange,
				spell.PercentMaxHPChange,
				item.PercentMaxHPChange
			).c_str()
		);
	}

	c->Message(
		Chat::White,
		"%s",
		fmt::format(
			"HPDIAG | client_version={} version_bit=0x{:x} OP_HPUpdate=0x{:04x} OP_MobHealth=0x{:04x} OP_ServerStatsUpdate=0x{:04x} EdgeStatLabel(custom)=0x1338",
			static_cast<int>(c->ClientVersion()),
			static_cast<uint64_t>(c->ClientVersionBit()),
			hpupdate_eq,
			mobhealth_eq,
			server_stats_eq
		).c_str()
	);

	// Force a self HP update to try to resync UI. For targets, normal percent packets are already used.
	if (t == c) {
		c->SendHPUpdate(true);
		c->Message(Chat::White, "HPDIAG | forced SendHPUpdate(true) to self");
	}
}
