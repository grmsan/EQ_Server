/*	EQEMu: Everquest Server Emulator
	Copyright (C) 2001-2016 EQEMu Development Team (http://eqemulator.net)

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

// Includes and globals
#include "item_instance.h"
#include "data_verification.h"
#include "say_link.h"
#include "inventory_profile.h"
#include "classes.h"
#include "item_data.h"
//#include "races.h"
#include "rulesys.h"
#include "shareddb.h"
#include "strings.h"
#include "evolving_items.h"
#include "item_scaling_config.h"

#include <limits.h>
#include <fstream>
#include <chrono>
#include <ctime>
#include <cmath>

// global serials and GUIDs are defined in patches (e.g., rof/rof2/sod) - do not redefine next_item_serial_number here
// but we do need a local guids set used by functions in this file
static std::unordered_set<uint64> guids{};

// forward declaration for GetNextItemInstSerialNumber - defined in patches

// Fallback implementation for GetNextItemInstSerialNumber if not provided by patches.
// This keeps behavior local to this TU and avoids link errors during builds where patch
// functions are not available.
static inline int32 GetNextItemInstSerialNumber_Fallback() {
	static int32 next_item_serial_number_local = 1;
	if (next_item_serial_number_local >= INT32_MAX) {
		next_item_serial_number_local = 1;
	}
	return next_item_serial_number_local++;
}
// If no external symbol provides the function, weak alias to fallback implementation.
// Some toolchains may not support weak alias; compile-time override is used below.

//
// class EQ::ItemInstance
//
EQ::ItemInstance::ItemInstance(const ItemData* item, int16 charges) {

	if (item) {
		m_item = new ItemData(*item);
	}

	m_charges = charges;

	if (m_item && m_item->IsClassCommon()) {
		m_color = m_item->Color;
	}

	if (IsEvolving()) {
		SetTimer("evolve", RuleI(EvolvingItems, DelayUponEquipping));
	}

	m_SerialNumber  = GetNextItemInstSerialNumber_Fallback();
}

EQ::ItemInstance::ItemInstance(SharedDatabase *db, uint32 item_id, int16 charges) {

	m_item     = db->GetItem(item_id);

	// If item not found and it's a dynamic item (ID >= 1 billion), try loading from database
	if (!m_item && item_id >= 1000000000U) {
		// Load from database and add to cache for future use
		db->LoadDynamicItemToCache(item_id);
		m_item = db->GetItem(item_id);
	}

	if (m_item) {
		m_item = new ItemData(*m_item);
	}

	m_charges = charges;

	if (m_item && m_item->IsClassCommon()) {
		m_color = m_item->Color;
	} else {
		m_color = 0;
	}

	if (IsEvolving()) {
		SetTimer("evolve", RuleI(EvolvingItems, DelayUponEquipping));
	}

	m_SerialNumber  = GetNextItemInstSerialNumber_Fallback();
}
EQ::ItemInstance::ItemInstance(ItemInstTypes use_type) {
	m_use_type     = use_type;
}

// Make a copy of an EQ::ItemInstance object
EQ::ItemInstance::ItemInstance(const ItemInstance& copy)
{
	m_use_type = copy.m_use_type;

	if (copy.m_item) {
		m_item = new ItemData(*copy.m_item);
	} else {
		m_item = nullptr;
	}

	m_charges       = copy.m_charges;
	m_price         = copy.m_price;
	m_color         = copy.m_color;
	m_merchantslot  = copy.m_merchantslot;
	m_currentslot   = copy.m_currentslot;
	m_attuned       = copy.m_attuned;
	m_merchantcount = copy.m_merchantcount;

	// Copy container contents
	for (auto it = copy.m_contents.begin(); it != copy.m_contents.end(); ++it) {
		ItemInstance* inst_old = it->second;
		ItemInstance* inst_new = nullptr;

		if (inst_old) {
			inst_new = inst_old->Clone();
		}

		if (inst_new) {
			m_contents[it->first] = inst_new;
		}
	}

	std::map<std::string, std::string>::const_iterator iter;
	for (iter = copy.m_custom_data.begin(); iter != copy.m_custom_data.end(); ++iter) {
		m_custom_data[iter->first] = iter->second;
	}

	m_SerialNumber = copy.m_SerialNumber;
	m_custom_data  = copy.m_custom_data;
	m_timers       = copy.m_timers;

	m_exp       = copy.m_exp;
	m_evolveLvl = copy.m_evolveLvl;

	if (copy.m_scaledItem) {
		m_scaledItem = new ItemData(*copy.m_scaledItem);
	} else {
		m_scaledItem = nullptr;
	}

	m_evolving_details    = copy.m_evolving_details;
	m_scaling             = copy.m_scaling;
	m_ornamenticon        = copy.m_ornamenticon;
	m_ornamentidfile      = copy.m_ornamentidfile;
	m_ornament_hero_model = copy.m_ornament_hero_model;
	m_recast_timestamp    = copy.m_recast_timestamp;
	m_new_id_file         = copy.m_new_id_file;

	// Reapply custom stats after copying (custom_data is already populated)
	ApplyCustomStats();
}

// Clean up container contents
EQ::ItemInstance::~ItemInstance()
{
	Clear();
	safe_delete(m_item);
	safe_delete(m_scaledItem);
}

// Query item type
bool EQ::ItemInstance::IsType(item::ItemClass item_class) const
{
	// IsType(<ItemClassTypes>) does not protect against 'm_item = nullptr'

	// Check usage type
	if (m_use_type == ItemInstWorldContainer && item_class == item::ItemClassBag) {
		return true;
	}

	if (!m_item) {
		return false;
	}

	return (m_item->ItemClass == item_class);
}

bool EQ::ItemInstance::IsClassCommon() const
{
	return (m_item && m_item->IsClassCommon());
}

bool EQ::ItemInstance::IsClassBag() const
{
	return (m_item && m_item->IsClassBag());
}

bool EQ::ItemInstance::IsClassBook() const
{
	return (m_item && m_item->IsClassBook());
}

// Is item stackable?
bool EQ::ItemInstance::IsStackable() const
{
	return (m_item && m_item->Stackable);
}

bool EQ::ItemInstance::IsCharged() const
{
	if (!m_item) {
		return false;
	}

	if (m_item->MaxCharges > 1) {
		return true;
	} else {
		return false;
	}
}

// Can item be equipped?
bool EQ::ItemInstance::IsEquipable(uint16 race, uint16 class_bits) const
{
	if (!m_item || !m_item->Slots) {
		return false;
	}

	return m_item->IsEquipable(race, class_bits);
}

// Can item be equipped by Class?
bool EQ::ItemInstance::IsClassEquipable(uint16 class_) const
{
	if (!m_item || !m_item->Slots) {
		return false;
	}

	return m_item->IsClassEquipable(class_);
}

// Can item be equipped by Race?
bool EQ::ItemInstance::IsRaceEquipable(uint16 race) const
{
	if (!m_item || !m_item->Slots) {
		return false;
	}

	return m_item->IsRaceEquipable(race);
}

// Can equip at this slot?
bool EQ::ItemInstance::IsEquipable(int16 slot_id) const
{
	if (!m_item || !m_item->Slots) {
		return false;
	}

	if (slot_id < EQ::invslot::EQUIPMENT_BEGIN || slot_id > EQ::invslot::EQUIPMENT_END) {
		return false;
	}

	return ((m_item->Slots & (1 << slot_id)) != 0);
}

bool EQ::ItemInstance::IsAugmentable() const
{
	if (!m_item) {
		return false;
	}

	for (int index = invaug::SOCKET_BEGIN; index <= invaug::SOCKET_END; ++index) {
		if (m_item->AugSlotType[index] != 0) {
			return true;
		}
	}

	return false;
}

bool EQ::ItemInstance::AvailableWearSlot(uint32 aug_wear_slots) const {
	if (!m_item || !m_item->IsClassCommon()) {
		return false;
	}

	int index = invslot::EQUIPMENT_BEGIN;
	for (; index <= invslot::EQUIPMENT_END; ++index) {
		if (m_item->Slots & (1 << index)) {
			if (aug_wear_slots & (1 << index)) {
				break;
			}
		}
	}

	return (index <= EQ::invslot::EQUIPMENT_END);
}

int8 EQ::ItemInstance::AvailableAugmentSlot(int32 augment_type) const
{
	if (!m_item || !m_item->IsClassCommon()) {
		return INVALID_INDEX;
	}

	for (int16 slot_id = invaug::SOCKET_BEGIN; slot_id <= invaug::SOCKET_END; ++slot_id) {
		if (IsAugmentSlotAvailable(augment_type, slot_id)) {
			return slot_id;
		}
	}

	return INVALID_INDEX;
}

bool EQ::ItemInstance::IsAugmentSlotAvailable(int32 augment_type, uint8 slot) const
{
	if (!m_item || !m_item->IsClassCommon() || GetItem(slot)) {
		return false;
	}

	return (
		(
			augment_type == -1 ||
			(
				m_item->AugSlotType[slot] &&
				((1 << (m_item->AugSlotType[slot] - 1)) & augment_type)
			)
		) &&
		(
			RuleB(Items, AugmentItemAllowInvisibleAugments) ||
			m_item->AugSlotVisible[slot]
		)
	);
}

// Retrieve item inside container
EQ::ItemInstance* EQ::ItemInstance::GetItem(uint8 index) const
{
	auto it = m_contents.find(index);
	if (it != m_contents.end()) {
		return it->second;
	}

	return nullptr;
}

uint32 EQ::ItemInstance::GetItemID(uint8 slot) const
{
	const auto item = GetItem(slot);
	if (item) {
		return item->GetID();
	}

	return 0;
}

void EQ::ItemInstance::PutItem(uint8 index, const ItemInstance& inst)
{
	// Clean up item already in slot (if exists)
	DeleteItem(index);

	// Delegate to internal method
	_PutItem(index, inst.Clone());
}

// Remove item inside container
void EQ::ItemInstance::DeleteItem(uint8 index)
{
	ItemInstance* inst = PopItem(index);
	safe_delete(inst);
}

// Remove item from container without memory delete
// Hands over memory ownership to client of this function call
EQ::ItemInstance* EQ::ItemInstance::PopItem(uint8 index)
{
	auto iter = m_contents.find(index);
	if (iter != m_contents.end()) {
		ItemInstance* inst = iter->second;
		m_contents.erase(index);
		return inst; // Return pointer that needs to be deleted (or otherwise managed)
	}

	return nullptr;
}

// Remove all items from container
void EQ::ItemInstance::Clear()
{
	// Destroy container contents
	for (auto iter = m_contents.begin(); iter != m_contents.end(); ++iter) {
		safe_delete(iter->second);
	}
	m_contents.clear();
}

// Remove all items from container
void EQ::ItemInstance::ClearByFlags(byFlagSetting is_nodrop, byFlagSetting is_norent)
{
	// TODO: This needs work...

	// Destroy container contents
	std::map<uint8, ItemInstance*>::const_iterator cur, end, del;
	cur = m_contents.begin();
	end = m_contents.end();
	for (; cur != end;) {
		ItemInstance* inst = cur->second;
		if (inst == nullptr) {
			cur = m_contents.erase(cur);
			continue;
		}

		const ItemData* item = inst->GetItem();
		if (item == nullptr) {
			cur = m_contents.erase(cur);
			continue;
		}

		del = cur;
		++cur;

		switch (is_nodrop) {
		case byFlagSet:
			if (item->NoDrop == 0) {
				safe_delete(inst);
				m_contents.erase(del->first);
				continue;
			}
			// no 'break;' deletes 'byFlagNotSet' type - can't add at the moment because it really *breaks* the process somewhere
		case byFlagNotSet:
			if (item->NoDrop != 0) {
				safe_delete(inst);
				m_contents.erase(del->first);
				continue;
			}
		default:
			break;
		}

		switch (is_norent) {
		case byFlagSet:
			if (item->NoRent == 0) {
				safe_delete(inst);
				m_contents.erase(del->first);
				continue;
			}
			// no 'break;' deletes 'byFlagNotSet' type - can't add at the moment because it really *breaks* the process somewhere
		case byFlagNotSet:
			if (item->NoRent != 0) {
				safe_delete(inst);
				m_contents.erase(del->first);
				continue;
			}
		default:
			break;
		}
	}
}

uint8 EQ::ItemInstance::FirstOpenSlot() const
{
	if (!m_item)
		return INVALID_INDEX;

	uint8 slots = m_item->BagSlots, i;
	for (i = invbag::SLOT_BEGIN; i < slots; i++) {
		if (!GetItem(i))
			break;
	}

	return (i < slots) ? i : INVALID_INDEX;
}

uint8 EQ::ItemInstance::GetTotalItemCount() const
{
	if (!m_item) {
		return 0;
	}

	uint8 item_count = 1;

	if (!m_item->IsClassBag()) {
		return item_count;
	}

	for (int index = invbag::SLOT_BEGIN; index < m_item->BagSlots; ++index) {
		if (GetItem(index)) {
			++item_count;
		}
	}

	return item_count;
}

bool EQ::ItemInstance::IsNoneEmptyContainer()
{
	if (!m_item || !m_item->IsClassBag())
		return false;

	for (int index = invbag::SLOT_BEGIN; index < m_item->BagSlots; ++index) {
		if (GetItem(index))
			return true;
	}

	return false;
}

// Retrieve augment inside item
EQ::ItemInstance* EQ::ItemInstance::GetAugment(uint8 augment_index) const
{
	if (m_item && m_item->IsClassCommon()) {
		return GetItem(augment_index);
	}

	return nullptr;
}

bool EQ::ItemInstance::IsOrnamentationAugment(EQ::ItemInstance* augment) const
{
	if (!m_item || !m_item->IsClassCommon() || !augment) {
		return false;
	}

	const auto augment_item = augment->GetItem();
	if (!augment_item) {
		return false;
	}

	const std::string& idfile = augment_item->IDFile;

	if (
		EQ::ValueWithin(
			augment->GetAugmentType(),
			OrnamentationAugmentTypes::StandardOrnamentation,
			OrnamentationAugmentTypes::SpecialOrnamentation
		) ||
		(
			idfile != "IT63" &&
			idfile != "IT64"
		) ||
		augment_item->HerosForgeModel
	) {
		return true;
	}

	return false;
}

EQ::ItemInstance* EQ::ItemInstance::GetOrnamentationAugment() const
{
	if (!m_item || !m_item->IsClassCommon()) {
		return nullptr;
	}

	for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; i++) {
		const auto augment = GetAugment(i);
		if (augment && IsOrnamentationAugment(augment)) {
			return augment;
		}
	}

	return nullptr;
}

uint32 EQ::ItemInstance::GetOrnamentHeroModel(int32 material_slot) const
{
	// Not a Hero Forge item.
	if (m_ornament_hero_model == 0) {
		return 0;
	}

	// Item is using an explicit Hero Forge ID
	if (m_ornament_hero_model >= 1000) {
		return m_ornament_hero_model;
	}

	// Item is using a shorthand ID
	return (m_ornament_hero_model * 100) + material_slot;
}

bool EQ::ItemInstance::UpdateOrnamentationInfo()
{
	if (!m_item || !m_item->IsClassCommon()) {
		return false;
	}

	const auto augment = GetOrnamentationAugment();

	if (augment) {
		const auto augment_item = GetOrnamentationAugment()->GetItem();

		if (augment_item) {
			SetOrnamentIcon(augment_item->Icon);
			SetOrnamentHeroModel(augment_item->HerosForgeModel);

			if (strlen(augment_item->IDFile) > 2) {
				SetOrnamentationIDFile(Strings::ToUnsignedInt(&augment_item->IDFile[2]));
			} else {
				SetOrnamentationIDFile(0);
			}

			return true;
		}
	}

	SetOrnamentIcon(0);
	SetOrnamentHeroModel(0);
	SetOrnamentationIDFile(0);

	return false;
}

bool EQ::ItemInstance::CanTransform(const ItemData *ItemToTry, const ItemData *Container, bool AllowAll) {
	if (!ItemToTry || !Container) return false;

	if (ItemToTry->ItemType == item::ItemTypeArrow || strnlen(Container->CharmFile, 30) == 0)
		return false;

	if (AllowAll && strncasecmp(Container->CharmFile, "ITEMTRANSFIGSHIELD", 18) && strncasecmp(Container->CharmFile, "ITEMTransfigBow", 15)) {
		switch (ItemToTry->ItemType) {
			case 0:
			case 1:
			case 2:
			case 3:
			case 4:
			case 35:
			case 45:
				return true;
		}
	}

	static std::map<std::string, int> types;
	types["itemtransfig1hp"] = 2;
	types["itemtransfig1hs"] = 0;
	types["itemtransfig2hb"] = 4;
	types["itemtransfig2hp"] = 35;
	types["itemtransfig2hs"] = 1;
	types["itemtransfigblunt"] = 3;
	types["itemtransfig1hb"] = 3;
	types["itemtransfigbow"] = 5;
	types["itemtransfighth"] = 45;
	types["itemtransfigshield"] = 8;
	types["itemtransfigslashing"] = 0;

	auto i = types.find(MakeLowerString(Container->CharmFile));
	if (i != types.end() && i->second == ItemToTry->ItemType)
		return true;

	static std::map<std::string, int> typestwo;
	typestwo["itemtransfigblunt"] = 4;
	typestwo["itemtransfigslashing"] = 1;

	i = typestwo.find(MakeLowerString(Container->CharmFile));
	if (i != typestwo.end() && i->second == ItemToTry->ItemType)
		return true;

	return false;
}

uint32 EQ::ItemInstance::GetAugmentItemID(uint8 augment_index) const
{
	if (!m_item || !m_item->IsClassCommon()) {
		return 0;
	}

	return GetItemID(augment_index);
}

// Add an augment to the item
void EQ::ItemInstance::PutAugment(uint8 slot, const ItemInstance& augment)
{
	if (!m_item || !m_item->IsClassCommon())
		return;

	PutItem(slot, augment);
}

void EQ::ItemInstance::PutAugment(SharedDatabase *db, uint8 slot, uint32 item_id)
{
	if (item_id == 0) { return; }
	if (db == nullptr) { return; /* TODO: add log message for nullptr */ }

	const ItemInstance* aug = db->CreateItem(item_id);
	if (aug) {
		PutAugment(slot, *aug);
		safe_delete(aug);
	}
}

