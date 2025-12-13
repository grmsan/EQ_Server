#include "../client.h"
#include "../../common/opcodemgr.h"
#include <algorithm>

static void SendRawHPUpdate(Client *c, uint16 eq_opcode, bool spawn_first, uint32 cur_hp, int32 max_hp)
{
	// RoF2 OP_HPUpdate payload is 10 bytes but ordering differs by client build.
	// We bypass struct encoding to test which ordering the stock client consumes.
	auto *outapp = new EQApplicationPacket(OP_HPUpdate, 10);
	outapp->SetOpcodeBypass(eq_opcode);

	auto *buf = outapp->pBuffer;

	if (spawn_first) {
		// [uint16 spawn_id][uint32 cur_hp][int32 max_hp]
		*reinterpret_cast<uint16 *>(buf + 0) = c->GetID();
		*reinterpret_cast<uint32 *>(buf + 2) = cur_hp;
		*reinterpret_cast<int32 *>(buf + 6) = max_hp;
	} else {
		// [uint32 cur_hp][int32 max_hp][uint16 spawn_id]
		*reinterpret_cast<uint32 *>(buf + 0) = cur_hp;
		*reinterpret_cast<int32 *>(buf + 4) = max_hp;
		*reinterpret_cast<uint16 *>(buf + 8) = c->GetID();
	}

	c->QueuePacket(outapp);
	safe_delete(outapp);
}

void command_hptest(Client *c, const Seperator *sep)
{
	auto *eqs = c->Connection();
	auto *opm = eqs ? eqs->GetOpcodeManager() : nullptr;
	const uint16 hpupdate_eq = opm ? opm->EmuToEQ(OP_HPUpdate) : 0;
	const uint16 mobhealth_eq = opm ? opm->EmuToEQ(OP_MobHealth) : 0;

	if (hpupdate_eq == 0) {
		c->Message(Chat::White, "HPTEST | OP_HPUpdate not mapped for this client");
		return;
	}

	const uint32 cur = static_cast<uint32>(c->GetHP() > 2000 ? (c->GetHP() - 2000) : c->GetHP());
	const int32 max = static_cast<int32>(c->GetMaxHP());

	// Usage:
	//   #hptest 1  -> send spawn-first HPUpdate
	//   #hptest 2  -> send cur/max-first HPUpdate
	//   #hptest 1 <cur> <max> -> send spawn-first with explicit values
	//   #hptest 2 <cur> <max> -> send cur/max-first with explicit values
	//   #hptest mob 50 -> send OP_MobHealth percent=50 to self
	if (sep->argnum >= 1 && sep->arg[1] && !strcasecmp(sep->arg[1], "mob")) {
		if (mobhealth_eq == 0) {
			c->Message(Chat::White, "HPTEST | OP_MobHealth not mapped for this client");
			return;
		}

		const int pct = (sep->argnum >= 2 && sep->IsNumber(2)) ? Strings::ToInt(sep->arg[2]) : 50;
		const uint8 hp_pct = static_cast<uint8>(std::clamp(pct, 1, 100));

		auto *outapp = new EQApplicationPacket(OP_MobHealth, 3);
		outapp->SetOpcodeBypass(mobhealth_eq);
		outapp->pBuffer[0] = static_cast<uint8>(c->GetID() & 0xFF);
		outapp->pBuffer[1] = static_cast<uint8>((c->GetID() >> 8) & 0xFF);
		outapp->pBuffer[2] = hp_pct;

		c->QueuePacket(outapp);
		safe_delete(outapp);

		c->Message(
			Chat::White,
			"%s",
			fmt::format(
				"HPTEST | sent raw OP_MobHealth opcode=0x{:04x} spawn={} hp_pct={}",
				mobhealth_eq,
				c->GetID(),
				hp_pct
			).c_str()
		);
		return;
	}

	const int mode = (sep->argnum >= 1 && sep->IsNumber(1)) ? Strings::ToInt(sep->arg[1]) : 1;
	const bool spawn_first = (mode != 2);

	uint32 send_cur = cur;
	int32 send_max = max;
	if (sep->argnum >= 3 && sep->IsNumber(2) && sep->IsNumber(3)) {
		send_cur = static_cast<uint32>(Strings::ToUnsignedInt(sep->arg[2]));
		send_max = static_cast<int32>(Strings::ToInt(sep->arg[3]));
	}

	SendRawHPUpdate(c, hpupdate_eq, spawn_first, send_cur, send_max);

	c->Message(
		Chat::White,
		"%s",
		fmt::format(
			"HPTEST | sent raw OP_HPUpdate opcode=0x{:04x} order={} cur={} max={} spawn={}",
			hpupdate_eq,
			spawn_first ? "spawn,cur,max" : "cur,max,spawn",
			send_cur,
			send_max,
			c->GetID()
		).c_str()
	);
}
