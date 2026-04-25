#include "database_update.h"

std::vector<ManifestEntry> manifest_entries_custom = {
	ManifestEntry{
		.version = 1,
		.description = "2025_05_16_new_database_check_test",
		.check = "SHOW TABLES LIKE 'new_table'",
		.condition = "empty",
		.match = "",
		.sql = R"(
CREATE TABLE `new_table`  (
  `id` int NOT NULL AUTO_INCREMENT,
  PRIMARY KEY (`id`)
);
)",
		.content_schema_update = false,
	},
	ManifestEntry{
		.version = 2,
		.description = "2026_04_21_character_pet_command_states_table",
		.check = "SHOW TABLES LIKE 'character_pet_command_states'",
		.condition = "empty",
		.match = "",
		.sql = R"(
CREATE TABLE `character_pet_command_states` (
  `character_id` int(10) NOT NULL,
  `pet_class` tinyint(4) NOT NULL,
  `command_id` tinyint(4) NOT NULL,
  `command_state` tinyint(4) NOT NULL,
  PRIMARY KEY (`character_id`,`pet_class`,`command_id`),
  KEY `idx_char_petclass` (`character_id`,`pet_class`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
)",
		.content_schema_update = false,
	},
// Used for testing
//	ManifestEntry{
//		.version = 9229,
//		.description = "new_database_check_test",
//		.check = "SHOW TABLES LIKE 'new_table'",
//		.condition = "empty",
//		.match = "",
//		.sql = R"(
//CREATE TABLE `new_table`  (
//  `id` int NOT NULL AUTO_INCREMENT,
//  PRIMARY KEY (`id`)
//);
//CREATE TABLE `new_table1`  (
//  `id` int NOT NULL AUTO_INCREMENT,
//  PRIMARY KEY (`id`)
//);
//CREATE TABLE `new_table2`  (
//  `id` int NOT NULL AUTO_INCREMENT,
//  PRIMARY KEY (`id`)
//);
//CREATE TABLE `new_table3`  (
//  `id` int NOT NULL AUTO_INCREMENT,
//  PRIMARY KEY (`id`)
//);
//)",
//	}

};

// see struct definitions for what each field does
// struct ManifestEntry {
// 	int         version{};     // database version of the migration
// 	std::string description{}; // description of the migration ex: "add_new_table" or "add_index_to_table"
// 	std::string check{};       // query that checks against the condition
// 	std::string condition{};   // condition or "match_type" - Possible values [contains|match|missing|empty|not_empty]
// 	std::string match{};       // match field that is not always used, but works in conjunction with "condition" values [missing|match|contains]
// 	std::string sql{};         // the SQL DDL that gets ran when the condition is true
// };