// Remove augment from item and destroy it
void EQ::ItemInstance::DeleteAugment(uint8 index)
{
	if (!m_item || !m_item->IsClassCommon())
		return;

	DeleteItem(index);
}

// Remove augment from item and return it
EQ::ItemInstance* EQ::ItemInstance::RemoveAugment(uint8 index)
{
	if (!m_item || !m_item->IsClassCommon())
		return nullptr;

	return PopItem(index);
}

bool EQ::ItemInstance::IsAugmented()
{
	if (!m_item || !m_item->IsClassCommon()) {
		return false;
	}

	for (uint8 slot_id = invaug::SOCKET_BEGIN; slot_id <= invaug::SOCKET_END; ++slot_id) {
		if (GetAugmentItemID(slot_id)) {
			return true;
		}
	}

	return false;
}

bool EQ::ItemInstance::ContainsAugmentByID(uint32 item_id)
{
	if (!m_item || !m_item->IsClassCommon()) {
		return false;
	}

	if (!item_id) {
		return false;
	}

	for (uint8 augment_slot = invaug::SOCKET_BEGIN; augment_slot <= invaug::SOCKET_END; ++augment_slot) {
		if (GetAugmentItemID(augment_slot) == item_id) {
			return true;
		}
	}

	return false;
}

int EQ::ItemInstance::CountAugmentByID(uint32 item_id)
{
	int quantity = 0;
	if (!m_item || !m_item->IsClassCommon()) {
		return quantity;
	}

	if (!item_id) {
		return quantity;
	}

	for (uint8 augment_slot = invaug::SOCKET_BEGIN; augment_slot <= invaug::SOCKET_END; ++augment_slot) {
		if (GetAugmentItemID(augment_slot) == item_id) {
			quantity++;
		}
	}

	return quantity;
}

// Has attack/delay?
bool EQ::ItemInstance::IsWeapon() const
{
	if (!m_item || !m_item->IsClassCommon())
		return false;

	if (m_item->ItemType == item::ItemTypeArrow && m_item->Damage != 0)
		return true;
	else
		return ((m_item->Damage != 0) && (m_item->Delay != 0));
}

bool EQ::ItemInstance::IsAmmo() const
{
	if (!m_item)
		return false;

	if ((m_item->ItemType == item::ItemTypeArrow) ||
		(m_item->ItemType == item::ItemTypeLargeThrowing) ||
		(m_item->ItemType == item::ItemTypeSmallThrowing)
		) {
		return true;
	}

	return false;

}

const EQ::ItemData* EQ::ItemInstance::GetItem() const
{
	if (!m_item)
		return nullptr;

	if (m_scaledItem)
		return m_scaledItem;

	return m_item;
}

const EQ::ItemData* EQ::ItemInstance::GetUnscaledItem() const
{
	// No operator calls and defaults to nullptr
	return m_item;
}

std::string EQ::ItemInstance::GetCustomDataString() const {
	std::string ret_val;
	auto iter = m_custom_data.begin();
	while (iter != m_custom_data.end()) {
		if (ret_val.length() > 0) {
			ret_val += "^";
		}
		ret_val += iter->first;
		ret_val += "^";
		ret_val += iter->second;
		++iter;

		if (ret_val.length() > 0) {
			ret_val += "^";
		}
	}
	return ret_val;
}

void EQ::ItemInstance::SetCustomDataString(const std::string& str)
{
	if (str.empty()) return;

	// TEMPORARILY DISABLED - JSON parsing causing crashes
	// Just log and skip for now
	if (str[0] == '{') {
		std::ofstream logfile("logs/inf/json_skipped.log", std::ios::app);
		if (logfile.is_open()) {
			auto now = std::chrono::system_clock::now();
			auto time = std::chrono::system_clock::to_time_t(now);
			char timebuf[32];
			std::strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", std::localtime(&time));
			logfile << "[" << timebuf << "] Skipped JSON: '" << str << "'" << std::endl;
			logfile.close();
		}
		return;
	}

	// Original format: "key^value^key^value"
	auto components = Strings::Split(str, "^");
	auto value_count = components.size() / 2;

	for (auto i = 0; i < value_count; i++) {
		auto identifier = components[i * 2];
		auto value = components[(i * 2) + 1];

		SetCustomData(identifier, value);
	}
}std::string EQ::ItemInstance::GetCustomData(const std::string& identifier) {
	std::map<std::string, std::string>::const_iterator iter = m_custom_data.find(identifier);
	if (iter != m_custom_data.end()) {
		return iter->second;
	}

	return "";
}

void EQ::ItemInstance::SetCustomData(const std::string& identifier, const std::string& value) {
	DeleteCustomData(identifier);
	m_custom_data[identifier] = value;
	ApplyCustomStats();
}

void EQ::ItemInstance::SetCustomData(const std::string& identifier, int value) {
	DeleteCustomData(identifier);
	std::stringstream ss;
	ss << value;
	m_custom_data[identifier] = ss.str();
	ApplyCustomStats();
}

void EQ::ItemInstance::SetCustomData(const std::string& identifier, float value) {
	DeleteCustomData(identifier);
	std::stringstream ss;
	ss << value;
	m_custom_data[identifier] = ss.str();
	ApplyCustomStats();
}

void EQ::ItemInstance::SetCustomData(const std::string& identifier, bool value) {
	DeleteCustomData(identifier);
	std::stringstream ss;
	ss << value;
	m_custom_data[identifier] = ss.str();
	ApplyCustomStats();
}

void EQ::ItemInstance::DeleteCustomData(const std::string& identifier) {
	auto iter = m_custom_data.find(identifier);
	if (iter != m_custom_data.end()) {
		m_custom_data.erase(iter);
	}
}

// Clone a type of EQ::ItemInstance object
// c++ doesn't allow a polymorphic copy constructor,
// so we have to resort to a polymorphic Clone()
EQ::ItemInstance* EQ::ItemInstance::Clone() const
{
	// Pseudo-polymorphic copy constructor
	return new ItemInstance(*this);
}

bool EQ::ItemInstance::IsSlotAllowed(int16 slot_id) const {
	if (!m_item) { return false; }
	else if (InventoryProfile::SupportsContainers(slot_id)) { return true; }
	else if (m_item->Slots & (1 << slot_id)) { return true; }
	else if (slot_id > invslot::EQUIPMENT_END) { return true; } // why do we call 'InventoryProfile::SupportsContainers' with this here?
	else { return false; }
}

bool EQ::ItemInstance::IsDroppable(bool recurse) const
{
	if (!m_item) {
		return false;
	}
	/*if (m_ornamentidfile) // not implemented
		return false;*/
	if (m_attuned) {
		return false;
	}

	if (RuleI(World, FVNoDropFlag) == FVNoDropFlagRule::Enabled && m_item->FVNoDrop == 0) {
		return true;
	}

	if (m_item->NoDrop == 0) {
		return false;
	}

	if (recurse) {
		for (auto iter: m_contents) {
			if (!iter.second) {
				continue;
			}

			if (!iter.second->IsDroppable(recurse)) {
				return false;
			}
		}
	}

	return true;
}

void EQ::ItemInstance::Initialize(SharedDatabase *db) {
	// if there's no actual item, don't do anything
	if (!m_item) {
		return;
	}

	// initialize scaling items
	if (m_item->CharmFileID != 0) {
		m_scaling = true;
		ScaleItem();
	}

	if (!m_custom_data.empty()) {
		ApplyCustomStats();
	}

	// initialize evolving items
	else if (db && m_item->LoreGroup >= 1000) {
		// not complete yet
	}
}

