#include "../client.h"
#include "../../common/races.h"

#include <array>
#include <algorithm>

namespace {

constexpr uint32 kArmorTextureMin = 0;
constexpr uint32 kArmorTextureMax = 25;

const std::array<uint16, 16> kPlayableRaces = {
	Race::Human,
	Race::Barbarian,
	Race::Erudite,
	Race::WoodElf,
	Race::HighElf,
	Race::DarkElf,
	Race::HalfElf,
	Race::Dwarf,
	Race::Troll,
	Race::Ogre,
	Race::Halfling,
	Race::Gnome,
	Race::Iksar,
	Race::VahShir,
	Race::Froglok2,
	Race::Drakkin
};

const std::array<uint32, 56> kWeaponModels = {
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
	10, 11, 12, 14, 15, 16, 17, 18, 19, 20,
	21, 23, 24, 25, 27, 28, 30, 31, 32, 33,
	34, 35, 36, 39, 40, 41, 42, 44, 46, 47,
	52, 54, 60, 63, 64, 65, 67, 70, 74, 80,
	82, 84, 92, 100, 120, 140
};

constexpr const char* kPlayableRaceSqlList = "1,2,3,4,5,6,7,8,9,10,11,12,128,130,330,522";

struct MirrorAppearance {
	uint32 donor_npc_id = 0;
	std::string donor_name;
	uint16 race = Race::Human;
	uint8 gender = Gender::Male;
	uint8 texture = 0;
	uint8 helmtexture = 0;
	int armtexture = 0;
	int bracertexture = 0;
	int handtexture = 0;
	int legtexture = 0;
	int feettexture = 0;
	uint8 armortint_red = 0;
	uint8 armortint_green = 0;
	uint8 armortint_blue = 0;
	uint32 d_melee_texture1 = 0;
	uint32 d_melee_texture2 = 0;
	uint8 face = 0;
	uint8 luclin_hairstyle = 0;
	uint8 luclin_haircolor = 0;
	uint8 luclin_eyecolor = 0;
	uint8 luclin_eyecolor2 = 0;
	uint8 luclin_beard = 0;
	uint8 luclin_beardcolor = 0;
	uint32 drakkin_heritage = 0;
	uint32 drakkin_tattoo = 0;
	uint32 drakkin_details = 0;
};

uint8 NormalizeClassForMirror(uint8 class_id)
{
	// THJ-style bazaar guildmasters are class IDs 20..35 mapping to base classes 1..16.
	if (class_id >= 20 && class_id <= 35) {
		return static_cast<uint8>(class_id - 19);
	}

	return class_id;
}

int32 ParseSqlInt(const char* value)
{
	return value ? Strings::ToInt(value) : 0;
}

uint32 ParseSqlUInt(const char* value)
{
	return value ? Strings::ToUnsignedInt(value) : 0;
}

uint8 ClampToByte(int32 value)
{
	return static_cast<uint8>(std::clamp(value, 0, 255));
}

uint32 ClampToMaterial(int32 value)
{
	return static_cast<uint32>(std::clamp(value, 0, static_cast<int32>(kArmorTextureMax)));
}

bool ParseMirrorAppearanceRow(MySQLRequestRow row, MirrorAppearance& out)
{
	if (!row[0]) {
		return false;
	}

	out.donor_npc_id = ParseSqlUInt(row[0]);
	out.donor_name = row[1] ? row[1] : "";
	out.race = static_cast<uint16>(ParseSqlUInt(row[2]));
	out.gender = static_cast<uint8>(ParseSqlUInt(row[3]));
	out.texture = ClampToByte(ParseSqlInt(row[4]));
	out.helmtexture = ClampToByte(ParseSqlInt(row[5]));
	out.armtexture = ParseSqlInt(row[6]);
	out.bracertexture = ParseSqlInt(row[7]);
	out.handtexture = ParseSqlInt(row[8]);
	out.legtexture = ParseSqlInt(row[9]);
	out.feettexture = ParseSqlInt(row[10]);
	out.armortint_red = ClampToByte(ParseSqlInt(row[11]));
	out.armortint_green = ClampToByte(ParseSqlInt(row[12]));
	out.armortint_blue = ClampToByte(ParseSqlInt(row[13]));
	out.d_melee_texture1 = ParseSqlUInt(row[14]);
	out.d_melee_texture2 = ParseSqlUInt(row[15]);
	out.face = ClampToByte(ParseSqlInt(row[16]));
	out.luclin_hairstyle = ClampToByte(ParseSqlInt(row[17]));
	out.luclin_haircolor = ClampToByte(ParseSqlInt(row[18]));
	out.luclin_eyecolor = ClampToByte(ParseSqlInt(row[19]));
	out.luclin_eyecolor2 = ClampToByte(ParseSqlInt(row[20]));
	out.luclin_beard = ClampToByte(ParseSqlInt(row[21]));
	out.luclin_beardcolor = ClampToByte(ParseSqlInt(row[22]));
	out.drakkin_heritage = ParseSqlUInt(row[23]);
	out.drakkin_tattoo = ParseSqlUInt(row[24]);
	out.drakkin_details = ParseSqlUInt(row[25]);

	return out.donor_npc_id > 0;
}

bool LoadMirrorAppearanceByNpcTypeId(uint32 donor_npc_type_id, MirrorAppearance& out)
{
	auto results = content_db.QueryDatabase(
		fmt::format(
			"SELECT id,name,race,gender,texture,helmtexture,armtexture,bracertexture,handtexture,legtexture,feettexture,"
			"armortint_red,armortint_green,armortint_blue,d_melee_texture1,d_melee_texture2,face,luclin_hairstyle,luclin_haircolor,"
			"luclin_eyecolor,luclin_eyecolor2,luclin_beard,luclin_beardcolor,drakkin_heritage,drakkin_tattoo,drakkin_details "
			"FROM npc_types WHERE id = {} LIMIT 1",
			donor_npc_type_id
		)
	);

	if (!results.Success() || results.RowCount() == 0) {
		return false;
	}

	for (auto row : results) {
		return ParseMirrorAppearanceRow(row, out);
	}

	return false;
}

bool LoadMirrorAppearanceByClass(uint8 base_class, uint32 exclude_npc_type_id, MirrorAppearance& out)
{
	auto results = content_db.QueryDatabase(
		fmt::format(
			"SELECT id,name,race,gender,texture,helmtexture,armtexture,bracertexture,handtexture,legtexture,feettexture,"
			"armortint_red,armortint_green,armortint_blue,d_melee_texture1,d_melee_texture2,face,luclin_hairstyle,luclin_haircolor,"
			"luclin_eyecolor,luclin_eyecolor2,luclin_beard,luclin_beardcolor,drakkin_heritage,drakkin_tattoo,drakkin_details "
			"FROM npc_types "
			"WHERE class = {} AND id != {} AND race IN ({}) AND name NOT LIKE '#%%' AND name NOT LIKE 'a\\_%%' ESCAPE '\\\\' AND name NOT LIKE 'an\\_%%' ESCAPE '\\\\' "
			"ORDER BY RAND() LIMIT 1",
			base_class,
			exclude_npc_type_id,
			kPlayableRaceSqlList
		)
	);

	if (!results.Success() || results.RowCount() == 0) {
		return false;
	}

	for (auto row : results) {
		return ParseMirrorAppearanceRow(row, out);
	}

	return false;
}

void ApplyMirrorAppearance(Mob* target, const MirrorAppearance& donor)
{
	if (!target) {
		return;
	}

	target->SendIllusionPacket(
		AppearanceStruct{
			.beard = donor.luclin_beard,
			.beard_color = donor.luclin_beardcolor,
			.drakkin_details = donor.drakkin_details,
			.drakkin_heritage = donor.drakkin_heritage,
			.drakkin_tattoo = donor.drakkin_tattoo,
			.eye_color_one = donor.luclin_eyecolor,
			.eye_color_two = donor.luclin_eyecolor2,
			.face = donor.face,
			.gender_id = donor.gender,
			.hair = donor.luclin_hairstyle,
			.hair_color = donor.luclin_haircolor,
			.helmet_texture = donor.helmtexture,
			.race_id = donor.race,
			.size = target->GetSize(),
			.texture = donor.texture,
		}
	);

	uint32 tint_color = (donor.armortint_red << 16) | (donor.armortint_green << 8) | donor.armortint_blue;
	if (tint_color) {
		tint_color |= 0xFF000000;
	}

	target->WearChange(EQ::textures::armorHead, ClampToMaterial(donor.helmtexture), tint_color, 0);
	target->WearChange(EQ::textures::armorChest, ClampToMaterial(donor.texture), tint_color, 0);
	target->WearChange(EQ::textures::armorArms, ClampToMaterial(donor.armtexture), tint_color, 0);
	target->WearChange(EQ::textures::armorWrist, ClampToMaterial(donor.bracertexture), tint_color, 0);
	target->WearChange(EQ::textures::armorHands, ClampToMaterial(donor.handtexture), tint_color, 0);
	target->WearChange(EQ::textures::armorLegs, ClampToMaterial(donor.legtexture), tint_color, 0);
	target->WearChange(EQ::textures::armorFeet, ClampToMaterial(donor.feettexture), tint_color, 0);
	target->WearChange(EQ::textures::weaponPrimary, donor.d_melee_texture1, tint_color, 0);
	target->WearChange(EQ::textures::weaponSecondary, donor.d_melee_texture2, tint_color, 0);
}

uint32 RandomTintColor()
{
	const auto red = static_cast<uint8>(zone->random.Int(0, 255));
	const auto green = static_cast<uint8>(zone->random.Int(0, 255));
	const auto blue = static_cast<uint8>(zone->random.Int(0, 255));

	uint32 color = (red << 16) | (green << 8) | blue;
	if (color) {
		color |= 0xFF000000;
	}

	return color;
}

uint32 RandomWeaponModel()
{
	const auto index = static_cast<size_t>(zone->random.Int(0, static_cast<int>(kWeaponModels.size() - 1)));
	return kWeaponModels[index];
}

void RandomizeGearVisuals(Mob* target)
{
	if (!target) {
		return;
	}

	for (uint8 slot = EQ::textures::armorHead; slot <= EQ::textures::armorFeet; ++slot) {
		const auto material = zone->random.Int(kArmorTextureMin, kArmorTextureMax);
		target->WearChange(slot, material, RandomTintColor(), 0);
	}

	target->WearChange(EQ::textures::weaponPrimary, RandomWeaponModel(), RandomTintColor(), 0);
	target->WearChange(EQ::textures::weaponSecondary, RandomWeaponModel(), RandomTintColor(), 0);
}

Mob* GetRandomizeTarget(Client* c)
{
	if (!c) {
		return nullptr;
	}

	auto* target = c->GetTarget();
	if (target) {
		return target;
	}

	return c;
}

void RandomizeFullLook(Mob* target)
{
	if (!target) {
		return;
	}

	const auto race_index = static_cast<size_t>(zone->random.Int(0, static_cast<int>(kPlayableRaces.size() - 1)));
	const auto race = kPlayableRaces[race_index];
	const auto requested_gender = static_cast<uint8>(zone->random.Int(Gender::Male, Gender::Female));
	const auto gender = Mob::GetDefaultGender(race, requested_gender);
	const auto size = GetRaceGenderDefaultHeight(race, gender);

	target->SendIllusionPacket(
		AppearanceStruct{
			.gender_id = gender,
			.helmet_texture = static_cast<uint8>(zone->random.Int(kArmorTextureMin, kArmorTextureMax)),
			.race_id = race,
			.size = size,
			.texture = static_cast<uint8>(zone->random.Int(kArmorTextureMin, kArmorTextureMax)),
		}
	);

	target->RandomizeFeatures(true, true);
	RandomizeGearVisuals(target);
}

} // namespace

