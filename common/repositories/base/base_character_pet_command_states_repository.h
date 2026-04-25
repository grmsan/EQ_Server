/**
 * DO NOT MODIFY THIS FILE
 *
 * This repository was automatically generated and is NOT to be modified directly.
 * Any repository modifications are meant to be made to the repository extending the base.
 * Any modifications to base repositories are to be made by the generator only
 *
 * @generator ./utils/scripts/generators/repository-generator.pl
 * @docs https://docs.eqemu.io/developer/repositories
 */

#ifndef EQEMU_BASE_CHARACTER_PET_COMMAND_STATES_REPOSITORY_H
#define EQEMU_BASE_CHARACTER_PET_COMMAND_STATES_REPOSITORY_H

#include "../../database.h"
#include "../../strings.h"
#include <ctime>

class BaseCharacterPetCommandStatesRepository {
public:
	struct CharacterPetCommandStates {
		int32_t character_id;
		int8_t  pet_class;
		int8_t  command_id;
		int8_t  command_state;
	};

	static std::string PrimaryKey()
	{
		return std::string("character_id");
	}

	static std::vector<std::string> Columns()
	{
		return {
			"character_id",
			"pet_class",
			"command_id",
			"command_state",
		};
	}

	static std::vector<std::string> SelectColumns()
	{
		return {
			"character_id",
			"pet_class",
			"command_id",
			"command_state",
		};
	}

	static std::string ColumnsRaw()
	{
		return std::string(Strings::Implode(", ", Columns()));
	}

	static std::string SelectColumnsRaw()
	{
		return std::string(Strings::Implode(", ", SelectColumns()));
	}

	static std::string TableName()
	{
		return std::string("character_pet_command_states");
	}

	static std::string BaseSelect()
	{
		return fmt::format(
			"SELECT {} FROM {}",
			SelectColumnsRaw(),
			TableName()
		);
	}

	static std::string BaseInsert()
	{
		return fmt::format(
			"INSERT INTO {} ({}) ",
			TableName(),
			ColumnsRaw()
		);
	}

	static CharacterPetCommandStates NewEntity()
	{
		CharacterPetCommandStates e{};

		e.character_id  = 0;
		e.pet_class     = 0;
		e.command_id    = 0;
		e.command_state = 0;

		return e;
	}

	static std::vector<CharacterPetCommandStates> GetWhere(Database& db, const std::string &where_filter)
	{
		std::vector<CharacterPetCommandStates> all_entries;

		auto results = db.QueryDatabase(
			fmt::format(
				"{} WHERE {}",
				BaseSelect(),
				where_filter
			)
		);

		all_entries.reserve(results.RowCount());

		for (auto row = results.begin(); row != results.end(); ++row) {
			CharacterPetCommandStates e{};

			e.character_id  = row[0] ? static_cast<int32_t>(atoi(row[0])) : 0;
			e.pet_class     = row[1] ? static_cast<int8_t>(atoi(row[1])) : 0;
			e.command_id    = row[2] ? static_cast<int8_t>(atoi(row[2])) : 0;
			e.command_state = row[3] ? static_cast<int8_t>(atoi(row[3])) : 0;

			all_entries.push_back(e);
		}

		return all_entries;
	}

	static std::string BaseReplace()
	{
		return fmt::format(
			"REPLACE INTO {} ({}) ",
			TableName(),
			ColumnsRaw()
		);
	}

	static int ReplaceOne(
		Database& db,
		const CharacterPetCommandStates &e
	)
	{
		std::vector<std::string> v;

		v.push_back(std::to_string(e.character_id));
		v.push_back(std::to_string(e.pet_class));
		v.push_back(std::to_string(e.command_id));
		v.push_back(std::to_string(e.command_state));

		auto results = db.QueryDatabase(
			fmt::format(
				"{} VALUES ({})",
				BaseReplace(),
				Strings::Implode(",", v)
			)
		);

		return (results.Success() ? results.RowsAffected() : 0);
	}
};

#endif //EQEMU_BASE_CHARACTER_PET_COMMAND_STATES_REPOSITORY_H