void EQ::ItemInstance::ScaleItem() {
	if (!m_item)
		return;

	if (m_scaledItem) {
		memcpy(m_scaledItem, m_item, sizeof(ItemData));
	}
	else {
		m_scaledItem = new ItemData(*m_item);
	}

	float Mult = (float)(GetExp()) / 10000;	// scaling is determined by exp, with 10,000 being full stats

	// Pool-distribution of attributes: compute scaled raw values, then redistribute using weights
	{
		struct AttrSlot { const char *name; int baseValue; int scaledRaw; double weight; int allocated; };
		AttrSlot slots[7] = {
			{ "AStr", m_item->AStr, 0, 0.0, 0 },
			{ "ASta", m_item->ASta, 0, 0.0, 0 },
			{ "AAgi", m_item->AAgi, 0, 0.0, 0 },
			{ "ADex", m_item->ADex, 0, 0.0, 0 },
			{ "AInt", m_item->AInt, 0, 0.0, 0 },
			{ "AWis", m_item->AWis, 0, 0.0, 0 },
			{ "ACha", m_item->ACha, 0, 0.0, 0 }
		};
		int pseudoLevel = static_cast<int>(Mult * 100.0f);
		// compute raw scaled values (global curve applied) and weights
		for (int i = 0; i < 7; ++i) {
			int base = slots[i].baseValue;
			if (base <= 0) {
				slots[i].scaledRaw = 0;
			} else {
				double curveMult = ItemScaling::Config::Get().GetGlobalAttrCurve(pseudoLevel);
				slots[i].scaledRaw = static_cast<int>(std::round(base * Mult * curveMult));
			}
			bool present = slots[i].baseValue > 0;
			slots[i].weight = ItemScaling::Config::Get().GetAttributePresenceMultiplier(slots[i].name, present, pseudoLevel);
		}
		// total pool
		int totalPool = 0;
		for (int i = 0; i < 7; ++i) totalPool += slots[i].scaledRaw;
		std::string mode = ItemScaling::Config::Get().GetAttributeBudgetMode();
		if (mode == "static") {
			int sb = ItemScaling::Config::Get().GetAttributeStaticBudget();
			if (sb > 0) totalPool = sb;
		}
		// Apply per-slot multiplier (Chest = 1.5, Wrist = 0.75 etc.) to modify total pool
		double slotMult = ItemScaling::Config::Get().GetSlotMultiplierByMask(m_item ? m_item->Slots : 0);
		if (slotMult > 0.0 && slotMult != 1.0) {
			totalPool = static_cast<int>(std::round(totalPool * slotMult));
		}
		if (totalPool <= 0) {
			// legacy: set each to computed scaledRaw (but still apply cap and presence multiplier)
			auto ApplyStatCapLambda = [](int8 &base_stat, int32 &heroic_stat, int raw_value) {
				const int CAP = 127;
				if (raw_value > CAP) { base_stat = CAP; heroic_stat += (raw_value - CAP); }
				else base_stat = static_cast<int8>(raw_value);
			};
			// set all attributes from scaledRaw * presence multiplier
			double pm; int finalVal;
			pm = ItemScaling::Config::Get().GetAttributePresenceMultiplier("AStr", m_item->AStr > 0, pseudoLevel);
			finalVal = static_cast<int>(std::round(slots[0].scaledRaw * pm)); ApplyStatCapLambda(m_scaledItem->AStr, m_scaledItem->HeroicStr, finalVal);
			pm = ItemScaling::Config::Get().GetAttributePresenceMultiplier("ASta", m_item->ASta > 0, pseudoLevel);
			finalVal = static_cast<int>(std::round(slots[1].scaledRaw * pm)); ApplyStatCapLambda(m_scaledItem->ASta, m_scaledItem->HeroicSta, finalVal);
			pm = ItemScaling::Config::Get().GetAttributePresenceMultiplier("AAgi", m_item->AAgi > 0, pseudoLevel);
			finalVal = static_cast<int>(std::round(slots[2].scaledRaw * pm)); ApplyStatCapLambda(m_scaledItem->AAgi, m_scaledItem->HeroicAgi, finalVal);
			pm = ItemScaling::Config::Get().GetAttributePresenceMultiplier("ADex", m_item->ADex > 0, pseudoLevel);
			finalVal = static_cast<int>(std::round(slots[3].scaledRaw * pm)); ApplyStatCapLambda(m_scaledItem->ADex, m_scaledItem->HeroicDex, finalVal);
			pm = ItemScaling::Config::Get().GetAttributePresenceMultiplier("AInt", m_item->AInt > 0, pseudoLevel);
			finalVal = static_cast<int>(std::round(slots[4].scaledRaw * pm)); ApplyStatCapLambda(m_scaledItem->AInt, m_scaledItem->HeroicInt, finalVal);
			pm = ItemScaling::Config::Get().GetAttributePresenceMultiplier("AWis", m_item->AWis > 0, pseudoLevel);
			finalVal = static_cast<int>(std::round(slots[5].scaledRaw * pm)); ApplyStatCapLambda(m_scaledItem->AWis, m_scaledItem->HeroicWis, finalVal);
			pm = ItemScaling::Config::Get().GetAttributePresenceMultiplier("ACha", m_item->ACha > 0, pseudoLevel);
			finalVal = static_cast<int>(std::round(slots[6].scaledRaw * pm)); ApplyStatCapLambda(m_scaledItem->ACha, m_scaledItem->HeroicCha, finalVal);
		} else {
			// allocate pool with weights
			double totalWeight = 0.0;
			for (int i = 0; i < 7; ++i) totalWeight += slots[i].weight;
			if (totalWeight <= 0.0) {
				// fallback: equal weights
				totalWeight = 7.0;
				for (int i = 0; i < 7; ++i) slots[i].weight = 1.0;
			}
			int remaining = totalPool;
			for (int i = 0; i < 7; ++i) {
				double share = (slots[i].weight / totalWeight) * totalPool;
				int value = static_cast<int>(std::round(share));
				if (i == 6) {
					// last attribute: ensure total sums to totalPool
					value = remaining;
				} else {
					remaining -= value;
				}
				slots[i].allocated = value;
			}
			// apply stat cap and store values
			auto ApplyStatCapLambda = [](int8 &base_stat, int32 &heroic_stat, int raw_value) {
				const int CAP = 127;
				if (raw_value > CAP) { base_stat = CAP; heroic_stat += (raw_value - CAP); }
				else base_stat = static_cast<int8>(raw_value);
			};
			ApplyStatCapLambda(m_scaledItem->AStr, m_scaledItem->HeroicStr, slots[0].allocated);
			ApplyStatCapLambda(m_scaledItem->ASta, m_scaledItem->HeroicSta, slots[1].allocated);
			ApplyStatCapLambda(m_scaledItem->AAgi, m_scaledItem->HeroicAgi, slots[2].allocated);
			ApplyStatCapLambda(m_scaledItem->ADex, m_scaledItem->HeroicDex, slots[3].allocated);
			ApplyStatCapLambda(m_scaledItem->AInt, m_scaledItem->HeroicInt, slots[4].allocated);
			ApplyStatCapLambda(m_scaledItem->AWis, m_scaledItem->HeroicWis, slots[5].allocated);
			ApplyStatCapLambda(m_scaledItem->ACha, m_scaledItem->HeroicCha, slots[6].allocated);
		}
	}


	// Resistances: apply per-slot resist multipliers if configured
	uint32_t slots_mask = m_item ? m_item->Slots : 0;
	double resDefault = ItemScaling::Config::Get().GetSlotMultiplierByMask(slots_mask, "Resists");
	double frSlot = ItemScaling::Config::Get().GetSlotMultiplierByMask(slots_mask, "FR");
	double mrSlot = ItemScaling::Config::Get().GetSlotMultiplierByMask(slots_mask, "MR");
	double prSlot = ItemScaling::Config::Get().GetSlotMultiplierByMask(slots_mask, "PR");
	double crSlot = ItemScaling::Config::Get().GetSlotMultiplierByMask(slots_mask, "CR");
	double drSlot = ItemScaling::Config::Get().GetSlotMultiplierByMask(slots_mask, "DR");
	m_scaledItem->MR = static_cast<int8>((float)m_item->MR * Mult * (mrSlot > 0.0 ? mrSlot : resDefault));
	m_scaledItem->PR = static_cast<int8>((float)m_item->PR * Mult * (prSlot > 0.0 ? prSlot : resDefault));
	m_scaledItem->DR = static_cast<int8>((float)m_item->DR * Mult * (drSlot > 0.0 ? drSlot : resDefault));
	m_scaledItem->CR = static_cast<int8>((float)m_item->CR * Mult * (crSlot > 0.0 ? crSlot : resDefault));
	m_scaledItem->FR = static_cast<int8>((float)m_item->FR * Mult * (frSlot > 0.0 ? frSlot : resDefault));

	m_scaledItem->HP = (int32)((float)m_item->HP*Mult);
	// Apply per-slot HP multiplier
	double hpSlot = ItemScaling::Config::Get().GetSlotMultiplierByMask(slots_mask, "HP");
	if (hpSlot != 1.0) {
		m_scaledItem->HP = static_cast<int32>(std::round(static_cast<double>(m_scaledItem->HP) * hpSlot));
	}
	m_scaledItem->Mana = (int32)((float)m_item->Mana*Mult);
	m_scaledItem->AC = (int32)((float)m_item->AC*Mult);
	// Apply per-slot AC multiplier
	double acSlot = ItemScaling::Config::Get().GetSlotMultiplierByMask(slots_mask, "AC");
	if (acSlot != 1.0) {
		m_scaledItem->AC = static_cast<int32>(std::round(static_cast<double>(m_scaledItem->AC) * acSlot));
	}

	// check these..some may not need to be modified (really need to check all stats/bonuses)
	//m_scaledItem->SkillModValue = (int32)((float)m_item->SkillModValue*Mult);
	//m_scaledItem->BaneDmgAmt = (int8)((float)m_item->BaneDmgAmt*Mult);	// watch (10 entries with charmfileid)
	m_scaledItem->BardValue = (int32)((float)m_item->BardValue*Mult);		// watch (no entries with charmfileid)
	m_scaledItem->ElemDmgAmt = (uint8)((float)m_item->ElemDmgAmt*Mult);		// watch (no entries with charmfileid)
	m_scaledItem->Damage = (uint32)((float)m_item->Damage*Mult);			// watch
	{
		int pseudoLevel = static_cast<int>(Mult * 100.0f);
		double wmult = ItemScaling::Config::Get().GetWeaponDamageCurve(pseudoLevel);
		m_scaledItem->Damage = static_cast<uint32>(static_cast<float>(m_item->Damage) * static_cast<float>(Mult * wmult));
		// Apply per-slot Damage multiplier
		double dmgSlot = ItemScaling::Config::Get().GetSlotMultiplierByMask(slots_mask, "Damage");
		if (dmgSlot != 1.0) {
			m_scaledItem->Damage = static_cast<uint32>(std::round(static_cast<double>(m_scaledItem->Damage) * dmgSlot));
			if (m_scaledItem->Damage < 1) m_scaledItem->Damage = 1;
		}
	}

	m_scaledItem->CombatEffects = (int8)((float)m_item->CombatEffects*Mult);
	m_scaledItem->Shielding = (int8)((float)m_item->Shielding*Mult);
	{
		int pseudoLevel = static_cast<int>(Mult * 100.0f);
		double mm = ItemScaling::Config::Get().GetMod2Curve("Shielding", pseudoLevel);
		m_scaledItem->Shielding = static_cast<int8>(static_cast<int>(m_item->Shielding * Mult * mm));
		// Apply per-slot Shielding multiplier
		double shieldSlot = ItemScaling::Config::Get().GetSlotMultiplierByMask(slots_mask, "Shielding");
		if (shieldSlot != 1.0) {
			m_scaledItem->Shielding = static_cast<int8>(std::round(static_cast<double>(m_scaledItem->Shielding) * shieldSlot));
		}
	}
	m_scaledItem->StunResist = (int8)((float)m_item->StunResist*Mult);
	m_scaledItem->StrikeThrough = (int8)((float)m_item->StrikeThrough*Mult);
	{
		int pseudoLevel = static_cast<int>(Mult * 100.0f);
		double mm = ItemScaling::Config::Get().GetMod2Curve("StrikeThrough", pseudoLevel);
		m_scaledItem->StrikeThrough = static_cast<int8>(static_cast<int>(m_item->StrikeThrough * Mult * mm));
	}
	m_scaledItem->ExtraDmgAmt = (uint32)((float)m_item->ExtraDmgAmt*Mult);
	m_scaledItem->SpellShield = (int8)((float)m_item->SpellShield*Mult);
	m_scaledItem->Avoidance = (int8)((float)m_item->Avoidance*Mult);
	m_scaledItem->Accuracy = (int8)((float)m_item->Accuracy*Mult);

	m_scaledItem->FactionAmt1 = (int32)((float)m_item->FactionAmt1*Mult);
	m_scaledItem->FactionAmt2 = (int32)((float)m_item->FactionAmt2*Mult);
	m_scaledItem->FactionAmt3 = (int32)((float)m_item->FactionAmt3*Mult);
	m_scaledItem->FactionAmt4 = (int32)((float)m_item->FactionAmt4*Mult);

	m_scaledItem->Endur = (uint32)((float)m_item->Endur*Mult);
	m_scaledItem->DotShielding = (uint32)((float)m_item->DotShielding*Mult);
	if (m_scaledItem->DotShielding > 0) {
		double dsSlot = ItemScaling::Config::Get().GetSlotMultiplierByMask(slots_mask, "DotShielding");
		if (dsSlot != 1.0) {
			m_scaledItem->DotShielding = static_cast<int32>(std::round(static_cast<double>(m_scaledItem->DotShielding) * dsSlot));
		}
	}
	m_scaledItem->Attack = (uint32)((float)m_item->Attack*Mult);
	{
		int pseudoLevel = static_cast<int>(Mult * 100.0f);
		double wmult = ItemScaling::Config::Get().GetWeaponAttackCurve(pseudoLevel);
		m_scaledItem->Attack = static_cast<uint32>(static_cast<float>(m_item->Attack) * static_cast<float>(Mult * wmult));
		// Apply per-slot Attack multiplier
		double atkSlot = ItemScaling::Config::Get().GetSlotMultiplierByMask(slots_mask, "Attack");
		if (atkSlot != 1.0) {
			m_scaledItem->Attack = static_cast<uint32>(std::round(static_cast<double>(m_scaledItem->Attack) * atkSlot));
		}
	}
	m_scaledItem->Regen = (uint32)((float)m_item->Regen*Mult);
	m_scaledItem->ManaRegen = (uint32)((float)m_item->ManaRegen*Mult);
	m_scaledItem->EnduranceRegen = (uint32)((float)m_item->EnduranceRegen*Mult);
	m_scaledItem->Haste = (uint32)((float)m_item->Haste*Mult);
	m_scaledItem->DamageShield = (uint32)((float)m_item->DamageShield*Mult);

	m_scaledItem->Purity = (uint32)((float)m_item->Purity*Mult);
	m_scaledItem->BackstabDmg = (uint32)((float)m_item->BackstabDmg*Mult);
	m_scaledItem->DSMitigation = (uint32)((float)m_item->DSMitigation*Mult);
	m_scaledItem->HeroicStr = (int32)((float)m_item->HeroicStr*Mult);
	m_scaledItem->HeroicInt = (int32)((float)m_item->HeroicInt*Mult);
	m_scaledItem->HeroicWis = (int32)((float)m_item->HeroicWis*Mult);
	m_scaledItem->HeroicAgi = (int32)((float)m_item->HeroicAgi*Mult);
	m_scaledItem->HeroicDex = (int32)((float)m_item->HeroicDex*Mult);
	m_scaledItem->HeroicSta = (int32)((float)m_item->HeroicSta*Mult);
	m_scaledItem->HeroicCha = (int32)((float)m_item->HeroicCha*Mult);
	m_scaledItem->HeroicMR = (int32)((float)m_item->HeroicMR*Mult);
	m_scaledItem->HeroicFR = (int32)((float)m_item->HeroicFR*Mult);
	m_scaledItem->HeroicCR = (int32)((float)m_item->HeroicCR*Mult);
	m_scaledItem->HeroicDR = (int32)((float)m_item->HeroicDR*Mult);
	m_scaledItem->HeroicPR = (int32)((float)m_item->HeroicPR*Mult);
	m_scaledItem->HeroicSVCorrup = (int32)((float)m_item->HeroicSVCorrup*Mult);
	m_scaledItem->HealAmt = (int32)((float)m_item->HealAmt*Mult);
	m_scaledItem->SpellDmg = (int32)((float)m_item->SpellDmg*Mult);
	m_scaledItem->Clairvoyance = (uint32)((float)m_item->Clairvoyance*Mult);

	// Derived caster stats: derive SpellDmg from INT, HealAmt from WIS
	// Use the ItemScaling Config to compute derived values (supports 'divisor' and 'curve' modes)
	int pseudoLevel = static_cast<int>(Mult * 100.0f);
	int total_int = static_cast<int>(m_scaledItem->AInt) + static_cast<int>(m_scaledItem->HeroicInt);
	int total_wis = static_cast<int>(m_scaledItem->AWis) + static_cast<int>(m_scaledItem->HeroicWis);
	if (total_int > 0) {
		int32 derivedSpell = ItemScaling::Config::Get().ComputeSpellDmgFromInt(total_int, pseudoLevel);
		if (derivedSpell > 0) m_scaledItem->SpellDmg += derivedSpell;
	}
	if (total_wis > 0) {
		int32 derivedSpellWis = ItemScaling::Config::Get().ComputeSpellDmgFromInt(total_wis, pseudoLevel);
		if (derivedSpellWis > 0) m_scaledItem->SpellDmg += derivedSpellWis;
		int32 derivedHeal = ItemScaling::Config::Get().ComputeHealFromWis(total_wis, pseudoLevel);
		if (derivedHeal > 0) m_scaledItem->HealAmt += derivedHeal;
	}

	// Apply per-slot SpellDmg & HealAmt multipliers
	double spellSlot = ItemScaling::Config::Get().GetSlotMultiplierByMask(slots_mask, "SpellDmg");
	double healSlot = ItemScaling::Config::Get().GetSlotMultiplierByMask(slots_mask, "HealAmt");
	if (spellSlot != 1.0) {
		m_scaledItem->SpellDmg = static_cast<int32>(std::round(static_cast<double>(m_scaledItem->SpellDmg) * spellSlot));
	}
	if (healSlot != 1.0) {
		m_scaledItem->HealAmt = static_cast<int32>(std::round(static_cast<double>(m_scaledItem->HealAmt) * healSlot));
	}

	// Log derived values for debugging
	{
		std::ofstream logfile("logs/inf/item_scaling.log", std::ios::app);
		if (logfile.is_open()) {
			logfile << "ScaleItem: ItemID=" << (m_item ? m_item->ID : 0)
			<< " PseudoLevel=" << static_cast<int>(Mult * 100.0f)
			<< " SpellDmg=" << m_scaledItem->SpellDmg << " HealAmt=" << m_scaledItem->HealAmt << std::endl;
			logfile.close();
		}
	}


	m_scaledItem->CharmFileID = 0;	// this stops the client from trying to scale the item itself.
}

