/*	EQEMu: Everquest Server Emulator
	Copyright (C) 2001-2004 EQEMu Development Team (http://eqemu.org)

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; version 2 of the License.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY except by those people which sell it, which
	are required to give you total support for your newly bought product;
	without even the implied warranty of MERCHANTABILITY or FITNESS FOR
	A PARTICULAR PURPOSE. See the GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

#include "../common/global_define.h"
#include "../common/spdat.h"
#include "../common/strings.h"

#include "../common/repositories/pets_repository.h"
#include "../common/repositories/pets_beastlord_data_repository.h"
#include "../common/repositories/character_pet_name_repository.h"

#include "entity.h"
#include "client.h"
#include "mob.h"
#include "npc.h"

#include "pets.h"
#include "zonedb.h"

#include <string>
#include <algorithm>

#include "bot.h"

#ifndef WIN32
#include <stdlib.h>
#include "../common/unix.h"
#endif

namespace {
	uint8 GetClassForFamiliar(uint16 spell_id)
	{
		for (uint8 class_id = Class::Warrior; class_id <= Class::Berserker; ++class_id) {
			if (GetSpellLevel(spell_id, class_id) != UINT8_MAX) {
				return class_id;
			}
		}

		return Class::None;
	}
}


// need to pass in a char array of 64 chars
void GetRandPetName(char *name)
{
	std::string temp;
	temp.reserve(64);
	// note these orders are used to make the exclusions cheap :P
	static const char *part1[] = {"G", "J", "K", "L", "V", "X", "Z"};
	static const char *part2[] = {nullptr, "ab", "ar", "as", "eb", "en", "ib", "ob", "on"};
	static const char *part3[] = {nullptr, "an", "ar", "ek", "ob"};
	static const char *part4[] = {"er", "ab", "n", "tik"};

	const char *first = part1[zone->random.Int(0, (sizeof(part1) / sizeof(const char *)) - 1)];
	const char *second = part2[zone->random.Int(0, (sizeof(part2) / sizeof(const char *)) - 1)];
	const char *third = part3[zone->random.Int(0, (sizeof(part3) / sizeof(const char *)) - 1)];
	const char *fourth = part4[zone->random.Int(0, (sizeof(part4) / sizeof(const char *)) - 1)];

	// if both of these are empty, we would get an illegally short name
	if (second == nullptr && third == nullptr)
		fourth = part4[(sizeof(part4) / sizeof(const char *)) - 1];

	// "ektik" isn't allowed either I guess?
	if (third == part3[3] && fourth == part4[3])
		fourth = part4[zone->random.Int(0, (sizeof(part4) / sizeof(const char *)) - 2)];

	// "Laser" isn't allowed either I guess?
	if (first == part1[3] && second == part2[3] && third == nullptr && fourth == part4[0])
		fourth = part4[zone->random.Int(1, (sizeof(part4) / sizeof(const char *)) - 2)];

	temp += first;
	if (second != nullptr)
		temp += second;
	if (third != nullptr)
		temp += third;
	temp += fourth;

	strn0cpy(name, temp.c_str(), 64);
}

void Mob::MakePet(uint16 spell_id, const char* pettype, const char *petname) {
	// petpower of -1 is used to get the petpower based on whichever focus is currently
	// equipped. This should replicate the old functionality for the most part.
	MakePoweredPet(spell_id, pettype, -1, petname);
}

NPC* Mob::GetFamiliar(uint16 spell_id)
{
	if (!IsClient() || !IsValidSpell(spell_id) || !IsEffectInSpell(spell_id, SpellEffect::Familiar)) {
		return nullptr;
	}

	const uint8 familiar_class_id = GetClassForFamiliar(spell_id);
	for (const auto& entry : entity_list.GetNPCList()) {
		auto* npc = entry.second;
		if (!npc || !npc->IsFamiliar()) {
			continue;
		}

		const bool same_owner =
			(npc->GetOwnerID() == GetID()) ||
			(npc->GetSwarmInfo() && npc->GetSwarmInfo()->owner_id == GetID());
		if (!same_owner) {
			continue;
		}

		if (familiar_class_id != Class::None) {
			if (GetClassForFamiliar(npc->GetPetSpellID()) == familiar_class_id) {
				return npc;
			}
		} else if (npc->GetPetSpellID() == spell_id) {
			return npc;
		}
	}

	return nullptr;
}

bool Mob::CheckFamiliarConflict(uint16 spell_id)
{
	return GetFamiliar(spell_id) != nullptr;
}

void Mob::DismissFamiliar(uint16 spell_id)
{
	auto* familiar = GetFamiliar(spell_id);
	if (familiar) {
		familiar->Depop();
	}
}

void Mob::MakeFamiliar(uint16 spell_id)
{
	if (!IsClient() || CheckFamiliarConflict(spell_id)) {
		return;
	}

	PetRecord record;
	if (!content_db.GetPoweredPetEntry(spells[spell_id].teleport_zone, 0, &record)) {
		LogError("Unknown familiar pet spell id: [{}], check pets table", spell_id);
		Message(Chat::Red, "Unable to find data for pet %s", spells[spell_id].teleport_zone);
		return;
	}

	const auto* npc_type = content_db.LoadNPCTypesData(record.npc_type);
	if (!npc_type) {
		LogError("Unknown npc type for familiar pet spell id: [{}]", spell_id);
		return;
	}

	static const glm::vec2 familiar_locations[MAX_SWARM_PETS] = {
		glm::vec2(5, 5), glm::vec2(-5, 5), glm::vec2(5, -5), glm::vec2(-5, -5),
		glm::vec2(10, 10), glm::vec2(-10, 10), glm::vec2(10, -10), glm::vec2(-10, -10),
		glm::vec2(8, 8), glm::vec2(-8, 8), glm::vec2(8, -8), glm::vec2(-8, -8)
	};

	auto* familiar = new NPC(
		npc_type,
		0,
		GetPosition() + glm::vec4(familiar_locations[0], 0.0f, 0.0f),
		GravityBehavior::Ground
	);

	std::string familiar_name = std::string(GetCleanName()) + "`s_Familiar";
	strn0cpy(familiar->name, familiar_name.c_str(), sizeof(familiar->name));
	entity_list.MakeNameUnique(familiar->name);

	// Familiars are not added to the owner's controllable pet list, but they still
	// need normal ownership semantics for pet-aware systems.
	familiar->SetOwnerID(GetID());
	familiar->SetFollowID(GetID());
	familiar->SetPetType(PetType::Familiar);
	familiar->SetPetSpellID(spell_id);

	if (!familiar->GetSwarmInfo()) {
		auto* swarm_info = new SwarmPet;
		familiar->SetSwarmInfo(swarm_info);
		familiar->GetSwarmInfo()->duration = new Timer(INT32_MAX);
	} else {
		familiar->GetSwarmInfo()->duration->Start(INT32_MAX);
	}

	familiar->StartSwarmTimer(INT32_MAX);
	familiar->GetSwarmInfo()->owner_id = GetUltimateOwner()->GetID();

	familiar->SetSpecialAbility(SpecialAbility::SlowImmunity, 1);
	familiar->SetSpecialAbility(SpecialAbility::CharmImmunity, 1);
	familiar->SetSpecialAbility(SpecialAbility::SnareImmunity, 1);
	familiar->SetSpecialAbility(SpecialAbility::DispellImmunity, 1);
	familiar->SetSpecialAbility(SpecialAbility::MeleeImmunity, 1);
	familiar->SetSpecialAbility(SpecialAbility::MagicImmunity, 1);
	familiar->SetSpecialAbility(SpecialAbility::FleeingImmunity, 1);
	familiar->SetSpecialAbility(SpecialAbility::MeleeImmunityExceptBane, 1);
	familiar->SetSpecialAbility(SpecialAbility::MeleeImmunityExceptMagical, 1);
	familiar->SetSpecialAbility(SpecialAbility::AggroImmunity, 1);
	familiar->SetSpecialAbility(SpecialAbility::BeingAggroImmunity, 1);
	familiar->SetSpecialAbility(SpecialAbility::CastingFromRangeImmunity, 1);
	familiar->SetSpecialAbility(SpecialAbility::HarmFromClientImmunity, 1);
	familiar->SetSpecialAbility(SpecialAbility::RangedAttackImmunity, 1);
	familiar->SetSpecialAbility(SpecialAbility::ClientDamageImmunity, 1);
	familiar->SetSpecialAbility(SpecialAbility::NPCDamageImmunity, 1);
	familiar->SetSpecialAbility(SpecialAbility::ClientAggroImmunity, 1);
	familiar->SetSpecialAbility(SpecialAbility::NPCAggroImmunity, 1);
	familiar->SetSpecialAbility(SpecialAbility::MemoryFadeImmunity, 1);
	familiar->SetSpecialAbility(SpecialAbility::OpenImmunity, 1);
	familiar->SetSpecialAbility(SpecialAbility::AssassinateImmunity, 1);
	familiar->SetSpecialAbility(SpecialAbility::HeadshotImmunity, 1);
	familiar->SetSpecialAbility(SpecialAbility::BotAggroImmunity, 1);
	familiar->SetSpecialAbility(SpecialAbility::BotDamageImmunity, 1);

	entity_list.AddNPC(familiar, true, true);
}

// Split from the basic MakePet to allow backward compatiblity with existing code while also
// making it possible for petpower to be retained without the focus item having to
// stay equipped when the character zones. petpower of -1 means that the currently equipped petfocus
// of a client is searched for and used instead.
void Mob::MakePoweredPet(uint16 spell_id, const char* pettype, int16 petpower,
		const char *petname, float in_size) {
	// Sanity and early out checking first.
	ValidatePetList();
	if (pettype == nullptr || petids.size() >= RuleI(Custom, AbsolutePetLimit))
		return;

	int16 act_power = 0; // The actual pet power we'll use.
	if (petpower == -1) {
		if (IsClient()) {
			act_power = CastToClient()->GetFocusEffect(focusPetPower, spell_id);//Client only
		}
		else if (IsBot())
			act_power = CastToBot()->GetFocusEffect(focusPetPower, spell_id);
	}
	else if (petpower > 0)
		act_power = petpower;

	// optional rule: classic style variance in pets. Achieve this by
	// adding a random 0-4 to pet power, since it only comes in increments
	// of five from focus effects.

	//lookup our pets table record for this type
	PetRecord record;
	if(!content_db.GetPoweredPetEntry(pettype, act_power, &record)) {
		Message(Chat::Red, "Unable to find data for pet %s", pettype);
		LogError("Unable to find data for pet [{}], check pets table", pettype);
		return;
	}

	//find the NPC data for the specified NPC type
	const NPCType *base = content_db.LoadNPCTypesData(record.npc_type);
	if(base == nullptr) {
		Message(Chat::Red, "Unable to load NPC data for pet %s", pettype);
		LogError("Unable to load NPC data for pet [{}] (NPC ID [{}]), check pets and npc_types tables", pettype, record.npc_type);
		return;
	}

	//we copy the npc_type data because we need to edit it a bit
	auto npc_type = new NPCType;
	memcpy(npc_type, base, sizeof(NPCType));

	// If pet power is set to -1 in the DB, use stat scaling
	if ((IsClient() || IsBot()) && record.petpower == -1)
	{
		float scale_power = (float)act_power / 100.0f;
		if(scale_power > 0)
		{
			npc_type->max_hp *= (1 + scale_power);
			npc_type->current_hp = npc_type->max_hp;
			npc_type->AC *= (1 + scale_power);
			npc_type->level += 1 + ((int)act_power / 25) > npc_type->level + RuleR(Pets, PetPowerLevelCap) ? RuleR(Pets, PetPowerLevelCap) : 1 + ((int)act_power / 25); // gains an additional level for every 25 pet power
			npc_type->min_dmg = (npc_type->min_dmg * (1 + (scale_power / 2)));
			npc_type->max_dmg = (npc_type->max_dmg * (1 + (scale_power / 2)));
			npc_type->size = npc_type->size * (1 + (scale_power / 2)) > npc_type->size * 3 ? npc_type->size * 3 : npc_type-> size * (1 + (scale_power / 2));
		}
		record.petpower = act_power;
	}

	//Live AA - Elemental Durability
	int64 MaxHP = aabonuses.PetMaxHP + itembonuses.PetMaxHP + spellbonuses.PetMaxHP;

	if (MaxHP){
		npc_type->max_hp += (npc_type->max_hp*MaxHP)/100;
		npc_type->current_hp = npc_type->max_hp;
	}

	//TODO: think about regen (engaged vs. not engaged)

	// Pet naming:
	// 0 - `s pet
	// 1 - `s familiar
	// 2 - `s Warder
	// 3 - Random name if client, `s pet for others
	// 4 - Keep DB name
	// 5 - `s ward

	const auto vanity_name = (IsClient() && !petname) ? CharacterPetNameRepository::FindOne(database, CastToClient()->CharacterID()) : CharacterPetNameRepository::CharacterPetName{};

	if (
		IsClient() &&
		!petname &&
		!vanity_name.name.empty()
	) {
		petname = vanity_name.name.c_str();
	}

	if (petname != nullptr) {
		// Name was provided, use it.
		strn0cpy(npc_type->name, petname, 64);
		EntityList::RemoveNumbers(npc_type->name);
		entity_list.MakeNameUnique(npc_type->name);
	} else if (record.petnaming == 0) {
		strcpy(npc_type->name, GetCleanName());
		npc_type->name[25] = '\0';
		strcat(npc_type->name, "`s_pet");
	} else if (record.petnaming == 1) {
		strcpy(npc_type->name, GetName());
		npc_type->name[19] = '\0';
		strcat(npc_type->name, "`s_familiar");
	} else if (record.petnaming == 2) {
		strcpy(npc_type->name, GetName());
		npc_type->name[21] = 0;
		strcat(npc_type->name, "`s_Warder");
	} else if (record.petnaming == 4) {
		// Keep the DB name
	} else if (record.petnaming == 3 && IsClient()) {
		GetRandPetName(npc_type->name);
	} else if (record.petnaming == 5 && IsClient()) {
		strcpy(npc_type->name, GetName());
		npc_type->name[24] = '\0';
		strcat(npc_type->name, "`s_ward");
	} else {
		strcpy(npc_type->name, GetCleanName());
		npc_type->name[25] = '\0';
		strcat(npc_type->name, "`s_pet");
	}

	// Beastlord Pets
	if (record.petnaming == 2) {
		uint16 race_id = GetBaseRace();

		auto d = content_db.GetBeastlordPetData(race_id);

		npc_type->race        = d.race_id;
		npc_type->texture     = d.texture;
		npc_type->helmtexture = d.helm_texture;
		npc_type->gender      = d.gender;
		npc_type->luclinface  = d.face;

		npc_type->size *= d.size_modifier;
	}

	// handle monster summoning pet appearance
	if(record.monsterflag) {

		uint32 monsterid = 0;

		// get a random npc id from the spawngroups assigned to this zone
		auto query = StringFormat("SELECT npcID "
									"FROM (spawnentry INNER JOIN spawn2 ON spawn2.spawngroupID = spawnentry.spawngroupID) "
									"INNER JOIN npc_types ON npc_types.id = spawnentry.npcID "
									"WHERE spawn2.zone = '%s' AND npc_types.bodytype NOT IN (11, 33, 66, 67) "
									"AND npc_types.race NOT IN (0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 44, "
									"55, 67, 71, 72, 73, 77, 78, 81, 90, 92, 93, 94, 106, 112, 114, 127, 128, "
									"130, 139, 141, 183, 236, 237, 238, 239, 254, 266, 329, 330, 378, 379, "
									"380, 381, 382, 383, 404, 522) "
									"ORDER BY RAND() LIMIT 1", zone->GetShortName());
		auto results = content_db.QueryDatabase(query);
		if (!results.Success()) {
			safe_delete(npc_type);
			return;
		}

		if (results.RowCount() != 0) {
			auto row = results.begin();
			monsterid = Strings::ToInt(row[0]);
		}

		// since we don't have any monsters, just make it look like an earth pet for now
		if (monsterid == 0)
			monsterid = 567;

		// give the summoned pet the attributes of the monster we found
		const NPCType* monster = content_db.LoadNPCTypesData(monsterid);
		if(monster) {
			npc_type->race = monster->race;
			npc_type->size = monster->size;
			npc_type->texture = monster->texture;
			npc_type->gender = monster->gender;
			npc_type->luclinface = monster->luclinface;
			npc_type->helmtexture = monster->helmtexture;
			npc_type->herosforgemodel = monster->herosforgemodel;
		} else
			LogError("Error loading NPC data for monster summoning pet (NPC ID [{}])", monsterid);

	}

	//this takes ownership of the npc_type data
	auto npc = new Pet(npc_type, this, record.petcontrol, spell_id, record.petpower);

	// Now that we have an actual object to interact with, load
	// the base items for the pet. These are always loaded
	// so that a rank 1 suspend minion does not kill things
	// like the special back items some focused pets may receive.
	uint32 petinv[EQ::invslot::EQUIPMENT_COUNT];
	memset(petinv, 0, sizeof(petinv));
	const EQ::ItemData *item = nullptr;

	if (content_db.GetBasePetItems(record.equipmentset, petinv)) {
		for (int i = EQ::invslot::EQUIPMENT_BEGIN; i <= EQ::invslot::EQUIPMENT_END; i++)
			if (petinv[i]) {
				item = database.GetItem(petinv[i]);
				npc->AddLootDrop(item, LootdropEntriesRepository::NewNpcEntity(), true);
			}
	}

	npc->UpdateEquipmentLight();

	// finally, override size if one was provided
	if (in_size > 0.0f)
		npc->size = in_size;

	entity_list.AddNPC(npc, true, true);
	SetPetID(npc->GetID());
	npc->ConfigureInitialCommands();
	if (IsClient()) {
		ConfigurePetWindow(npc);
	}
	// We need to handle PetType 5 (petHatelist), add the current target to the hatelist of the pet

	if (record.petcontrol == PetType::TargetLock)
	{
		Mob* m_target = GetTarget();

		bool activiate_pet = false;
		if (m_target && m_target->GetID() != GetID()) {

			if (spells[spell_id].target_type == ST_Self) {
				float distance = CalculateDistance(m_target->GetX(), m_target->GetY(), m_target->GetZ());
				if (distance <= 200) { //Live distance on targetlock pets that self cast. No message is given if not in range.
					activiate_pet = true;
				}
			}
			else {
				activiate_pet = true;
			}
		}

		if (activiate_pet){
			npc->AddToHateList(m_target, 1);
			npc->SetPetTargetLockID(m_target->GetID());
			npc->SetSpecialAbility(SpecialAbility::AggroImmunity, 1);
		}
		else {
			npc->CastSpell(SPELL_UNSUMMON_SELF, npc->GetID()); //Live like behavior, damages self for 20K
			if (!npc->HasDied()) {
				npc->Kill(); //Ensure pet dies if over 20k HP.
			}
		}
	}
}

void NPC::TryDepopTargetLockedPets(Mob* current_target) {

	if (!current_target || (current_target && (current_target->GetID() != GetPetTargetLockID()) || current_target->IsCorpse())) {

		//Use when swarmpets are set to auto lock from quest or rule
		if (GetSwarmInfo() && GetSwarmInfo()->target) {
			Mob* owner = entity_list.GetMobID(GetSwarmInfo()->owner_id);
			if (owner) {
				owner->SetTempPetCount(owner->GetTempPetCount() - 1);
			}
			Depop();
			return;
		}
		//Use when pets are given petype 5
		if (IsPet() && GetPetType() == PetType::TargetLock && GetPetTargetLockID()) {
			CastSpell(SPELL_UNSUMMON_SELF, GetID()); //Live like behavior, damages self for 20K
			if (!HasDied()) {
				Kill(); //Ensure pet dies if over 20k HP.
			}
			return;
		}
	}
}



/* This is why the pets ghost - pets were being spawned too far away from its npc owner and some
into walls or objects (+10), this sometimes creates the "ghost" effect. I changed to +2 (as close as I
could get while it still looked good). I also noticed this can happen if an NPC is spawned on the same spot of another or in a related bad spot.*/
Pet::Pet(NPCType *type_data, Mob *owner, uint8 pet_type, uint16 spell_id, int16 power)
: NPC(type_data, 0, owner->GetPosition() + glm::vec4(2.0f, 2.0f, 0.0f, 0.0f), GravityBehavior::Water)
{
	GiveNPCTypeData(type_data);
	SetPetType(pet_type);
	SetPetPower(power);
	SetOwnerID(owner ? owner->GetID() : 0);
	SetPetSpellID(spell_id);

	// All pets start at false on newer clients. The client
	// turns it on and tracks the state.
	SetTaunting(false);

	// Older clients didn't track state, and default taunting is on (per @mackal)
	// Familiar and animation pets don't get taunt until an AA.
	if (owner && owner->IsClient()) {
		if (!(owner->CastToClient()->ClientVersionBit() & EQ::versions::maskUFAndLater)) {
			if (
				(GetPetType() != PetType::Familiar && GetPetType() != PetType::Animation) ||
				aabonuses.PetCommands[PetCommand::Taunt]
			) {
				SetTaunting(true);
			}
		}
	}

	// Class should use npc constructor to set light properties
}

