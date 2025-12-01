#ifndef CLASSLESS_PACKET_H
#define CLASSLESS_PACKET_H

#include "types.h"

#define OP_EdgeStatLabel 0x1338


enum eStatEntry
{
	eStatClassless = 1,
	eStatCurHP,
	eStatCurMana,
	eStatCurEndur,
	eStatMaxHP,
	eStatMaxMana,
	eStatMaxEndur,
	eStatATK,
	eStatAC,
	eStatSTR,
	eStatSTA,
	eStatDEX,
	eStatAGI,
	eStatINT,
	eStatWIS,
	eStatCHA,
	eStatMR,
	eStatFR,
	eStatCR,
	eStatPR,
	eStatDR,
	eStatWalkspeed,
	eStatRunspeed,
	eStatWeight,
	eStatMaxWeight,
	eStatMeleePower,
	eStatSpellPower,
	eStatHealingPower,
	eStatMeleeHaste,
	eStatSpellHaste,
	eStatHealingHaste,
	eStatMeleeCrit,
	eStatSpellCrit,
	eStatHealingCrit,
	eStatTotalPower,
	eStatSynergyLevel,
	eStatMitigation,
	eStatAAPoints,
};

struct EdgeStatEntry_Struct {
	uint32_t statKey;
	uint64_t statValue;
};

struct EdgeStat_Struct
{
	uint32_t count;
	EdgeStatEntry_Struct entries[0];
};


#endif
