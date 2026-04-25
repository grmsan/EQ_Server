#ifndef EQEMU_CHARACTER_PET_COMMAND_STATES_REPOSITORY_H
#define EQEMU_CHARACTER_PET_COMMAND_STATES_REPOSITORY_H

#include "../database.h"
#include "../strings.h"
#include "../../zone/common.h"
#include "../classes.h"
#include "base/base_character_pet_command_states_repository.h"

class CharacterPetCommandStatesRepository: public BaseCharacterPetCommandStatesRepository {
public:
	struct PetCommandStates {
		bool assist = false;
		bool hold = false;
		bool ghold = false;
		bool focus = false;
		bool spellhold = false;
		bool taunt = false;
		bool has_assist = false;
		bool has_hold = false;
		bool has_ghold = false;
		bool has_focus = false;
		bool has_spellhold = false;
		bool has_taunt = false;

		PetCommandStates(int8_t pet_class = Class::None)
		{
			assist = (pet_class == Class::None);
		}
	};

	static PetCommandStates GetAllCommandStates(Database& db, int32_t character_id, int8_t pet_class)
	{
		PetCommandStates states(pet_class);

		auto results = GetWhere(db, fmt::format(
			"character_id = {} AND pet_class = {}",
			character_id,
			pet_class
		));

		for (const auto& e : results) {
			switch (e.command_id) {
				case CUSTOM_PET_ASSIST:
					states.assist = (e.command_state == 1);
					states.has_assist = true;
					break;
				case PET_HOLD:
					states.hold = (e.command_state == 1);
					states.has_hold = true;
					break;
				case PET_GHOLD:
					states.ghold = (e.command_state == 1);
					states.has_ghold = true;
					break;
				case PET_FOCUS:
					states.focus = (e.command_state == 1);
					states.has_focus = true;
					break;
				case PET_SPELLHOLD:
					states.spellhold = (e.command_state == 1);
					states.has_spellhold = true;
					break;
				case PET_TAUNT:
					states.taunt = (e.command_state == 1);
					states.has_taunt = true;
					break;
			}
		}

		return states;
	}

	static void SetCommandState(Database& db, int32_t character_id, int8_t pet_class, int8_t command_id, int8_t state)
	{
		CharacterPetCommandStates e;
		e.character_id = character_id;
		e.pet_class = pet_class;
		e.command_id = command_id;
		e.command_state = state;

		ReplaceOne(db, e);
	}
};

#endif //EQEMU_CHARACTER_PET_COMMAND_STATES_REPOSITORY_H