bool ZoneDatabase::GetPetEntry(const std::string& pet_type, PetRecord *p)
{
	return GetPoweredPetEntry(pet_type, 0, p);
}

bool ZoneDatabase::GetPoweredPetEntry(const std::string& pet_type, int16 pet_power, PetRecord* r)
{
	const auto& l = PetsRepository::GetWhere(
		content_db,
		fmt::format(
			"`type` = '{}' AND `petpower` <= {} ORDER BY `petpower` DESC LIMIT 1",
			pet_type,
			pet_power <= 0 ? 0 : pet_power
		)
	);

	if (l.empty()) {
		return false;
	}

	auto &e = l.front();

	r->npc_type     = e.npcID;
	r->temporary    = e.temp;
	r->petpower     = e.petpower;
	r->petcontrol   = e.petcontrol;
	r->petnaming    = e.petnaming;
	r->monsterflag  = e.monsterflag;
	r->equipmentset = e.equipmentset;

	return true;
}

uint16 Mob::GetPetID(uint8 idx) const
{
	if (idx < petids.size()) {
		return petids[idx];
	}

	if (idx == 0) {
		return petid;
	}

	return 0;
}

bool Mob::IsPetAllowed(uint16 spell_id)
{
	ValidatePetList();
	const int pet_count = static_cast<int>(petids.size());
	int cumulative_bitmask = 0;

	if (pet_count >= RuleI(Custom, AbsolutePetLimit)) {
		Message(Chat::SpellFailure, "You may not control any additional pets.");
		return false;
	}

	for (auto pet : GetAllPets()) {
		if (!pet) {
			continue;
		}

		uint16 origin_spell = 0;
		auto pet_buffs = pet->GetBuffs();

		for (int i = 0; i < pet->GetMaxTotalSlots(); i++) {
			if (IsCharmSpell(pet_buffs[i].spellid)) {
				origin_spell = pet_buffs[i].spellid;
				break;
			}
		}

		if (!origin_spell && pet->IsNPC()) {
			origin_spell = pet->CastToNPC()->GetPetSpellID();
		}

		if (origin_spell == spell_id) {
			Message(Chat::SpellFailure, "You may not control any additional pets of this type (%s).", spells[spell_id].name);
			return false;
		}

		for (int i = Class::Warrior; i <= Class::Berserker; i++) {
			if (GetSpellLevel(origin_spell, i) < UINT8_MAX) {
				cumulative_bitmask |= (1 << i);
			}
		}
	}

	for (int i = Class::Warrior; i <= Class::Berserker; i++) {
		if (GetSpellLevel(spell_id, i) < UINT8_MAX) {
			if (cumulative_bitmask & (1 << i)) {
				Message(Chat::SpellFailure, "You may not control any additional pets for this class (%s).", GetClassIDName(i));
				return false;
			}
		}
	}

	return true;
}