// Scale a dynamic item based on level
// This is a standalone version of the DynamicItemManager scaling logic that works in common code
void EQ::ItemInstance::ScaleDynamicItem(int level) {
	if (!m_item || level <= 0) return;

	// Safety check - validate m_item has required fields
	if (m_item->ID == 0 || !m_item->Name[0]) {
		std::ofstream logfile("logs/inf/scaling_error.log", std::ios::app);
		if (logfile.is_open()) {
			logfile << "ERROR: ScaleDynamicItem called with invalid base item (ID=" << m_item->ID << ")" << std::endl;
			logfile.close();
		}
		return;
	}

	// Create or reset m_scaledItem
	if (m_scaledItem) {
		memcpy(m_scaledItem, m_item, sizeof(ItemData));
	}
	else {
		m_scaledItem = new ItemData(*m_item);
	}

	// Verify m_scaledItem was created successfully
	if (!m_scaledItem) {
		std::ofstream logfile("logs/inf/scaling_error.log", std::ios::app);
		if (logfile.is_open()) {
			logfile << "ERROR: Failed to allocate m_scaledItem for item " << m_item->ID << std::endl;
			logfile.close();
		}
		return;
	}

	// Helper lambda for tiered stat calculation
	auto CalculateTieredStat = [](int base_value, int level, int base_increment, int tier_bonus, int tier_size = 10) -> int {
		if (level <= 0) return base_value;
		int total = base_value;
		int tier = level / tier_size;
		// Add full tiers
		for (int t = 0; t < tier; t++) {
			int increment = base_increment + (t * tier_bonus);
			total += tier_size * increment;
		}
		// Add remaining levels
		int remaining_levels = level % tier_size;
		int current_tier_increment = base_increment + (tier * tier_bonus);
		total += remaining_levels * current_tier_increment;
		return total;
	};

	// Helper lambda for stat capping (127 base + heroic overflow)
	auto ApplyStatCap = [](int8& base_stat, int32& heroic_stat, int raw_value) {
		const int CAP = 127;
		if (raw_value > CAP) {
			base_stat = CAP;
			heroic_stat += (raw_value - CAP);
		} else {
			base_stat = static_cast<int8>(raw_value);
		}
	};

	// Primary stats with tiered scaling * JSON curves * slot multipliers (mirrors DynamicItemManager)
	uint32_t slots_mask = m_item ? m_item->Slots : 0;
	auto primaryCurve = [&](const std::string &key) { return ItemScaling::Config::Get().GetMod2Curve(key, level); };
	double ac_base_factor = 1.0 + (static_cast<double>(m_item->AC) / 200.0); // emphasize base AC
	m_scaledItem->AC = static_cast<int32>(std::round(CalculateTieredStat(m_item->AC, level, 1, 1) * primaryCurve("AC") * ac_base_factor));
	double hp_base_factor = 1.0 + (static_cast<double>(m_item->HP) / 500.0); // emphasize base HP
	m_scaledItem->HP = static_cast<int32>(std::round(CalculateTieredStat(m_item->HP, level, 4, 4) * primaryCurve("HP") * hp_base_factor));
	m_scaledItem->Mana = static_cast<int32>(std::round(CalculateTieredStat(m_item->Mana, level, 1, 1) * primaryCurve("Mana")));
	m_scaledItem->Endur = static_cast<int32>(std::round(CalculateTieredStat(m_item->Endur, level, 4, 4) * primaryCurve("Endur")));
	// Apply per-slot primary multipliers (AC, HP, Mana, Endur)
	double hpSlotMult = ItemScaling::Config::Get().GetSlotMultiplierByMask(slots_mask, "HP");
	double acSlotMult = ItemScaling::Config::Get().GetSlotMultiplierByMask(slots_mask, "AC");
	double manaSlotMult = ItemScaling::Config::Get().GetSlotMultiplierByMask(slots_mask, "Mana");
	double endurSlotMult = ItemScaling::Config::Get().GetSlotMultiplierByMask(slots_mask, "Endur");
	if (hpSlotMult != 1.0) m_scaledItem->HP = static_cast<int32>(std::round(static_cast<double>(m_scaledItem->HP) * hpSlotMult));
	if (acSlotMult != 1.0) m_scaledItem->AC = static_cast<int32>(std::round(static_cast<double>(m_scaledItem->AC) * acSlotMult));
	if (manaSlotMult != 1.0) m_scaledItem->Mana = static_cast<int32>(std::round(static_cast<double>(m_scaledItem->Mana) * manaSlotMult));
	if (endurSlotMult != 1.0) m_scaledItem->Endur = static_cast<int32>(std::round(static_cast<double>(m_scaledItem->Endur) * endurSlotMult));

	// Attribute stats with 127 cap + heroic overflow
	{
		// compute raw scaled values per attribute and allocate via configured pool mode
		struct AttrSlot { const char *name; int baseValue; int rawScaled; double weight; int allocated; };
		AttrSlot slots[7] = {
			{ "AStr", m_item->AStr, 0, 0.0, 0 },
			{ "ASta", m_item->ASta, 0, 0.0, 0 },
			{ "AAgi", m_item->AAgi, 0, 0.0, 0 },
			{ "ADex", m_item->ADex, 0, 0.0, 0 },
			{ "AInt", m_item->AInt, 0, 0.0, 0 },
			{ "AWis", m_item->AWis, 0, 0.0, 0 },
			{ "ACha", m_item->ACha, 0, 0.0, 0 }
		};
		for (int i = 0; i < 7; ++i) {
			int raw = CalculateTieredStat(slots[i].baseValue, level, 1, 1);
			double curveMult = ItemScaling::Config::Get().GetGlobalAttrCurve(level);
			double pref = ItemScaling::Config::Get().GetAttributePresenceMultiplier(slots[i].name, slots[i].baseValue > 0, level);
			double baseFactor = 1.0 + (static_cast<double>(slots[i].baseValue) / 22.0);
			slots[i].rawScaled = static_cast<int>(std::round(raw * curveMult * pref * baseFactor));
			bool present = slots[i].baseValue > 0;
			slots[i].weight = ItemScaling::Config::Get().GetAttributePresenceMultiplier(slots[i].name, present, level);
		}
		int totalPool = 0;
		for (int i = 0; i < 7; ++i) totalPool += slots[i].rawScaled;
		std::string mode = ItemScaling::Config::Get().GetAttributeBudgetMode();
		if (mode == "static") {
			int sb = ItemScaling::Config::Get().GetAttributeStaticBudget();
			if (sb > 0) totalPool = sb;
		}
		// Apply per-slot multiplier using the base item's slot mask
		uint32_t slots_mask = m_item ? m_item->Slots : 0;
		double slotMult = ItemScaling::Config::Get().GetSlotMultiplierByMask(slots_mask);
		if (slotMult > 0.0 && slotMult != 1.0) {
			totalPool = static_cast<int>(std::round(totalPool * slotMult));
		}
		if (totalPool <= 0) {
			// legacy behavior: assign computed rawScaled * presence multiplier
			auto ApplyStatCapLambda = [](int8 &base_stat, int32 &heroic_stat, int raw_value) {
				const int CAP = 127;
				if (raw_value > CAP) { base_stat = CAP; heroic_stat += (raw_value - CAP); }
				else base_stat = static_cast<int8>(raw_value);
			};
			double pm; int finalVal;
			pm = ItemScaling::Config::Get().GetAttributePresenceMultiplier("AStr", m_item->AStr > 0, level);
			finalVal = static_cast<int>(std::round(slots[0].rawScaled * pm)); ApplyStatCapLambda(m_scaledItem->AStr, m_scaledItem->HeroicStr, finalVal);
			pm = ItemScaling::Config::Get().GetAttributePresenceMultiplier("ASta", m_item->ASta > 0, level);
			finalVal = static_cast<int>(std::round(slots[1].rawScaled * pm)); ApplyStatCapLambda(m_scaledItem->ASta, m_scaledItem->HeroicSta, finalVal);
			pm = ItemScaling::Config::Get().GetAttributePresenceMultiplier("AAgi", m_item->AAgi > 0, level);
			finalVal = static_cast<int>(std::round(slots[2].rawScaled * pm)); ApplyStatCapLambda(m_scaledItem->AAgi, m_scaledItem->HeroicAgi, finalVal);
			pm = ItemScaling::Config::Get().GetAttributePresenceMultiplier("ADex", m_item->ADex > 0, level);
			finalVal = static_cast<int>(std::round(slots[3].rawScaled * pm)); ApplyStatCapLambda(m_scaledItem->ADex, m_scaledItem->HeroicDex, finalVal);
			pm = ItemScaling::Config::Get().GetAttributePresenceMultiplier("AInt", m_item->AInt > 0, level);
			finalVal = static_cast<int>(std::round(slots[4].rawScaled * pm)); ApplyStatCapLambda(m_scaledItem->AInt, m_scaledItem->HeroicInt, finalVal);
			pm = ItemScaling::Config::Get().GetAttributePresenceMultiplier("AWis", m_item->AWis > 0, level);
			finalVal = static_cast<int>(std::round(slots[5].rawScaled * pm)); ApplyStatCapLambda(m_scaledItem->AWis, m_scaledItem->HeroicWis, finalVal);
			pm = ItemScaling::Config::Get().GetAttributePresenceMultiplier("ACha", m_item->ACha > 0, level);
			finalVal = static_cast<int>(std::round(slots[6].rawScaled * pm)); ApplyStatCapLambda(m_scaledItem->ACha, m_scaledItem->HeroicCha, finalVal);
		} else {
			double totalWeight = 0.0;
			for (int i = 0; i < 7; ++i) totalWeight += slots[i].weight;
			if (totalWeight <= 0.0) { totalWeight = 7.0; for (int i = 0; i < 7; ++i) slots[i].weight = 1.0; }
			int remaining = totalPool;
			for (int i = 0; i < 7; ++i) {
				double share = (slots[i].weight / totalWeight) * totalPool;
				int val = static_cast<int>(std::round(share));
				if (i == 6) { val = remaining; } else { remaining -= val; }
				slots[i].allocated = val;
			}
			auto ApplyStatCapLambda = [](int8 &base_stat, int32 &heroic_stat, int raw_value) {
				const int CAP = 127;
				if (raw_value > CAP) { base_stat = CAP; heroic_stat += (raw_value - CAP); }
				else base_stat = static_cast<int8>(raw_value);
			};
			ApplyStatCapLambda(m_scaledItem->AStr, m_scaledItem->HeroicStr, slots[0].allocated);
			ApplyStatCapLambda(m_scaledItem->ASta, m_scaledItem->HeroicSta, slots[1].allocated);
			ApplyStatCapLambda(m_scaledItem->AAgi, m_scaledItem->HeroicAgi, slots[2].allocated);
			ApplyStatCapLambda(m_scaledItem->ADex, m_scaledItem->HeroicDex, slots[3].allocated);
			ApplyStatCapLambda(m_scaledItem->AInt, m_scaledItem->HeroicInt, slots[4].allocated);
			ApplyStatCapLambda(m_scaledItem->AWis, m_scaledItem->HeroicWis, slots[5].allocated);
			ApplyStatCapLambda(m_scaledItem->ACha, m_scaledItem->HeroicCha, slots[6].allocated);
		}
	}

	// Derived caster stats for dynamic items - use configured curve/divisor (base + heroic)
	if (m_item->AInt > 0) {
		int total_int = static_cast<int>(m_scaledItem->AInt) + static_cast<int>(m_scaledItem->HeroicInt);
		int32 derivedSpell = ItemScaling::Config::Get().ComputeSpellDmgFromInt(total_int, level);
		if (derivedSpell > 0) m_scaledItem->SpellDmg += derivedSpell;
	}
	if (m_item->AWis > 0) {
		int total_wis = static_cast<int>(m_scaledItem->AWis) + static_cast<int>(m_scaledItem->HeroicWis);
		int32 derivedHeal = ItemScaling::Config::Get().ComputeHealFromWis(total_wis, level);
		if (derivedHeal > 0) m_scaledItem->HealAmt += derivedHeal;
	}
    // Apply per-slot SpellDmg and HealAmt multipliers
    double spellMult = ItemScaling::Config::Get().GetSlotMultiplierByMask(slots_mask, "SpellDmg");
    if (spellMult != 1.0 && m_scaledItem->SpellDmg > 0) {
        m_scaledItem->SpellDmg = static_cast<int32>(std::round(static_cast<double>(m_scaledItem->SpellDmg) * spellMult));
    }
    double healMult = ItemScaling::Config::Get().GetSlotMultiplierByMask(slots_mask, "HealAmt");
    if (healMult != 1.0 && m_scaledItem->HealAmt > 0) {
        m_scaledItem->HealAmt = static_cast<int32>(std::round(static_cast<double>(m_scaledItem->HealAmt) * healMult));
    }

	// Log derived values for debugging
	{
		std::ofstream logfile("logs/inf/item_scaling.log", std::ios::app);
		if (logfile.is_open()) {
			logfile << "ScaleDynamicItem: ItemID=" << (m_item ? m_item->ID : 0)
			<< " Level=" << level
			<< " SpellDmg=" << m_scaledItem->SpellDmg << " HealAmt=" << m_scaledItem->HealAmt << std::endl;
			logfile.close();
		}
	}

	// === Weapon Stats ===
	// Check if this is a weapon (has Damage and Delay, or is an arrow/throwing weapon)
	bool is_weapon = (m_item->Damage > 0 && m_item->Delay > 0) ||
	                 (m_item->ItemType == EQ::item::ItemTypeArrow ||
	                  m_item->ItemType == EQ::item::ItemTypeLargeThrowing ||
	                  m_item->ItemType == EQ::item::ItemTypeSmallThrowing);

	if (is_weapon) {
		// Scale weapon damage using RATIO-BASED scaling
		// This ensures fast 1H weapons and slow 2H weapons scale proportionally
		// We scale based on damage-per-delay ratio to maintain balance
		if (m_item->Damage > 0 && m_item->Delay > 0) {
			// Calculate base ratio (damage per delay tick)
			float base_ratio = static_cast<float>(m_item->Damage) / static_cast<float>(m_item->Delay);

			// Aggressive scaling: each level multiplies damage significantly
			// At level 100: ~8x multiplier (30dmg -> 240dmg)
			// At level 250: ~65x multiplier (30dmg -> 1950dmg)
			// Formula: 1 + (level * 0.26) gives us the scaling curve we want
			float ratio_multiplier = 1.0f + (level * 0.10f);  // softened to pair with JSON curve
			double wmult = ItemScaling::Config::Get().GetWeaponDamageCurve(level);

			// Calculate new damage based on scaled ratio
			float new_damage = base_ratio * static_cast<float>(ratio_multiplier * wmult) * m_item->Delay;

			// Round to nearest integer (important for low-damage weapons)
			m_scaledItem->Damage = static_cast<uint32>(new_damage + 0.5f);

			// Ensure minimum: at least base damage + 1 so tiny weapons still grow
			uint32 floorVal = static_cast<uint32>(m_item->Damage + 1);
			if (m_scaledItem->Damage < floorVal) m_scaledItem->Damage = floorVal;
			// Apply per-slot Damage multiplier for weapons
			double dmgSlot = ItemScaling::Config::Get().GetSlotMultiplierByMask(slots_mask, "Damage");
			if (dmgSlot != 1.0) {
				m_scaledItem->Damage = static_cast<uint32>(std::round(static_cast<double>(m_scaledItem->Damage) * dmgSlot));
				if (m_scaledItem->Damage < floorVal) m_scaledItem->Damage = floorVal;
			}
		}

		// Scale Attack stat (ATK bonus) - modest scaling
		if (m_item->Attack > 0) {
			int raw = CalculateTieredStat(m_item->Attack, level, 1, 1);
			double amult = ItemScaling::Config::Get().GetWeaponAttackCurve(level);
			m_scaledItem->Attack = static_cast<uint32>(static_cast<int>(raw * amult));
			// Per-slot Attack multiplier
			double atkSlot = ItemScaling::Config::Get().GetSlotMultiplierByMask(slots_mask, "Attack");
			if (atkSlot != 1.0) {
				m_scaledItem->Attack = static_cast<uint32>(std::round(static_cast<double>(m_scaledItem->Attack) * atkSlot));
			}
		}

		// DON'T scale elemental/bane damage - these have specific types/targets
		// The base item defines the element/bane type, and scaling would need
		// to preserve those relationships. Better to leave these as-is.

		// Scale Backstab Damage (rogue weapons) - ratio-based like normal damage
		if (m_item->BackstabDmg > 0) {
			float backstab_mult = 1.0f + (level * 0.26f);  // Same 26% as weapon damage
			m_scaledItem->BackstabDmg = static_cast<uint32>((m_item->BackstabDmg * backstab_mult) + 0.5f);
		}

		// Scale Proc Rate (if item has a proc)
		// ProcRate is a percentage modifier, we scale it slowly
		// A weapon with ProcRate 100 at level 100 should be around 150 (50% increase)
		if (m_item->ProcRate > 0) {
			// Add 0.5% per level (level 100 = +50%, level 250 = +125%)
			int proc_increase = (level * m_item->ProcRate) / 200;  // 0.5% per level
			m_scaledItem->ProcRate = m_item->ProcRate + proc_increase;
		}

		// Note: We do NOT scale Delay - keeping weapon speed constant is important for balance
		// Note: We do NOT scale ElemDmg, BaneDmg, or ExtraDmg - these are specialized
		// and scaling them could cause balance issues with their specific mechanics
	}

	// === Bard Instrument Modifiers (can be on ANY item type) ===
	// Scale Bard instruments (BardValue) - appears on weapons, armor, jewelry, etc.
	// BardValue is in 10ths (38 = 3.8 modifier), so we scale aggressively for bard progression
	// Tiered scaling: 5% per level to 200%, then 2% to 400%, then 1% after
	if (m_item->BardValue > 0) {
		float bard_mult = 1.0f;

		if (level <= 40) {
			// Levels 1-40: 5% per level (40 levels = 200% total = 3x multiplier)
			bard_mult = 1.0f + (level * 0.05f);
		} else if (level <= 140) {
			// Levels 41-140: Start at 3x, add 2% per level for 100 levels (200% more = 5x total)
			bard_mult = 3.0f + ((level - 40) * 0.02f);
		} else {
			// Levels 141+: Start at 5x, add 1% per level (110 more levels = 6.1x at 250)
			bard_mult = 5.0f + ((level - 140) * 0.01f);
		}

		m_scaledItem->BardValue = static_cast<int32>((m_item->BardValue * bard_mult) + 0.5f);
	}

	// Resistances with cap
	const int resist_cap = 500;
	if (m_item->FR > 0) {
		int raw = CalculateTieredStat(m_item->FR, level, 1, 1);
		double frSlot = ItemScaling::Config::Get().GetSlotMultiplierByMask(slots_mask, "FR");
		double resistSlotDefault = ItemScaling::Config::Get().GetSlotMultiplierByMask(slots_mask, "Resists");
		double useMult = (frSlot != 0.0 ? frSlot : resistSlotDefault);
		if (useMult != 1.0) raw = static_cast<int>(std::round(raw * useMult));
		m_scaledItem->FR = std::min(raw, resist_cap);
	}
	if (m_item->CR > 0) {
		int raw = CalculateTieredStat(m_item->CR, level, 1, 1);
		double crSlot = ItemScaling::Config::Get().GetSlotMultiplierByMask(slots_mask, "CR");
		double resistSlotDefault = ItemScaling::Config::Get().GetSlotMultiplierByMask(slots_mask, "Resists");
		double useMult = (crSlot != 0.0 ? crSlot : resistSlotDefault);
		if (useMult != 1.0) raw = static_cast<int>(std::round(raw * useMult));
		m_scaledItem->CR = std::min(raw, resist_cap);
	}
	if (m_item->MR > 0) {
		int raw = CalculateTieredStat(m_item->MR, level, 1, 1);
		double mrSlot = ItemScaling::Config::Get().GetSlotMultiplierByMask(slots_mask, "MR");
		double resistSlotDefault = ItemScaling::Config::Get().GetSlotMultiplierByMask(slots_mask, "Resists");
		double useMult = (mrSlot != 0.0 ? mrSlot : resistSlotDefault);
		if (useMult != 1.0) raw = static_cast<int>(std::round(raw * useMult));
		m_scaledItem->MR = std::min(raw, resist_cap);
	}
	if (m_item->PR > 0) {
		int raw = CalculateTieredStat(m_item->PR, level, 1, 1);
		double prSlot = ItemScaling::Config::Get().GetSlotMultiplierByMask(slots_mask, "PR");
		double resistSlotDefault = ItemScaling::Config::Get().GetSlotMultiplierByMask(slots_mask, "Resists");
		double useMult = (prSlot != 0.0 ? prSlot : resistSlotDefault);
		if (useMult != 1.0) raw = static_cast<int>(std::round(raw * useMult));
		m_scaledItem->PR = std::min(raw, resist_cap);
	}
	if (m_item->DR > 0) {
		int raw = CalculateTieredStat(m_item->DR, level, 1, 1);
		double drSlot = ItemScaling::Config::Get().GetSlotMultiplierByMask(slots_mask, "DR");
		double resistSlotDefault = ItemScaling::Config::Get().GetSlotMultiplierByMask(slots_mask, "Resists");
		double useMult = (drSlot != 0.0 ? drSlot : resistSlotDefault);
		if (useMult != 1.0) raw = static_cast<int>(std::round(raw * useMult));
		m_scaledItem->DR = std::min(raw, resist_cap);
	}

	// Caster stats (aggressive scaling ~5 per level)
	if (m_item->HealAmt > 0) {
		m_scaledItem->HealAmt = CalculateTieredStat(m_item->HealAmt, level, 5, 5);
	}
	if (m_item->SpellDmg > 0) {
		m_scaledItem->SpellDmg = CalculateTieredStat(m_item->SpellDmg, level, 5, 5);
	}
		// Apply per-slot caster multipliers
		double spellSlot = ItemScaling::Config::Get().GetSlotMultiplierByMask(slots_mask, "SpellDmg");
		if (spellSlot != 1.0 && m_scaledItem->SpellDmg > 0) m_scaledItem->SpellDmg = static_cast<int32>(std::round(static_cast<double>(m_scaledItem->SpellDmg) * spellSlot));
		double healSlot = ItemScaling::Config::Get().GetSlotMultiplierByMask(slots_mask, "HealAmt");
		if (healSlot != 1.0 && m_scaledItem->HealAmt > 0) m_scaledItem->HealAmt = static_cast<int32>(std::round(static_cast<double>(m_scaledItem->HealAmt) * healSlot));

	// Damage Shield and Dot Shielding (moderate scaling)
	if (m_item->DamageShield > 0) {
		m_scaledItem->DamageShield = CalculateTieredStat(m_item->DamageShield, level, 2, 2);
	}
	if (m_item->DotShielding > 0) {
		m_scaledItem->DotShielding = CalculateTieredStat(m_item->DotShielding, level, 2, 2);
	}

	// Regen stats (moderate scaling ~1 per level)
	if (m_item->ManaRegen > 0) {
		m_scaledItem->ManaRegen = CalculateTieredStat(m_item->ManaRegen, level, 1, 1);
	}
	if (m_item->EnduranceRegen > 0) {
		m_scaledItem->EnduranceRegen = CalculateTieredStat(m_item->EnduranceRegen, level, 1, 1);
	}

	// Heroic Resistances (moderate scaling)
	if (m_item->HeroicMR > 0) {
		m_scaledItem->HeroicMR = CalculateTieredStat(m_item->HeroicMR, level, 1, 1);
	}
	if (m_item->HeroicFR > 0) {
		m_scaledItem->HeroicFR = CalculateTieredStat(m_item->HeroicFR, level, 1, 1);
	}
	if (m_item->HeroicCR > 0) {
		m_scaledItem->HeroicCR = CalculateTieredStat(m_item->HeroicCR, level, 1, 1);
	}
	if (m_item->HeroicDR > 0) {
		m_scaledItem->HeroicDR = CalculateTieredStat(m_item->HeroicDR, level, 1, 1);
	}
	if (m_item->HeroicPR > 0) {
		m_scaledItem->HeroicPR = CalculateTieredStat(m_item->HeroicPR, level, 1, 1);
	}
	if (m_item->HeroicSVCorrup > 0) {
		m_scaledItem->HeroicSVCorrup = CalculateTieredStat(m_item->HeroicSVCorrup, level, 1, 1);
	}

	// === Milestone Bonuses ===

	// Haste (only on waist, back, range slots - linear 1:1 scaling)
	bool is_haste_slot = false;
	if (m_item->Slots) {
		is_haste_slot = (m_item->Slots & (1 << EQ::invslot::slotWaist)) ||
		                (m_item->Slots & (1 << EQ::invslot::slotBack)) ||
		                (m_item->Slots & (1 << EQ::invslot::slotRange));
	}
	if (is_haste_slot && level >= 1) {
		int haste = 0;
		if (level <= 100) {
			haste = level;  // Linear 1:1 scaling
		} else {
			haste = 100 + (level - 100) / 10;  // Slow increase beyond 100
		}
		m_scaledItem->Haste = std::min(haste, 150);  // Cap at 150
	}

	// Heroic stats from milestones (bonus beyond overflow)
	if (level >= 10) {  // Start at level 10
		int milestone_heroic = (level - 10) / 10;  // +1 per 10 levels
		if (m_item->AStr > 0 || m_scaledItem->HeroicStr > 0) m_scaledItem->HeroicStr += milestone_heroic;
		if (m_item->ASta > 0 || m_scaledItem->HeroicSta > 0) m_scaledItem->HeroicSta += milestone_heroic;
		if (m_item->AAgi > 0 || m_scaledItem->HeroicAgi > 0) m_scaledItem->HeroicAgi += milestone_heroic;
		if (m_item->ADex > 0 || m_scaledItem->HeroicDex > 0) m_scaledItem->HeroicDex += milestone_heroic;
		if (m_item->AInt > 0 || m_scaledItem->HeroicInt > 0) m_scaledItem->HeroicInt += milestone_heroic;
		if (m_item->AWis > 0 || m_scaledItem->HeroicWis > 0) m_scaledItem->HeroicWis += milestone_heroic;
		if (m_item->ACha > 0 || m_scaledItem->HeroicCha > 0) m_scaledItem->HeroicCha += milestone_heroic;
	}

	// HP Regeneration (unlocks at level 50)
	if (level >= 50) {
		m_scaledItem->Regen = (level - 50) / 5;  // +1 per 5 levels
	}

	// Combat stats (slow scaling ~10 at level 100, cap at 127)
	if (level >= 10) {
		int combat_bonus = level / 10;  // +1 per 10 levels
		if (m_item->Shielding > 0) m_scaledItem->Shielding = std::min(m_scaledItem->Shielding + combat_bonus, 127);
		if (m_item->StrikeThrough > 0) m_scaledItem->StrikeThrough = std::min(m_scaledItem->StrikeThrough + combat_bonus, 127);
		if (m_item->StunResist > 0) m_scaledItem->StunResist = std::min(m_scaledItem->StunResist + combat_bonus, 127);
		if (m_item->SpellShield > 0) m_scaledItem->SpellShield = std::min(m_scaledItem->SpellShield + combat_bonus, 127);
		if (m_item->Avoidance > 0) m_scaledItem->Avoidance = std::min(m_scaledItem->Avoidance + combat_bonus, 127);
		if (m_item->Accuracy > 0) m_scaledItem->Accuracy = std::min(m_scaledItem->Accuracy + combat_bonus, 127);
		if (m_item->CombatEffects > 0) m_scaledItem->CombatEffects = std::min(m_scaledItem->CombatEffects + combat_bonus, 127);
	}
}