void command_randomizegear(Client* c, const Seperator* sep)
{
	auto* target = GetRandomizeTarget(c);
	if (!target) {
		c->Message(Chat::White, "You must have a target to use this command.");
		return;
	}

	RandomizeGearVisuals(target);

	c->Message(
		Chat::White,
		fmt::format(
			"Randomized gear materials and tints for {}.",
			c->GetTargetDescription(target, TargetDescriptionType::UCSelf)
		).c_str()
	);
}

void command_randomize(Client* c, const Seperator* sep)
{
	auto* target = GetRandomizeTarget(c);
	if (!target) {
		c->Message(Chat::White, "You must have a target to use this command.");
		return;
	}

	const auto mode = sep->arg[1] ? Strings::ToLower(sep->arg[1]) : "";
	if (mode == "gear") {
		command_randomizegear(c, sep);
		return;
	}

	if (!mode.empty() && mode != "look" && mode != "full" && mode != "mirror") {
		c->Message(Chat::White, "Usage: #randomize [mirror [npc_type_id]|look|gear]");
		return;
	}

	// Default to mirror mode for more coherent NPC results.
	const bool use_mirror_mode = mode.empty() || mode == "mirror";
	if (use_mirror_mode) {
		MirrorAppearance donor;
		bool loaded_donor = false;

		if (sep->IsNumber(2)) {
			const uint32 donor_npc_type_id = Strings::ToUnsignedInt(sep->arg[2]);
			loaded_donor = LoadMirrorAppearanceByNpcTypeId(donor_npc_type_id, donor);
		}
		else {
			const uint8 base_class = NormalizeClassForMirror(target->GetClass());
			uint32 target_npc_type_id = 0;
			if (target->IsNPC()) {
				target_npc_type_id = target->CastToNPC()->GetNPCTypeID();
			}

			loaded_donor = LoadMirrorAppearanceByClass(base_class, target_npc_type_id, donor);
		}

		if (!loaded_donor) {
			c->Message(Chat::White, "No suitable donor NPC appearance found for mirror mode.");
			return;
		}

		ApplyMirrorAppearance(target, donor);

		c->Message(
			Chat::White,
			fmt::format(
				"Mirrored appearance for {} from {} ({}) [{}].",
				c->GetTargetDescription(target, TargetDescriptionType::UCSelf),
				donor.donor_name,
				donor.donor_npc_id,
				GetRaceIDName(donor.race)
			).c_str()
		);
		return;
	}

	RandomizeFullLook(target);

	c->Message(
		Chat::White,
		fmt::format(
			"Randomized full look for {} (race/gender/features/materials/tints).",
			c->GetTargetDescription(target, TargetDescriptionType::UCSelf)
		).c_str()
	);
}