void Mob::ValidatePetList()
{
	for (auto it = petids.begin(); it != petids.end();) {
		auto pet = entity_list.GetMob(*it);
		if (!pet || pet->GetOwnerID() != GetID()) {
			it = petids.erase(it);
		} else {
			++it;
		}
	}

	if (petid != 0) {
		auto legacy_pet = entity_list.GetMob(petid);
		if (!legacy_pet || legacy_pet->GetOwnerID() != GetID()) {
			petid = 0;
		}
	}

	if (petid != 0) {
		if (std::find(petids.begin(), petids.end(), petid) == petids.end()) {
			petids.insert(petids.begin(), petid);
		}
	} else if (!petids.empty()) {
		petid = petids.front();
	}

	if (focused_pet_id && std::find(petids.begin(), petids.end(), focused_pet_id) == petids.end()) {
		focused_pet_id = 0;
	}
}

Mob* Mob::GetPet(uint8 idx)
{
	ValidatePetList();
	auto id = GetPetID(idx);
	if (!id) {
		return nullptr;
	}

	auto m = entity_list.GetMob(id);
	if (!m || m->GetOwnerID() != GetID()) {
		ValidatePetList();
		return nullptr;
	}

	return m;
}

Mob* Mob::GetActivePet()
{
	if (!IsClient()) {
		return GetPet();
	}

	if (!focused_pet_id) {
		return GetPet();
	}

	auto focused = GetPetByID(focused_pet_id);
	return focused ? focused : GetPet();
}

