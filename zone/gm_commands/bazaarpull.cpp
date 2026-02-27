#include "../client.h"
#include "../../common/strings.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace {
constexpr uint32 kBazaarZoneID = 151;
constexpr float kStageCenterX = -824.32f;
constexpr float kStageCenterY = 1.64f;
constexpr float kStageCenterZ = 3.44f;
constexpr float kStageMaxOffset = 132.0f; // 20x20 grid at 12 spacing -> 120, allow small margin
constexpr float kStageMaxZDelta = 6.0f;
constexpr uint32 kDefaultPullCount = 1;
constexpr uint32 kMaxPullCount = 20;
constexpr float kTwoPi = 6.28318530718f;
}

static bool IsSpawnInBazaarStageGrid(Spawn2* spawn)
{
	if (!spawn) {
		return false;
	}

	return
		std::fabs(spawn->GetX() - kStageCenterX) <= kStageMaxOffset &&
		std::fabs(spawn->GetY() - kStageCenterY) <= kStageMaxOffset &&
		std::fabs(spawn->GetZ() - kStageCenterZ) <= kStageMaxZDelta;
}

void command_bazaarpull(Client* c, const Seperator* sep)
{
	if (!c) {
		return;
	}

	if (!zone || zone->GetZoneID() != kBazaarZoneID) {
		c->Message(Chat::White, "This command may only be used in Bazaar.");
		return;
	}

	bool count_only = false;
	uint32 pull_count = kDefaultPullCount;

	if (sep->argnum >= 1 && sep->arg[1] && sep->arg[1][0] != '\0') {
		const auto arg = Strings::ToLower(sep->arg[1]);
		if (arg == "count" || arg == "status") {
			count_only = true;
		}
		else if (sep->IsNumber(1)) {
			pull_count = Strings::ToUnsignedInt(sep->arg[1], kDefaultPullCount);
			if (pull_count < 1) {
				pull_count = kDefaultPullCount;
			}
			if (pull_count > kMaxPullCount) {
				pull_count = kMaxPullCount;
			}
		}
		else {
			c->Message(Chat::White, "Usage: #bazaarpull [count|status]");
			return;
		}
	}

	std::vector<NPC*> staged_npcs;
	staged_npcs.reserve(entity_list.GetNPCList().size());

	for (const auto& npc_entity : entity_list.GetNPCList()) {
		auto* npc = npc_entity.second;
		if (!npc) {
			continue;
		}

		auto* spawn = npc->GetSpawn();
		if (!spawn) {
			continue;
		}

		if (IsSpawnInBazaarStageGrid(spawn)) {
			staged_npcs.emplace_back(npc);
		}
	}

	std::sort(
		staged_npcs.begin(),
		staged_npcs.end(),
		[](NPC* a, NPC* b) {
			auto* sa = a ? a->GetSpawn() : nullptr;
			auto* sb = b ? b->GetSpawn() : nullptr;
			if (!sa || !sb) {
				return a < b;
			}
			return sa->GetID() < sb->GetID();
		}
	);

	if (count_only) {
		c->Message(
			Chat::White,
			fmt::format(
				"Bazaar staged/unplaced NPCs currently spawned: {}",
				staged_npcs.size()
			).c_str()
		);
		return;
	}

	if (staged_npcs.empty()) {
		c->Message(Chat::White, "No staged/unplaced Bazaar NPCs are currently spawned.");
		return;
	}

	const uint32 to_move = std::min<uint32>(pull_count, static_cast<uint32>(staged_npcs.size()));
	const float center_x = c->GetX();
	const float center_y = c->GetY();
	const float center_z = c->GetZ();
	const float center_h = c->GetHeading();

	// Place pulled NPCs around the player in a small ring so they don't stack.
	const float radius = 12.0f;
	const float step = kTwoPi / static_cast<float>(std::max<uint32>(to_move, 1));

	for (uint32 i = 0; i < to_move; ++i) {
		const float angle = step * static_cast<float>(i);
		const float nx = center_x + (radius * std::cos(angle));
		const float ny = center_y + (radius * std::sin(angle));
		auto* npc = staged_npcs[i];
		npc->GMMove(nx, ny, center_z, center_h);
	}

	auto* first_spawn = staged_npcs.front()->GetSpawn();
	const uint32 first_spawn2_id = first_spawn ? first_spawn->GetID() : 0;

	c->Message(
		Chat::White,
		fmt::format(
			"Pulled {} staged Bazaar NPC{} near you. Remaining staged: {}. Use #spawnfix after placing each NPC.",
			to_move,
			to_move == 1 ? "" : "s",
			static_cast<uint32>(staged_npcs.size()) - to_move
		).c_str()
	);

	if (to_move == 1) {
		c->Message(
			Chat::White,
			fmt::format(
				"Current NPC: {} (Spawn2 ID: {}).",
				c->GetTargetDescription(staged_npcs.front()),
				first_spawn2_id
			).c_str()
		);
	}
}