void EQ::ItemInstance::ApplyCustomStats() {
	if (!m_item) return;

	// If disabled, always revert to base item data and skip any dynamic scaling/custom overrides.
	if (!RuleB(Items, EnableCustomItemStats)) {
		if (m_scaledItem) {
			delete m_scaledItem;
			m_scaledItem = nullptr;
		}
		return;
	}

	// Helper to write to logs/inf/item_scaling.log
	auto LogInf = [this](const std::string& msg) {
		std::ofstream logfile("logs/inf/item_scaling.log", std::ios::app);
		if (logfile.is_open()) {
			auto now = std::chrono::system_clock::now();
			auto time = std::chrono::system_clock::to_time_t(now);
			char timebuf[32];
			std::strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", std::localtime(&time));
			logfile << "[" << timebuf << "] " << msg << std::endl;
			logfile.close();
		}
	};

	// Check for dynamic_level in custom data
	bool has_dynamic_level = false;
	int dynamic_level = 0;
	auto it = m_custom_data.find("dynamic_level");
	if (it != m_custom_data.end()) {
		try {
			dynamic_level = std::stoi(it->second);
			has_dynamic_level = (dynamic_level > 0);
			LogInf("ApplyCustomStats: Found dynamic_level=" + std::to_string(dynamic_level) + " for item " + std::to_string(m_item->ID) + " (" + m_item->Name + ")");
		} catch (...) {
			has_dynamic_level = false;
		}
	}

	// If we have no custom data and no scaling and no dynamic level, we don't need m_scaledItem
	if (m_custom_data.empty() && !m_scaling && !has_dynamic_level) {
		if (m_scaledItem) {
			delete m_scaledItem;
			m_scaledItem = nullptr;
		}
		return;
	}

	// Ensure m_scaledItem exists
	if (!m_scaledItem) {
		m_scaledItem = new ItemData(*m_item);
	}

	// Apply dynamic item scaling FIRST (if applicable)
	if (has_dynamic_level) {
		LogInf("ApplyCustomStats: Calling ScaleDynamicItem(" + std::to_string(dynamic_level) + ") for item " + std::to_string(m_item->ID));
		ScaleDynamicItem(dynamic_level);
		LogInf("ApplyCustomStats: After scaling - Damage=" + std::to_string(m_scaledItem->Damage) + ", HP=" + std::to_string(m_scaledItem->HP) + ", AC=" + std::to_string(m_scaledItem->AC));
	}
	else if (m_scaling) {
		ScaleItem(); // This resets m_scaledItem from m_item and applies scaling
	}
	else {
		// Reset to base if not scaling (so we don't accumulate custom stats)
		memcpy(m_scaledItem, m_item, sizeof(ItemData));
	}

	// Now apply custom stat modifiers (but skip dynamic_level as it's already processed)
	for (auto const& [key, val] : m_custom_data) {
		if (key == "dynamic_level") continue;  // Skip - already handled above

		try {
			int iVal = std::stoi(val);
			if (key == "STR") { int32 v = (int32)m_scaledItem->AStr + iVal; m_scaledItem->AStr = (int8)(v > 127 ? 127 : (v < -128 ? -128 : v)); }
			else if (key == "STA") { int32 v = (int32)m_scaledItem->ASta + iVal; m_scaledItem->ASta = (int8)(v > 127 ? 127 : (v < -128 ? -128 : v)); }
			else if (key == "DEX") { int32 v = (int32)m_scaledItem->ADex + iVal; m_scaledItem->ADex = (int8)(v > 127 ? 127 : (v < -128 ? -128 : v)); }
			else if (key == "AGI") { int32 v = (int32)m_scaledItem->AAgi + iVal; m_scaledItem->AAgi = (int8)(v > 127 ? 127 : (v < -128 ? -128 : v)); }
			else if (key == "INT") { int32 v = (int32)m_scaledItem->AInt + iVal; m_scaledItem->AInt = (int8)(v > 127 ? 127 : (v < -128 ? -128 : v)); }
			else if (key == "WIS") { int32 v = (int32)m_scaledItem->AWis + iVal; m_scaledItem->AWis = (int8)(v > 127 ? 127 : (v < -128 ? -128 : v)); }
			else if (key == "CHA") { int32 v = (int32)m_scaledItem->ACha + iVal; m_scaledItem->ACha = (int8)(v > 127 ? 127 : (v < -128 ? -128 : v)); }
			else if (key == "HP") m_scaledItem->HP += iVal;
			else if (key == "MANA") m_scaledItem->Mana += iVal;
			else if (key == "AC") m_scaledItem->AC += iVal;

			// Heroic Stats
			else if (key == "HEROIC_STR") m_scaledItem->HeroicStr += iVal;
			else if (key == "HEROIC_STA") m_scaledItem->HeroicSta += iVal;
			else if (key == "HEROIC_DEX") m_scaledItem->HeroicDex += iVal;
			else if (key == "HEROIC_AGI") m_scaledItem->HeroicAgi += iVal;
			else if (key == "HEROIC_INT") m_scaledItem->HeroicInt += iVal;
			else if (key == "HEROIC_WIS") m_scaledItem->HeroicWis += iVal;
			else if (key == "HEROIC_CHA") m_scaledItem->HeroicCha += iVal;
			else if (key == "HEROIC_MR") m_scaledItem->HeroicMR += iVal;
			else if (key == "HEROIC_FR") m_scaledItem->HeroicFR += iVal;
			else if (key == "HEROIC_CR") m_scaledItem->HeroicCR += iVal;
			else if (key == "HEROIC_DR") m_scaledItem->HeroicDR += iVal;
			else if (key == "HEROIC_PR") m_scaledItem->HeroicPR += iVal;
			else if (key == "HEROIC_SV_CORRUP") m_scaledItem->HeroicSVCorrup += iVal;

			// Base resistances (allow setting base resists via custom data keys)
			else if (key == "MR") { int32 v = (int32)m_scaledItem->MR + iVal; m_scaledItem->MR = (int8)(v > 127 ? 127 : (v < -128 ? -128 : v)); }
			else if (key == "FR") { int32 v = (int32)m_scaledItem->FR + iVal; m_scaledItem->FR = (int8)(v > 127 ? 127 : (v < -128 ? -128 : v)); }
			else if (key == "CR") { int32 v = (int32)m_scaledItem->CR + iVal; m_scaledItem->CR = (int8)(v > 127 ? 127 : (v < -128 ? -128 : v)); }
			else if (key == "DR") { int32 v = (int32)m_scaledItem->DR + iVal; m_scaledItem->DR = (int8)(v > 127 ? 127 : (v < -128 ? -128 : v)); }
			else if (key == "PR") { int32 v = (int32)m_scaledItem->PR + iVal; m_scaledItem->PR = (int8)(v > 127 ? 127 : (v < -128 ? -128 : v)); }
			else if (key == "SV_CORRUP") m_scaledItem->SVCorruption += iVal;

			// Mod2 / Other Stats
			else if (key == "ATTACK") m_scaledItem->Attack += iVal;
			else if (key == "HASTE") m_scaledItem->Haste += iVal;
			else if (key == "HP_REGEN") m_scaledItem->Regen += iVal;
			else if (key == "MANA_REGEN") m_scaledItem->ManaRegen += iVal;
			else if (key == "END_REGEN") m_scaledItem->EnduranceRegen += iVal;
			else if (key == "DAMAGE_SHIELD") m_scaledItem->DamageShield += iVal;
			else if (key == "DS_MITIGATION") m_scaledItem->DSMitigation += iVal;
			else if (key == "SPELL_SHIELD") { int32 v = (int32)m_scaledItem->SpellShield + iVal; m_scaledItem->SpellShield = (int8)(v > 127 ? 127 : (v < -128 ? -128 : v)); }
			else if (key == "SHIELDING") { int32 v = (int32)m_scaledItem->Shielding + iVal; m_scaledItem->Shielding = (int8)(v > 127 ? 127 : (v < -128 ? -128 : v)); }
			else if (key == "AVOIDANCE") { int32 v = (int32)m_scaledItem->Avoidance + iVal; m_scaledItem->Avoidance = (int8)(v > 127 ? 127 : (v < -128 ? -128 : v)); }
			else if (key == "ACCURACY") { int32 v = (int32)m_scaledItem->Accuracy + iVal; m_scaledItem->Accuracy = (int8)(v > 127 ? 127 : (v < -128 ? -128 : v)); }
			else if (key == "STUN_RESIST") { int32 v = (int32)m_scaledItem->StunResist + iVal; m_scaledItem->StunResist = (int8)(v > 127 ? 127 : (v < -128 ? -128 : v)); }
			else if (key == "STRIKETHROUGH") { int32 v = (int32)m_scaledItem->StrikeThrough + iVal; m_scaledItem->StrikeThrough = (int8)(v > 127 ? 127 : (v < -128 ? -128 : v)); }
			else if (key == "HEAL_AMT") m_scaledItem->HealAmt += iVal;
			else if (key == "SPELL_DMG") m_scaledItem->SpellDmg += iVal;
			else if (key == "CLAIRVOYANCE") m_scaledItem->Clairvoyance += iVal;
		}
		catch (...) {}
	}
}