std::vector<Mob*> Mob::GetAllPets()
{
	ValidatePetList();
	std::vector<Mob*> pets;
	pets.reserve(petids.size());
	for (auto id : petids) {
		auto pet = entity_list.GetMob(id);
		if (pet && pet->GetOwnerID() == GetID()) {
			pets.push_back(pet);
		}
	}
	return pets;
}

std::vector<Mob*> Mob::GetAllSwarmPets()
{
	std::vector<Mob*> swarm_list;
	for (auto e : entity_list.GetNPCList()) {
		if (e.second && e.second->GetSwarmOwner() == GetID() && e.second->GetSwarmInfo()) {
			swarm_list.push_back(e.second);
		}
	}

	return swarm_list;
}

Mob* Mob::GetPetByID(uint16 id)
{
	ValidatePetList();
	for (uint16 pet_id : petids) {
		if (pet_id == id) {
			auto pet = entity_list.GetMob(pet_id);
			if (pet && pet->GetOwnerID() == GetID()) {
				return pet;
			}
			RemovePet(pet_id);
			break;
		}
	}

	return nullptr;
}

bool Mob::RemovePetByIndex(uint8 idx)
{
	if (idx >= petids.size()) {
		return false;
	}

	auto m = entity_list.GetMob(GetPetID(idx));
	if (m) {
		m->SetOwnerID(0);
	}

	petids.erase(petids.begin() + idx);
	petid = petids.empty() ? 0 : petids.front();
	if (focused_pet_id && std::find(petids.begin(), petids.end(), focused_pet_id) == petids.end()) {
		focused_pet_id = 0;
	}

	return true;
}

