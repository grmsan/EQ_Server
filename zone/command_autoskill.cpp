#include "client.h"
#include "object.h"
#include "command.h"
#include <charconv>
#include <map>
#include <string>
#include <tuple>
#include <vector>
#include <fmt/format.h>

/*
Skill ID	Skill Name
8	Backstab
10	Bash
16	Disarm
21	Dragon Punch
23	Eagle Strike
26	Flying Kick
30	Kick
38	Round Kick
52	Tiger Claw
74	Frenzy
*/

// Define the CommandMode enum at file scope
enum CommandMode { Status, Enable, Disable };

// Function declarations
std::tuple<std::map<std::string, int>, std::map<std::string, int>, std::map<int, std::string>> initialize_skill_maps(Client* c);
void display_skill_list(Client *c, const std::map<int, std::string>& id_to_name_map);
std::pair<CommandMode, bool> parse_command_mode(const std::string& arg);
std::string extract_skill_name(const Seperator *sep, bool has_command_param);
int find_skill_id(const std::string& skill_name_input,
                 const std::map<std::string, int>& skill_lookup_map,
                 const std::map<int, std::string>& id_to_name_map);
void process_command(Client *c, CommandMode cmd_mode, int skill_id,
                    const std::map<int, std::string>& id_to_name_map);

void command_autoskill(Client *c, const Seperator *sep)
{
    if (!c || !sep) {
        return;
    }

    std::string usage = "Usage: #autoskill [skill id or name] [enable/disable/status] or #autoskill list";

    // Check if we have at least one argument
    if (sep->argnum < 1) {
        c->Message(Chat::Skills, usage.c_str());
        return;
    }

    // Initialize our skill maps
    static auto skill_maps = initialize_skill_maps(c);
    const auto& skill_display_map = std::get<0>(skill_maps);
    const auto& skill_lookup_map = std::get<1>(skill_maps);
    const auto& id_to_name_map = std::get<2>(skill_maps);

    // Check if the command is "list"
    if (Strings::ToLower(sep->arg[1]) == "list") {
        display_skill_list(c, id_to_name_map);
        return;
    }

    // First determine if the last argument is a command parameter (enable/disable/status)
    CommandMode cmd_mode;
    bool has_command_param;
    std::tie(cmd_mode, has_command_param) = parse_command_mode(sep->arg[sep->argnum]);

    // Now determine the skill name or ID based on whether we have a command param
    if (has_command_param && sep->argnum < 2) {
        c->Message(Chat::Skills, "Autoskill configuration failed. Missing skill name or ID.");
        return;
    }

    // Extract skill name from arguments
    std::string skill_name_input = extract_skill_name(sep, has_command_param);

    // Find skill ID
    int skill_id = find_skill_id(skill_name_input, skill_lookup_map, id_to_name_map);

    // Check if we found a valid skill ID
    if (skill_id == -1) {
        c->Message(Chat::Skills, "Autoskill configuration failed. Invalid skill name or ID.");
        c->Message(Chat::Skills, usage.c_str());
        return;
    }

    if (c->HasSkill((EQ::skills::SkillType)skill_id)) {
        // Output based on the command mode
        process_command(c, cmd_mode, skill_id, id_to_name_map);
    } else {
        c->Message(Chat::Skills, "Autoskill configuration failed. You do not have that skill.");
        c->Message(Chat::Skills, usage.c_str());
    }
}

// Helper function implementations
std::tuple<std::map<std::string, int>, std::map<std::string, int>, std::map<int, std::string>> initialize_skill_maps(Client* c)
{
    // Define maps we'll populate
    static std::map<std::string, int> skill_display_map;
    static std::map<std::string, int> skill_lookup_map;
    static std::map<int, std::string> id_to_name_map;

    // Only initialize once
    if (skill_display_map.empty()) {
        // Get the available auto skills from the client
        auto available_skills = c->GetAvailableAutoSkills();

        // Populate the maps using the available skills
        for (const auto& skill_id : available_skills) {
            // Get the proper skill name using EQ::skills::GetSkillName
            std::string skill_name = EQ::skills::GetSkillName(skill_id);
            int skill_id_int = static_cast<int>(skill_id);

            // Add to display map
            skill_display_map[skill_name] = skill_id_int;

            // Add to lookup map (lowercase version)
            std::string skill_name_lower = Strings::ToLower(skill_name);
            skill_lookup_map[skill_name_lower] = skill_id_int;

            // Add to ID to Name map
            id_to_name_map[skill_id_int] = skill_name;
        }
    }

    return std::make_tuple(skill_display_map, skill_lookup_map, id_to_name_map);
}