void EQ::ItemInstance::SetTimer(std::string name, uint32 time) {
	Timer t(time);
	t.Start(time, false);
	m_timers[name] = t;
}

void EQ::ItemInstance::StopTimer(std::string name) {
	auto iter = m_timers.find(name);
	if(iter != m_timers.end()) {
		m_timers.erase(iter);
	}
}

void EQ::ItemInstance::ClearTimers() {
	m_timers.clear();
}

int EQ::ItemInstance::GetItemArmorClass(bool augments) const
{
	int ac = 0;
	const auto item = GetItem();
	if (item) {
		ac = item->AC;
		if (augments)
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i)
				if (GetAugment(i))
					ac += GetAugment(i)->GetItemArmorClass();
	}
	return ac;
}

int EQ::ItemInstance::GetItemElementalDamage(int &magic, int &fire, int &cold, int &poison, int &disease, int &chromatic, int &prismatic, int &physical, int &corruption, bool augments) const
{
	const auto item = GetItem();
	if (item) {
		switch (item->ElemDmgType) {
		case RESIST_MAGIC:
			magic += item->ElemDmgAmt;
			break;
		case RESIST_FIRE:
			fire += item->ElemDmgAmt;
			break;
		case RESIST_COLD:
			cold += item->ElemDmgAmt;
			break;
		case RESIST_POISON:
			poison += item->ElemDmgAmt;
			break;
		case RESIST_DISEASE:
			disease += item->ElemDmgAmt;
			break;
		case RESIST_CHROMATIC:
			chromatic += item->ElemDmgAmt;
			break;
		case RESIST_PRISMATIC:
			prismatic += item->ElemDmgAmt;
			break;
		case RESIST_PHYSICAL:
			physical += item->ElemDmgAmt;
			break;
		case RESIST_CORRUPTION:
			corruption += item->ElemDmgAmt;
			break;
		}

		if (augments)
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i)
				if (GetAugment(i))
					GetAugment(i)->GetItemElementalDamage(magic, fire, cold, poison, disease, chromatic, prismatic, physical, corruption);
	}
	return magic + fire + cold + poison + disease + chromatic + prismatic + physical + corruption;
}