bool Mob::RemovePet(Mob* pet)
{
	if (!pet) {
		return false;
	}

	return RemovePet(pet->GetID());
}

bool Mob::RemovePet(uint16 pet_id_to_remove)
{
	for (auto it = petids.begin(); it != petids.end(); ++it) {
		if (*it == pet_id_to_remove) {
			auto pet = entity_list.GetMob(pet_id_to_remove);
			if (pet) {
				pet->SetOwnerID(0);
				pet->SendAppearancePacket(AppearanceType::Pet, 0, true, true);
			}
			petids.erase(it);
			break;
		}
	}

	petid = petids.empty() ? 0 : petids.front();
	if (focused_pet_id == pet_id_to_remove) {
		focused_pet_id = 0;
	}

	return true;
}

void Mob::RemoveAllPets()
{
	for (auto pet_id_to_remove : petids) {
		auto pet = entity_list.GetMob(pet_id_to_remove);
		if (pet) {
			pet->SetOwnerID(0);
		}
	}
	petids.clear();
	petid = 0;
	focused_pet_id = 0;
}

bool Mob::HasPet(uint8 idx) const
{
	if (petids.empty() || idx >= petids.size()) {
		if (idx == 0 && petid) {
			auto m = entity_list.GetMob(petid);
			return m && m->GetOwnerID() == GetID();
		}
		return false;
	}

	auto m = entity_list.GetMob(petids[idx]);
	return m && m->GetOwnerID() == GetID();
}