void display_skill_list(Client *c, const std::map<int, std::string>& id_to_name_map)
{
    c->Message(Chat::Skills, "Available Autoskills:");
    for (const auto& skill : c->GetAutoSkillsList()) {
        int skill_id = static_cast<int>(skill);
        auto it = id_to_name_map.find(skill_id);
        if (it != id_to_name_map.end()) {
            bool is_enabled = c->GetAutoSkillStatus(skill);
            c->Message(Chat::Skills, "  %s (ID: %d) - %s",
                      it->second.c_str(), skill_id, is_enabled ? "ENABLED" : "disabled");
        }
    }
}

std::pair<CommandMode, bool> parse_command_mode(const std::string& arg)
{
    std::string arg_lower = Strings::ToLower(arg);
    if (arg_lower == "enable" || arg_lower == "on" || arg_lower == "1") {
        return {Enable, true};
    } else if (arg_lower == "disable" || arg_lower == "off" || arg_lower == "0") {
        return {Disable, true};
    } else if (arg_lower == "status") {
        return {Status, true};
    }

    return {Status, false}; // Default to Status, but signal that no parameter was found
}

std::string extract_skill_name(const Seperator *sep, bool has_command_param)
{
    std::string skill_name_input = "";
    int end_arg = has_command_param ? sep->argnum - 1 : sep->argnum;

    for (int i = 1; i <= end_arg; ++i) {
        if (i > 1) skill_name_input += " ";
        skill_name_input += sep->arg[i];
    }

    return skill_name_input;
}

int find_skill_id(const std::string& skill_name_input,
                 const std::map<std::string, int>& skill_lookup_map,
                 const std::map<int, std::string>& id_to_name_map)
{
    // Parse numeric IDs without exceptions so malformed inputs cannot crash zone.
    int skill_id = 0;
    const char* begin = skill_name_input.data();
    const char* end = begin + skill_name_input.size();
    auto parse_result = std::from_chars(begin, end, skill_id);
    if (parse_result.ec == std::errc() && parse_result.ptr == end) {
        if (id_to_name_map.count(skill_id)) {
            return skill_id;
        }
    }

    // Try to find in the lookup map
    std::string skill_name_lower = Strings::ToLower(skill_name_input);
    auto it = skill_lookup_map.find(skill_name_lower);
    if (it != skill_lookup_map.end()) {
        return it->second;
    }

    return -1;
}

void process_command(Client *c, CommandMode cmd_mode, int skill_id,
                    const std::map<int, std::string>& id_to_name_map)
{
    std::string skill_name = id_to_name_map.at(skill_id);
    bool is_enabled;

    switch (cmd_mode) {
        case Enable:
            c->SetAutoSkillStatus((EQ::skills::SkillType)skill_id, true);
            c->Message(Chat::Skills, "Autoskill for %s is now ENABLED.", skill_name.c_str());
            break;
        case Disable:
            c->SetAutoSkillStatus((EQ::skills::SkillType)skill_id, false);
            c->Message(Chat::Skills, "Autoskill for %s is now disabled.", skill_name.c_str());
            break;
        case Status:
        default:
            is_enabled = c->GetAutoSkillStatus((EQ::skills::SkillType)skill_id);
            c->Message(Chat::Skills, "Autoskill for %s is currently %s.",
                      skill_name.c_str(), is_enabled ? "ENABLED" : "disabled");
            break;
    }
}