int EQ::ItemInstance::GetItemElementalFlag(bool augments) const
{
	int flag = 0;
	const auto item = GetItem();
	if (item) {
		flag = item->ElemDmgType;
		if (flag)
			return flag;

		if (augments) {
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i) {
				if (GetAugment(i))
					flag = GetAugment(i)->GetItemElementalFlag();
				if (flag)
					return flag;
			}
		}
	}
	return flag;
}

int EQ::ItemInstance::GetItemElementalDamage(bool augments) const
{
	int64 damage = 0;
	const auto item = GetItem();
	if (item) {
		damage = item->ElemDmgAmt;
		if (damage)
			return damage;

		if (augments) {
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i) {
				if (GetAugment(i))
					damage = GetAugment(i)->GetItemElementalDamage();
				if (damage)
					return damage;
			}
		}
	}
	return damage;
}

int EQ::ItemInstance::GetItemRecommendedLevel(bool augments) const
{
	int level = 0;
	const auto item = GetItem();
	if (item) {
		level = item->RecLevel;

		if (augments) {
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i) {
				int temp = 0;
				if (GetAugment(i)) {
					temp = GetAugment(i)->GetItemRecommendedLevel();
					if (temp > level)
						level = temp;
				}
			}
		}
	}

	return level;
}

int EQ::ItemInstance::GetItemRequiredLevel(bool augments) const
{
	int level = 0;
	const auto item = GetItem();
	if (item) {
		level = item->ReqLevel;

		if (augments) {
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i) {
				int temp = 0;
				if (GetAugment(i)) {
					temp = GetAugment(i)->GetItemRequiredLevel();
					if (temp > level)
						level = temp;
				}
			}
		}
	}

	return level;
}

int EQ::ItemInstance::GetItemWeaponDamage(bool augments) const
{
	int64 damage = 0;
	const auto item = GetItem();
	if (item) {
		damage = item->Damage;

		if (augments) {
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i)
				if (GetAugment(i))
					damage += GetAugment(i)->GetItemWeaponDamage();
		}
	}
	return damage;
}

int EQ::ItemInstance::GetItemBackstabDamage(bool augments) const
{
	int64 damage = 0;
	const auto item = GetItem();
	if (item) {
		damage = item->BackstabDmg;

		if (augments) {
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i)
				if (GetAugment(i))
					damage += GetAugment(i)->GetItemBackstabDamage();
		}
	}
	return damage;
}

int EQ::ItemInstance::GetItemBaneDamageBody(bool augments) const
{
	int body = 0;
	const auto item = GetItem();
	if (item) {
		body = item->BaneDmgBody;
		if (body)
			return body;

		if (augments) {
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i)
				if (GetAugment(i)) {
					body = GetAugment(i)->GetItemBaneDamageBody();
					if (body)
						return body;
				}
		}
	}
	return body;
}

int EQ::ItemInstance::GetItemBaneDamageRace(bool augments) const
{
	int race = Race::Doug;
	const auto item = GetItem();
	if (item) {
		race = item->BaneDmgRace;
		if (race)
			return race;

		if (augments) {
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i)
				if (GetAugment(i)) {
					race = GetAugment(i)->GetItemBaneDamageRace();
					if (race)
						return race;
				}
		}
	}
	return race;
}

int EQ::ItemInstance::GetItemBaneDamageBody(uint8 against, bool augments) const
{
	int64 damage = 0;
	const auto item = GetItem();
	if (item) {
		if (item->BaneDmgBody == against)
			damage += item->BaneDmgAmt;

		if (augments) {
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i)
				if (GetAugment(i))
					damage += GetAugment(i)->GetItemBaneDamageBody(against);
		}
	}
	return damage;
}

int EQ::ItemInstance::GetItemBaneDamageRace(uint16 against, bool augments) const
{
	int64 damage = 0;
	const auto item = GetItem();
	if (item) {
		if (item->BaneDmgRace == against)
			damage += item->BaneDmgRaceAmt;

		if (augments) {
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i)
				if (GetAugment(i))
					damage += GetAugment(i)->GetItemBaneDamageRace(against);
		}
	}
	return damage;
}

int EQ::ItemInstance::GetItemMagical(bool augments) const
{
	const auto item = GetItem();
	if (item) {
		if (item->Magic)
			return 1;

		if (augments) {
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i)
				if (GetAugment(i) && GetAugment(i)->GetItemMagical())
					return 1;
		}
	}
	return 0;
}

int EQ::ItemInstance::GetItemHP(bool augments) const
{
	int hp = 0;
	const auto item = GetItem();
	if (item) {
		hp = item->HP;
		if (augments)
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i)
				if (GetAugment(i))
					hp += GetAugment(i)->GetItemHP();
	}
	return hp;
}

int EQ::ItemInstance::GetItemMana(bool augments) const
{
	int mana = 0;
	const auto item = GetItem();
	if (item) {
		mana = item->Mana;
		if (augments)
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i)
				if (GetAugment(i))
					mana += GetAugment(i)->GetItemMana();
	}
	return mana;
}

int EQ::ItemInstance::GetItemEndur(bool augments) const
{
	int endur = 0;
	const auto item = GetItem();
	if (item) {
		endur = item->Endur;
		if (augments)
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i)
				if (GetAugment(i))
					endur += GetAugment(i)->GetItemEndur();
	}
	return endur;
}

int EQ::ItemInstance::GetItemAttack(bool augments) const
{
	int atk = 0;
	const auto item = GetItem();
	if (item) {
		atk = item->Attack;
		if (augments)
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i)
				if (GetAugment(i))
					atk += GetAugment(i)->GetItemAttack();
	}
	return atk;
}

int EQ::ItemInstance::GetItemStr(bool augments) const
{
	int str = 0;
	const auto item = GetItem();
	if (item) {
		str = item->AStr;
		if (augments)
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i)
				if (GetAugment(i))
					str += GetAugment(i)->GetItemStr();
	}
	return str;
}

int EQ::ItemInstance::GetItemSta(bool augments) const
{
	int sta = 0;
	const auto item = GetItem();
	if (item) {
		sta = item->ASta;
		if (augments)
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i)
				if (GetAugment(i))
					sta += GetAugment(i)->GetItemSta();
	}
	return sta;
}

int EQ::ItemInstance::GetItemDex(bool augments) const
{
	int total = 0;
	const auto item = GetItem();
	if (item) {
		total = item->ADex;
		if (augments)
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i)
				if (GetAugment(i))
					total += GetAugment(i)->GetItemDex();
	}
	return total;
}

int EQ::ItemInstance::GetItemAgi(bool augments) const
{
	int total = 0;
	const auto item = GetItem();
	if (item) {
		total = item->AAgi;
		if (augments)
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i)
				if (GetAugment(i))
					total += GetAugment(i)->GetItemAgi();
	}
	return total;
}