bool Mob::AddPet(Mob* newpet)
{
	return newpet && AddPet(newpet->GetID());
}

bool Mob::AddPet(uint16 new_pet_id)
{
	ValidatePetList();
	auto newpet = entity_list.GetMob(new_pet_id);
	if (!newpet) {
		return false;
	}

	if (petids.size() >= RuleI(Custom, AbsolutePetLimit)) {
		return false;
	}

	if (std::find(petids.begin(), petids.end(), new_pet_id) != petids.end()) {
		return true;
	}

	petids.push_back(new_pet_id);
	newpet->SetOwnerID(GetID());
	petid = petids.front();
	focused_pet_id = new_pet_id;
	ConfigurePetWindow(newpet);

	if (
		newpet->IsNPC() &&
		IsClient() &&
		!IsCharmSpell(newpet->CastToNPC()->GetPetSpellID())
	) {
		CastToClient()->DoPetBagResync(newpet->CastToNPC()->GetPetOriginClass());
	}

	return true;
}

bool Mob::SetPet(Mob* newpet, uint8 idx)
{
	return SetPet(newpet ? newpet->GetID() : 0, idx);
}

bool Mob::SetPet(uint16 new_pet_id, uint8 idx)
{
	ValidatePetList();

	if (idx >= RuleI(Custom, AbsolutePetLimit)) {
		return false;
	}

	if (new_pet_id == 0) {
		if (idx < petids.size()) {
			return RemovePetByIndex(idx);
		}
		if (idx == 0) {
			petid = 0;
			return true;
		}
		return false;
	}

	auto newpet = entity_list.GetMob(new_pet_id);
	if (!newpet) {
		return false;
	}

	if (idx >= petids.size()) {
		petids.resize(idx + 1, 0);
	}

	if (petids[idx] == new_pet_id) {
		return true;
	}

	Mob* oldowner = entity_list.GetMob(newpet->GetOwnerID());
	if (oldowner && oldowner != this) {
		oldowner->ValidatePetList();
		for (auto it = oldowner->petids.begin(); it != oldowner->petids.end(); ++it) {
			if (*it == new_pet_id) {
				oldowner->petids.erase(it);
				break;
			}
		}
		oldowner->petid = oldowner->petids.empty() ? 0 : oldowner->petids.front();
	}

	if (petids[idx]) {
		auto existing = entity_list.GetMob(petids[idx]);
		if (existing && existing->GetOwnerID() == GetID()) {
			existing->SetOwnerID(0);
		}
	}

	petids[idx] = new_pet_id;
	newpet->SetOwnerID(GetID());
	petid = petids.empty() ? 0 : petids.front();
	focused_pet_id = new_pet_id;
	ConfigurePetWindow(newpet);

	if (
		newpet->IsNPC() &&
		IsClient() &&
		!IsCharmSpell(newpet->CastToNPC()->GetPetSpellID())
	) {
		CastToClient()->DoPetBagResync(newpet->CastToNPC()->GetPetOriginClass());
	}

	return true;
}

void Mob::ConfigurePetWindow(Mob* selected_pet)
{
	if (!IsClient()) {
		return;
	}

	if (!selected_pet || selected_pet->GetOwnerID() != GetID() || !selected_pet->IsNPC()) {
		return;
	}

	auto this_client = CastToClient();
	auto pet_npc = selected_pet->CastToNPC();
	auto outapp = new EQApplicationPacket;
	auto outapp2 = new EQApplicationPacket;

	focused_pet_id = pet_npc->GetID();

	pet_npc->CreateDespawnPacket(outapp, false);
	pet_npc->CreateSpawnPacket(outapp2, this);

	this_client->QueuePacket(outapp);
	this_client->QueuePacket(outapp2);

	pet_npc->SendAppearancePacket(AppearanceType::Pet, GetID(), true, true);

	for (auto pet_iter : GetAllPets()) {
		if (pet_iter && pet_iter->GetID() != pet_npc->GetID()) {
			pet_iter->SendAppearancePacket(AppearanceType::Pet, GetID(), true, true);
		}
	}

	if (GetTarget() && GetTarget()->GetID() == pet_npc->GetID()) {
		pet_npc->SendBuffsToClient(this_client);
	}

	pet_npc->SendPetBuffsToClient();

	this_client->SetPetCommandState(PetButton::Sit, pet_npc->GetPetOrder() == SPO_Sit);
	this_client->SetPetCommandState(PetButton::Stop, pet_npc->IsPetStop());
	this_client->SetPetCommandState(PetButton::Regroup, pet_npc->IsPetRegroup());
	this_client->SetPetCommandState(PetButton::Follow, pet_npc->GetPetOrder() == SPO_Follow);
	this_client->SetPetCommandState(PetButton::Guard, pet_npc->GetPetOrder() == SPO_Guard);
	this_client->SetPetCommandState(PetButton::Taunt, pet_npc->IsTaunting());
	this_client->SetPetCommandState(PetButton::Hold, pet_npc->IsHeld());
	this_client->SetPetCommandState(PetButton::GreaterHold, pet_npc->IsGHeld());
	this_client->SetPetCommandState(PetButton::Focus, pet_npc->IsFocused());
	this_client->SetPetCommandState(PetButton::SpellHold, pet_npc->IsNoCast());

	safe_delete(outapp);
	safe_delete(outapp2);

	if (GetTarget()) {
		auto app = new EQApplicationPacket(OP_PetHoTT, sizeof(ClientTarget_Struct));
		auto ct = (ClientTarget_Struct*)app->pBuffer;
		ct->new_target = pet_npc->GetTarget() ? pet_npc->GetTarget()->GetID() : 0;
		this_client->FastQueuePacket(&app);
	}
}

bool Mob::IsMyPet(Mob* mob) const
{
	if (!mob) {
		return false;
	}

	if (mob->GetOwnerID() != GetID()) {
		return false;
	}

	for (auto id : petids) {
		if (id == mob->GetID()) {
			return true;
		}
	}

	return false;
}

void Mob::SetPetID(uint16 NewPetID)
{
	if (NewPetID == GetID() && NewPetID != 0) {
		return;
	}

	if (NewPetID == 0) {
		RemovePetByIndex(0);
	} else {
		SetPet(NewPetID, 0);
	}

	petid = petids.empty() ? 0 : petids.front();

	if (IsClient()) {
		Mob* NewPet = entity_list.GetMob(GetPetID());
		CastToClient()->UpdateXTargetType(MyPet, NewPet);
	}
}

void NPC::GetPetState(SpellBuff_Struct *pet_buffs, uint32 *items, char *name) {
	//save the pet name
	strn0cpy(name, GetName(), 64);

	//save their items, we only care about what they are actually wearing
	memcpy(items, equipment, sizeof(uint32) * EQ::invslot::EQUIPMENT_COUNT);

	//save their buffs.
	for (int i=EQ::invslot::EQUIPMENT_BEGIN; i < GetPetMaxTotalSlots(); i++) {
		if (IsValidSpell(buffs[i].spellid)) {
			pet_buffs[i].spellid = buffs[i].spellid;
			pet_buffs[i].effect_type = i+1;
			pet_buffs[i].duration = buffs[i].ticsremaining;
			pet_buffs[i].level = buffs[i].casterlevel;
			pet_buffs[i].bard_modifier = 10;
			pet_buffs[i].counters = buffs[i].counters;
			pet_buffs[i].bard_modifier = buffs[i].instrument_mod;
		}
		else {
			pet_buffs[i].spellid = SPELL_UNKNOWN;
			pet_buffs[i].duration = 0;
			pet_buffs[i].level = 0;
			pet_buffs[i].bard_modifier = 10;
			pet_buffs[i].counters = 0;
		}
	}
}