int EQ::ItemInstance::GetItemInt(bool augments) const
{
	int total = 0;
	const auto item = GetItem();
	if (item) {
		total = item->AInt;
		if (augments)
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i)
				if (GetAugment(i))
					total += GetAugment(i)->GetItemInt();
	}
	return total;
}

int EQ::ItemInstance::GetItemWis(bool augments) const
{
	int total = 0;
	const auto item = GetItem();
	if (item) {
		total = item->AWis;
		if (augments)
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i)
				if (GetAugment(i))
					total += GetAugment(i)->GetItemWis();
	}
	return total;
}

int EQ::ItemInstance::GetItemCha(bool augments) const
{
	int total = 0;
	const auto item = GetItem();
	if (item) {
		total = item->ACha;
		if (augments)
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i)
				if (GetAugment(i))
					total += GetAugment(i)->GetItemCha();
	}
	return total;
}

int EQ::ItemInstance::GetItemMR(bool augments) const
{
	int total = 0;
	const auto item = GetItem();
	if (item) {
		total = item->MR;
		if (augments)
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i)
				if (GetAugment(i))
					total += GetAugment(i)->GetItemMR();
	}
	return total;
}

int EQ::ItemInstance::GetItemFR(bool augments) const
{
	int total = 0;
	const auto item = GetItem();
	if (item) {
		total = item->FR;
		if (augments)
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i)
				if (GetAugment(i))
					total += GetAugment(i)->GetItemFR();
	}
	return total;
}

int EQ::ItemInstance::GetItemCR(bool augments) const
{
	int total = 0;
	const auto item = GetItem();
	if (item) {
		total = item->CR;
		if (augments)
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i)
				if (GetAugment(i))
					total += GetAugment(i)->GetItemCR();
	}
	return total;
}

int EQ::ItemInstance::GetItemPR(bool augments) const
{
	int total = 0;
	const auto item = GetItem();
	if (item) {
		total = item->PR;
		if (augments)
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i)
				if (GetAugment(i))
					total += GetAugment(i)->GetItemPR();
	}
	return total;
}

int EQ::ItemInstance::GetItemDR(bool augments) const
{
	int total = 0;
	const auto item = GetItem();
	if (item) {
		total = item->DR;
		if (augments)
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i)
				if (GetAugment(i))
					total += GetAugment(i)->GetItemDR();
	}
	return total;
}

int EQ::ItemInstance::GetItemCorrup(bool augments) const
{
	int total = 0;
	const auto item = GetItem();
	if (item) {
		total = item->SVCorruption;
		if (augments)
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i)
				if (GetAugment(i))
					total += GetAugment(i)->GetItemCorrup();
	}
	return total;
}

int EQ::ItemInstance::GetItemHeroicStr(bool augments) const
{
	int total = 0;
	const auto item = GetItem();
	if (item) {
		total = item->HeroicStr;
		if (augments)
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i)
				if (GetAugment(i))
					total += GetAugment(i)->GetItemHeroicStr();
	}
	return total;
}

int EQ::ItemInstance::GetItemHeroicSta(bool augments) const
{
	int total = 0;
	const auto item = GetItem();
	if (item) {
		total = item->HeroicSta;
		if (augments)
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i)
				if (GetAugment(i))
					total += GetAugment(i)->GetItemHeroicSta();
	}
	return total;
}

int EQ::ItemInstance::GetItemHeroicDex(bool augments) const
{
	int total = 0;
	const auto item = GetItem();
	if (item) {
		total = item->HeroicDex;
		if (augments)
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i)
				if (GetAugment(i))
					total += GetAugment(i)->GetItemHeroicDex();
	}
	return total;
}

int EQ::ItemInstance::GetItemHeroicAgi(bool augments) const
{
	int total = 0;
	const auto item = GetItem();
	if (item) {
		total = item->HeroicAgi;
		if (augments)
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i)
				if (GetAugment(i))
					total += GetAugment(i)->GetItemHeroicAgi();
	}
	return total;
}

int EQ::ItemInstance::GetItemHeroicInt(bool augments) const
{
	int total = 0;
	const auto item = GetItem();
	if (item) {
		total = item->HeroicInt;
		if (augments)
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i)
				if (GetAugment(i))
					total += GetAugment(i)->GetItemHeroicInt();
	}
	return total;
}

int EQ::ItemInstance::GetItemHeroicWis(bool augments) const
{
	int total = 0;
	const auto item = GetItem();
	if (item) {
		total = item->HeroicWis;
		if (augments)
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i)
				if (GetAugment(i))
					total += GetAugment(i)->GetItemHeroicWis();
	}
	return total;
}

int EQ::ItemInstance::GetItemHeroicCha(bool augments) const
{
	int total = 0;
	const auto item = GetItem();
	if (item) {
		total = item->HeroicCha;
		if (augments)
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i)
				if (GetAugment(i))
					total += GetAugment(i)->GetItemHeroicCha();
	}
	return total;
}

int EQ::ItemInstance::GetItemHeroicMR(bool augments) const
{
	int total = 0;
	const auto item = GetItem();
	if (item) {
		total = item->HeroicMR;
		if (augments)
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i)
				if (GetAugment(i))
					total += GetAugment(i)->GetItemHeroicMR();
	}
	return total;
}

int EQ::ItemInstance::GetItemHeroicFR(bool augments) const
{
	int total = 0;
	const auto item = GetItem();
	if (item) {
		total = item->HeroicFR;
		if (augments)
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i)
				if (GetAugment(i))
					total += GetAugment(i)->GetItemHeroicFR();
	}
	return total;
}

int EQ::ItemInstance::GetItemHeroicCR(bool augments) const
{
	int total = 0;
	const auto item = GetItem();
	if (item) {
		total = item->HeroicCR;
		if (augments)
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i)
				if (GetAugment(i))
					total += GetAugment(i)->GetItemHeroicCR();
	}
	return total;
}

int EQ::ItemInstance::GetItemHeroicPR(bool augments) const
{
	int total = 0;
	const auto item = GetItem();
	if (item) {
		total = item->HeroicPR;
		if (augments)
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i)
				if (GetAugment(i))
					total += GetAugment(i)->GetItemHeroicPR();
	}
	return total;
}

int EQ::ItemInstance::GetItemHeroicDR(bool augments) const
{
	int total = 0;
	const auto item = GetItem();
	if (item) {
		total = item->HeroicDR;
		if (augments)
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i)
				if (GetAugment(i))
					total += GetAugment(i)->GetItemHeroicDR();
	}
	return total;
}

int EQ::ItemInstance::GetItemHeroicCorrup(bool augments) const
{
	int total = 0;
	const auto item = GetItem();
	if (item) {
		total = item->HeroicSVCorrup;
		if (augments)
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i)
				if (GetAugment(i))
					total += GetAugment(i)->GetItemHeroicCorrup();
	}
	return total;
}

int EQ::ItemInstance::GetItemHaste(bool augments) const
{
	int total = 0;
	const auto item = GetItem();
	if (item) {
		total = item->Haste;
		if (augments)
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i)
				if (GetAugment(i)) {
					int temp = GetAugment(i)->GetItemHaste();
					if (temp > total)
						total = temp;
				}
	}
	return total;
}

int EQ::ItemInstance::RemoveTaskDeliveredItems()
{
	int count = IsStackable() ? GetCharges() : 1;
	count -= GetTaskDeliveredCount();
	if (IsStackable())
	{
		SetCharges(count);
	}
	SetTaskDeliveredCount(0);
	return count;
}

uint32 EQ::ItemInstance::GetItemGuildFavor() const
{
	uint32 total = 0;
	const auto item = GetItem();
	if (item) {
		return total = item->GuildFavor;
	}
	return 0;
}

std::vector<uint32> EQ::ItemInstance::GetAugmentIDs() const
{
	std::vector<uint32> augments;

	for (uint8 slot_id = invaug::SOCKET_BEGIN; slot_id <= invaug::SOCKET_END; slot_id++) {
		augments.push_back(GetAugment(slot_id) ? GetAugmentItemID(slot_id) : 0);
	}

	return augments;
}

std::vector<std::string> EQ::ItemInstance::GetAugmentNames() const
{
	std::vector<std::string> augment_names;

	for (uint8 slot_id = invaug::SOCKET_BEGIN; slot_id <= invaug::SOCKET_END; slot_id++) {
		const auto augment = GetAugment(slot_id);
		augment_names.push_back(augment ? augment->GetItem()->Name : "");
	}

	return augment_names;
}

int EQ::ItemInstance::GetItemRegen(bool augments) const
{
	int        stat = 0;
	const auto item = GetItem();
	if (item) {
		stat = item->Regen;
		if (augments) {
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i) {
				if (GetAugment(i)) {
					stat += GetAugment(i)->GetItemRegen();
				}
			}
		}
	}
	return stat;
}

int EQ::ItemInstance::GetItemManaRegen(bool augments) const
{
	int        stat = 0;
	const auto item = GetItem();
	if (item) {
		stat = item->ManaRegen;
		if (augments) {
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i) {
				if (GetAugment(i)) {
					stat += GetAugment(i)->GetItemManaRegen();
				}
			}
		}
	}
	return stat;
}

int EQ::ItemInstance::GetItemDamageShield(bool augments) const
{
	int        stat = 0;
	const auto item = GetItem();
	if (item) {
		stat = item->DamageShield;
		if (augments) {
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i) {
				if (GetAugment(i)) {
					stat += GetAugment(i)->GetItemDamageShield();
				}
			}
		}
	}
	return stat;
}

int EQ::ItemInstance::GetItemDSMitigation(bool augments) const
{
	int        stat = 0;
	const auto item = GetItem();
	if (item) {
		stat = item->DSMitigation;
		if (augments) {
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i) {
				if (GetAugment(i)) {
					stat += GetAugment(i)->GetItemDSMitigation();
				}
			}
		}
	}
	return stat;
}

int EQ::ItemInstance::GetItemHealAmt(bool augments) const
{
	int        stat = 0;
	const auto item = GetItem();
	if (item) {
		stat = item->HealAmt;
		if (augments) {
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i) {
				if (GetAugment(i)) {
					stat += GetAugment(i)->GetItemHealAmt();
				}
			}
		}
	}
	return stat;
}

int EQ::ItemInstance::GetItemSpellDamage(bool augments) const
{
	int        stat = 0;
	const auto item = GetItem();
	if (item) {
		stat = item->SpellDmg;
		if (augments) {
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i) {
				if (GetAugment(i)) {
					stat += GetAugment(i)->GetItemSpellDamage();
				}
			}
		}
	}
	return stat;
}

int EQ::ItemInstance::GetItemClairvoyance(bool augments) const
{
	int        stat = 0;
	const auto item = GetItem();
	if (item) {
		stat = item->Clairvoyance;
		if (augments) {
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i) {
				if (GetAugment(i)) {
					stat += GetAugment(i)->GetItemClairvoyance();
				}
			}
		}
	}
	return stat;
}

int EQ::ItemInstance::GetItemSkillsStat(EQ::skills::SkillType skill, bool augments) const
{
	int        stat = 0;
	const auto item = GetItem();
	if (item) {
		stat = item->ExtraDmgSkill == skill ? item->ExtraDmgAmt : 0;
		if (augments) {
			for (int i = invaug::SOCKET_BEGIN; i <= invaug::SOCKET_END; ++i) {
				if (GetAugment(i)) {
					stat += GetAugment(i)->GetItemSkillsStat(skill);
				}
			}
		}
	}
	return stat;
}

void EQ::ItemInstance::AddGUIDToMap(uint64 existing_serial_number)
{
	guids.emplace(existing_serial_number);
}

void EQ::ItemInstance::ClearGUIDMap()
{
	guids.clear();
}

bool EQ::ItemInstance::TransferOwnership(Database &db, const uint32 to_char_id) const
{
	if (!to_char_id || !IsEvolving()) {
		return false;
	}

	SetEvolveCharID(to_char_id);
	CharacterEvolvingItemsRepository::UpdateCharID(db, GetEvolveUniqueID(), to_char_id);
	return true;
}

uint32 EQ::ItemInstance::GetAugmentEvolveUniqueID(uint8 augment_index) const
{
	if (!m_item || !m_item->IsClassCommon()) {
		return 0;
	}

	const auto item = GetItem(augment_index);
	if (item) {
		return item->GetEvolveUniqueID();
	}

	return 0;
}

void EQ::ItemInstance::SetTimer(std::string name, uint32 time) const{
	Timer t(time);
	t.Start(time, false);
	m_timers[name] = t;
}

void EQ::ItemInstance::SetEvolveEquipped(const bool in) const
{
	if (!IsEvolving()) {
		return;
	}

	m_evolving_details.equipped = in;
	if (in && !GetTimers().contains("evolve")) {
		SetTimer("evolve", RuleI(EvolvingItems, DelayUponEquipping));
		return;
	}

	if (in) {
		GetTimers().at("evolve").SetTimer(RuleI(EvolvingItems, DelayUponEquipping));
		return;
	}

	GetTimers().at("evolve").Disable();
}