void NPC::SetPetState(SpellBuff_Struct *pet_buffs, uint32 *items) {
	//restore their buffs...

	int i;
	for (i = 0; i < GetPetMaxTotalSlots(); i++) {
		for(int z = 0; z < GetPetMaxTotalSlots(); z++) {
		// check for duplicates
			if(IsValidSpell(buffs[z].spellid) && buffs[z].spellid == pet_buffs[i].spellid) {
				buffs[z].spellid = SPELL_UNKNOWN;
				pet_buffs[i].spellid = 0xFFFFFFFF;
			}
		}

		if (pet_buffs[i].spellid <= (uint32)SPDAT_RECORDS && pet_buffs[i].spellid != 0 && (pet_buffs[i].duration > 0 || pet_buffs[i].duration == -1)) {
			if(pet_buffs[i].level == 0 || pet_buffs[i].level > 100)
				pet_buffs[i].level = 1;
			buffs[i].spellid			= pet_buffs[i].spellid;
			buffs[i].ticsremaining		= pet_buffs[i].duration;
			buffs[i].casterlevel		= pet_buffs[i].level;
			buffs[i].casterid			= 0;
			buffs[i].counters			= pet_buffs[i].counters;
			buffs[i].hit_number			= spells[pet_buffs[i].spellid].hit_number;
			buffs[i].instrument_mod		= pet_buffs[i].bard_modifier;
		}
		else {
			buffs[i].spellid = SPELL_UNKNOWN;
			pet_buffs[i].spellid = 0xFFFFFFFF;
			pet_buffs[i].effect_type = 0;
			pet_buffs[i].level = 0;
			pet_buffs[i].duration = 0;
			pet_buffs[i].bard_modifier = 0;
		}
	}
	for (int j1=0; j1 < GetPetMaxTotalSlots(); j1++) {
		if (buffs[j1].spellid <= (uint32)SPDAT_RECORDS) {
			for (int x1=0; x1 < EFFECT_COUNT; x1++) {
				switch (spells[buffs[j1].spellid].effect_id[x1]) {
					case SpellEffect::AddMeleeProc:
					case SpellEffect::WeaponProc:
						// We need to reapply buff based procs
						// We need to do this here so suspended pets also regain their procs.
						AddProcToWeapon(GetProcID(buffs[j1].spellid,x1), false, 100+spells[buffs[j1].spellid].limit_value[x1], buffs[j1].spellid, buffs[j1].casterlevel, GetSpellProcLimitTimer(buffs[j1].spellid, ProcType::MELEE_PROC));
						break;
					case SpellEffect::DefensiveProc:
						AddDefensiveProc(GetProcID(buffs[j1].spellid, x1), 100 + spells[buffs[j1].spellid].limit_value[x1], buffs[j1].spellid, GetSpellProcLimitTimer(buffs[j1].spellid, ProcType::DEFENSIVE_PROC));
						break;
					case SpellEffect::RangedProc:
						AddRangedProc(GetProcID(buffs[j1].spellid, x1), 100 + spells[buffs[j1].spellid].limit_value[x1], buffs[j1].spellid, GetSpellProcLimitTimer(buffs[j1].spellid, ProcType::RANGED_PROC));
						break;
					case SpellEffect::Charm:
					case SpellEffect::Rune:
					case SpellEffect::NegateAttacks:
					case SpellEffect::Illusion:
						buffs[j1].spellid = SPELL_UNKNOWN;
						pet_buffs[j1].spellid = SPELLBOOK_UNKNOWN;
						pet_buffs[j1].effect_type = 0;
						pet_buffs[j1].level = 0;
						pet_buffs[j1].duration = 0;
						pet_buffs[j1].bard_modifier = 0;
						x1 = EFFECT_COUNT;
						break;
					// We can't send appearance packets yet, put down at CompleteConnect
				}
			}
		}
	}

	//restore their equipment...
	for (i = EQ::invslot::EQUIPMENT_BEGIN; i <= EQ::invslot::EQUIPMENT_END; i++) {
		if (items[i] == 0) {
			continue;
		}

		const EQ::ItemData *item2 = database.GetItem(items[i]);

		if (item2) {
			bool noDrop           = (item2->NoDrop == 0); // Field is reverse logic
			bool petCanHaveNoDrop = (RuleB(Pets, CanTakeNoDrop) && _CLIENTPET(this) && GetPetType() <= PetType::Normal);

			if (!noDrop || petCanHaveNoDrop) {
				AddLootDrop(item2, LootdropEntriesRepository::NewNpcEntity(), true);
			}
		}
	}
}

// Load the equipmentset from the DB. Might be worthwhile to load these into
// shared memory at some point due to the number of queries needed to load a
// nested set.
bool ZoneDatabase::GetBasePetItems(int32 equipmentset, uint32 *items) {
	if (equipmentset < 0 || items == nullptr)
		return false;

	// Equipment sets can be nested. We start with the top-most one and
	// add all items in it to the items array. Referenced equipmentsets
	// are loaded after that, up to a max depth of 5. (Arbitrary limit
	// so we don't go into an endless loop if the DB data is cyclic for
	// some reason.)
	// A slot will only get an item put in it if it is empty. That way
	// an equipmentset can overload a slot for the set(s) it includes.

	int depth = 0;
	int32 curset = equipmentset;
	int32 nextset = -1;
	uint32 slot;

	// outline:
	// get equipmentset from DB. (Mainly check if we exist and get the
	// nested ID)
	// query pets_equipmentset_entries with the set_id and loop over
	// all of the result rows. Check if we have something in the slot
	// already. If no, add the item id to the equipment array.
	while (curset >= 0 && depth < 5) {
		std::string  query = StringFormat("SELECT nested_set FROM pets_equipmentset WHERE set_id = '%d'", curset);
		auto results = QueryDatabase(query);
		if (!results.Success()) {
			return false;
		}

		if (results.RowCount() != 1) {
			// invalid set reference, it doesn't exist
			LogError("Error in GetBasePetItems equipment set [{}] does not exist", curset);
			return false;
		}

		auto row = results.begin();
		nextset = Strings::ToInt(row[0]);

		query = StringFormat("SELECT slot, item_id FROM pets_equipmentset_entries WHERE set_id='%d'", curset);
		results = QueryDatabase(query);
		if (results.Success()) {
			for (row = results.begin(); row != results.end(); ++row)
			{
				slot = Strings::ToInt(row[0]);

				if (slot > EQ::invslot::EQUIPMENT_END)
					continue;

				if (items[slot] == 0)
					items[slot] = Strings::ToInt(row[1]);
			}
		}

		curset = nextset;
		depth++;
	}

	return true;
}

bool Pet::CheckSpellLevelRestriction(Mob *caster, uint16 spell_id)
{
	auto owner = GetOwner();
	if (owner)
		return owner->CheckSpellLevelRestriction(caster, spell_id);
	return true;
}

BeastlordPetData::PetStruct ZoneDatabase::GetBeastlordPetData(uint16 race_id) {
	BeastlordPetData::PetStruct d;

	const auto& e = PetsBeastlordDataRepository::FindOne(*this, race_id);

	if (!e.player_race) {
		return d;
	}

	d.race_id       = e.pet_race;
	d.texture       = e.texture;
	d.helm_texture  = e.helm_texture;
	d.gender        = e.gender;
	d.size_modifier = e.size_modifier;
	d.face          = e.face;

	return d;
}
