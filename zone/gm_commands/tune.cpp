#include "../client.h"
#include "../../common/item_scaling_config.h"
#include "../dynamic_item_manager.h"

void command_tune(Client *c, const Seperator *sep)
{
	//Work in progress - Kayen

	if (sep->arg[1][0] == '\0' || !strcasecmp(sep->arg[1], "help")) {
		c->Message(Chat::White, "Syntax: #tune [subcommand].");
		c->Message(Chat::White, "-- Tune System Commands --");
		c->Message(
			Chat::White,
			"-- Usage: Returns recommended combat statistical values based on a desired outcome through simulated combat."
		);
		c->Message(
			Chat::White,
			"-- This commmand can answer the following difficult questions whening tunings NPCs and Players."
		);
		c->Message(
			Chat::White,
			"-- Question: What is the average damage mitigation my AC provides against a specific targets attacks?"
		);
		c->Message(
			Chat::White,
			"-- Question: What is amount of AC would I need to add to acheive a specific average damage mitigation agianst specific targets attacks?"
		);
		c->Message(
			Chat::White,
			"-- Question: What is amount of AC would I need to add to my target to acheive a specific average damage mitigation from my attacks?"
		);
		c->Message(Chat::White, "-- Question: What is my targets average AC damage mitigation based on my ATK stat?");
		c->Message(
			Chat::White,
			"-- Question: What is amount of ATK would I need to add to myself to acheive a specific average damage mitigation on my target?"
		);
		c->Message(
			Chat::White,
			"-- Question: What is amount of ATK would I need to add to my target to acheive a specific average AC damage mitigation on myself?"
		);
		c->Message(Chat::White, "-- Question: What is my hit chance against a target?");
		c->Message(
			Chat::White,
			"-- Question: What is the amount of avoidance I need to add to my target to achieve a specific hit chance?"
		);
		c->Message(
			Chat::White,
			"-- Question: What is the amount of accuracy I need to add to my target to achieve a specific chance of hitting me?"
		);
		c->Message(Chat::White, "-- Question: ... and many more...");
		c->Message(Chat::White, " ");
		c->Message(Chat::White, "...#tune stats [A/D]");
		c->Message(
			Chat::White,
			"...#tune FindATK [A/D] [pct mitigation] [interval] [loop_max] [AC override] [Info Level]"
		);
		c->Message(
			Chat::White,
			"...#tune FindAC  [A/D] [pct mitigation] [interval] [loop_max] [ATK override] [Info Level] "
		);
		c->Message(
			Chat::White,
			"...#tune FindAccuracy  [A/D] [hit chance] [interval] [loop_max] [Avoidance override] [Info Level]"
		);
		c->Message(
			Chat::White,
			"...#tune FindAvoidance [A/D] [hit chance] [interval] [loop_max] [Accuracy override] [Info Level] "
		);
		c->Message(Chat::White, " ");
		c->Message(Chat::White, "-- DETAILS AND EXAMPLES ON USAGE");
		c->Message(Chat::White, " ");
		c->Message(
			Chat::White,
			"...Returns combat statistics, including AC mitigation pct, hit chance, and avoid melee chance for attacker and defender."
		);
		c->Message(Chat::White, "...#tune stats [A/D]");
		c->Message(Chat::White, "...");
		c->Message(
			Chat::White,
			"...Returns recommended ATK adjustment (+/-) on ATTACKER that will result in a specific average AC mitigation pct on DEFENDER. "
		);
		c->Message(
			Chat::White,
			"...#tune FindATK [A/D] [pct mitigation] [interval][loop_max][AC override][Info Level]"
		);
		c->Message(
			Chat::White,
			"...Example: Find the amount of ATK stat I need to add to the targeted NPC so that it hits me for 50 pct damage on average."
		);
		c->Message(Chat::White, "...Example: #tune FindATK D 50");
		c->Message(Chat::White, "...");
		c->Message(
			Chat::White,
			"...Returns recommended AC adjustment(+/-) on DEFENDER for a specific average AC mitigation pct from ATTACKER. "
		);
		c->Message(
			Chat::White,
			"...#tune FindAC  [A/D] [pct mitigation] [interval][loop_max][ATK override][Info Level] "
		);
		c->Message(
			Chat::White,
			"...Example: Find the amount of AC stat I need to add to the targeted NPC so that I hit it for 70 pct damage on average."
		);
		c->Message(Chat::White, "...Example: #tune FindAC D 70");
		c->Message(Chat::White, "...");
		c->Message(
			Chat::White,
			"...Returns recommended Accuracy adjustment (+/-) on ATTACKER that will result in a specific hit chance pct on DEFENDER. "
		);
		c->Message(
			Chat::White,
			"...#tune FindAccuracy  [A/D] [hit chance] [interval][loop_max][Avoidance override][Info Level]"
		);
		c->Message(
			Chat::White,
			"...Example: Find the amount of Accuracy stat I need to add to the targeted NPC so that it has a 60 pct hit chance against me."
		);
		c->Message(Chat::White, "...Example: #tune FindAccuracy D 60");
		c->Message(Chat::White, "...");
		c->Message(
			Chat::White,
			"...Returns recommended Avoidance adjustment (+/-) on DEFENDER for in a specific hit chance pct from ATTACKER. "
		);
		c->Message(
			Chat::White,
			"...#tune FindAvoidance [A/D] [hit chance] [interval][loop_max][Accuracy override][Info Level] "
		);
		c->Message(
			Chat::White,
			"...Example: Find the amount of Avoidance stat I need to add to the targeted NPC so that I have a 30 pct hit chance against it."
		);
		c->Message(Chat::White, "...Example: #tune FindAvoidance D 30");
		c->Message(Chat::White, "... ");
		c->Message(Chat::White, "...Usage: [A/D] You must input either A or D.");
		c->Message(Chat::White, "...Category [A] : YOU are the ATTACKER. YOUR TARGET is the DEFENDER.");
		c->Message(Chat::White, "...Category [D] : YOU are the DEFENDER. YOUR TARGET is the ATTACKER.");
		c->Message(Chat::White, "...If TARGET is in combat, DEFENDER is the TARGETs TARGET.");

		c->Message(Chat::White, " ");

		c->Message(
			Chat::White,
			"-- Warning: The calculations done in this process are intense and can potentially cause zone crashes depending on parameters set, use with caution!"
		);
		c->Message(Chat::White, "-- Below are OPTIONAL parameters.");
		c->Message(
			Chat::White,
			"-- Note: [interval] Determines how much the stat being checked increases/decreases till it finds the best result. Lower is more accurate. Default=10"
		);
		c->Message(
			Chat::White,
			"-- Note: [loop_max] Determines how many iterations are done to increases/decreases the stat till it finds the best result. Higher is more accurate. Default=1000"
		);
		c->Message(
			Chat::White,
			"-- Note: [Stat Override] Will override that stat on mob being checked with the specified value. Default=0"
		);
		c->Message(
			Chat::White,
			"-- Example: If as the attacker you want to find the ATK value you would need to have agianst a target with 1000 AC to achieve an average AC mitigation of 50 pct."
		);
		c->Message(Chat::White, "-- Example: #tune FindATK A 50 0 0 1000");
		c->Message(Chat::White, "-- Note: [Info Level] How much parsing detail is displayed[0 - 1]. Default: [0] ");
		c->Message(Chat::White, " ");

		return;
	}

	if (!strcasecmp(sep->arg[1], "itemscale")) {
		// Usage: #tune itemscale spelldmg <intValue> <level>
		//        #tune itemscale heal <wisValue> <level>
		if (!strcasecmp(sep->arg[2], "reload")) {
			ItemScaling::Config::Get().Load();
			c->Message(Chat::White, "Item scaling config reloaded.");
		} else if (!strcasecmp(sep->arg[2], "spelldmg") && sep->IsNumber(3) && sep->IsNumber(4)) {
			int attr = Strings::ToInt(sep->arg[3]);
			int lvl = Strings::ToInt(sep->arg[4]);
			int derived = ItemScaling::Config::Get().ComputeSpellDmgFromInt(attr, lvl);
			c->Message(Chat::White, fmt::format("SpellDmg from INT={} at lvl {} => {}", attr, lvl, derived).c_str());
		} else if (!strcasecmp(sep->arg[2], "class")) {
			c->Message(Chat::White, "#tune itemscale class is deprecated (we no longer apply class multipliers on equip)");
		} else if (!strcasecmp(sep->arg[2], "pref") && sep->arg[3][0] != '\0' && sep->arg[4][0] != '\0') {
			// Usage: #tune itemscale pref <AttrName> present|absent
			std::string attrName = sep->arg[3];
			std::string mode = sep->arg[4];
			bool present = !strcasecmp(mode.c_str(), "present");
			double v = ItemScaling::Config::Get().GetAttributePresenceMultiplier(attrName, present, 1);
			c->Message(Chat::White, fmt::format("Attribute preference multiplier {} {} => {}", attrName, mode, v).c_str());
		} else if (!strcasecmp(sep->arg[2], "heal") && sep->IsNumber(3) && sep->IsNumber(4)) {
			int attr = Strings::ToInt(sep->arg[3]);
			int lvl = Strings::ToInt(sep->arg[4]);
			int derived = ItemScaling::Config::Get().ComputeHealFromWis(attr, lvl);
			c->Message(Chat::White, fmt::format("HealAmt from WIS={} at lvl {} => {}", attr, lvl, derived).c_str());
		} else if (!strcasecmp(sep->arg[2], "preview")) {
			// Usage: #tune itemscale preview <str> <sta> <agi> <dex> <int> <wis> <cha> <level>
			if (sep->IsNumber(3) && sep->IsNumber(4) && sep->IsNumber(5) && sep->IsNumber(6) && sep->IsNumber(7) && sep->IsNumber(8) && sep->IsNumber(9) && sep->IsNumber(10)) {
				int str = Strings::ToInt(sep->arg[3]);
				int sta = Strings::ToInt(sep->arg[4]);
				int agi = Strings::ToInt(sep->arg[5]);
				int dex = Strings::ToInt(sep->arg[6]);
				int in  = Strings::ToInt(sep->arg[7]);
				int wis = Strings::ToInt(sep->arg[8]);
				int cha = Strings::ToInt(sep->arg[9]);
				int level = Strings::ToInt(sep->arg[10]);
				auto CalculateTieredStat = [](int base_value, int level, int base_increment, int tier_bonus, int tier_size = 10) -> int {
					if (level <= 0) return base_value;
					int total = base_value;
					int tier = level / tier_size;
					for (int t = 0; t < tier; t++) {
						int increment = base_increment + (t * tier_bonus);
						total += tier_size * increment;
					}
					int remaining_levels = level % tier_size;
					int current_tier_increment = base_increment + (tier * tier_bonus);
					total += remaining_levels * current_tier_increment;
					return total;
				};
				int bases[7] = {str, sta, agi, dex, in, wis, cha};
				int rawScaled[7]; double weight[7];
				int totalPool = 0;
				for (int i = 0; i < 7; ++i) {
					rawScaled[i] = CalculateTieredStat(bases[i], level, 1, 1);
					bool present = bases[i] > 0;
					weight[i] = ItemScaling::Config::Get().GetAttributePresenceMultiplier((i==0?"AStr":(i==1?"ASta":(i==2?"AAgi":(i==3?"ADex":(i==4?"AInt":(i==5?"AWis":"ACha")))))), present, level);
					totalPool += rawScaled[i];
				}
				std::string mode = ItemScaling::Config::Get().GetAttributeBudgetMode();
				if (mode == "static") {
					int sb = ItemScaling::Config::Get().GetAttributeStaticBudget();
					if (sb > 0) totalPool = sb;
				}
				double totalWeight = 0.0; for (int i = 0; i < 7; ++i) totalWeight += weight[i];
				if (totalWeight <= 0.0) { totalWeight = 7.0; for (int i = 0; i < 7; ++i) weight[i] = 1.0; }
				int remaining = totalPool;
				int alloc[7];
				for (int i = 0; i < 7; ++i) {
					double share = (weight[i] / totalWeight) * totalPool;
					int val = static_cast<int>(std::round(share));
					if (i == 6) { val = remaining; } else { remaining -= val; }
					alloc[i] = val;
				}
				for (int i = 0; i < 7; ++i) {
					std::string aname = (i==0?"AStr":(i==1?"ASta":(i==2?"AAgi":(i==3?"ADex":(i==4?"AInt":(i==5?"AWis":"ACha"))))));
					c->Message(Chat::White, fmt::format("{:s} base={:d} rawScaled={:d} weight={:.2f} alloc={:d}", aname, bases[i], rawScaled[i], weight[i], alloc[i]).c_str());
				}
				} else {
					c->Message(Chat::White, "Usage: #tune itemscale preview <str> <sta> <agi> <dex> <int> <wis> <cha> <level>");
				}
			} else if (!strcasecmp(sep->arg[2], "fuse_charge") && sep->IsNumber(3) && sep->IsNumber(4)) {
				// Usage: #tune itemscale fuse_charge <receiver_base_id> <donor_level>
				uint32 receiver_base_id = static_cast<uint32>(Strings::ToInt(sep->arg[3]));
				int donorLevel = Strings::ToInt(sep->arg[4]);
				if (receiver_base_id == 0 || donorLevel <= 0) {
					c->Message(Chat::Red, "Invalid receiver or donor level");
				} else {
					// Fetch base item data
					auto* base_item = database.GetItem(receiver_base_id);
					if (!base_item) {
						c->Message(Chat::Red, "Base item not found");
					} else {
						// Create a temporary receiver instance
						EQ::ItemInstance receiver(base_item, 1);
						// Create fused item via charge
						auto* result = EQ::DynamicItemManager::Get().FuseWithCharge(donorLevel, &receiver);
						if (result) {
							c->Message(Chat::White, fmt::format("FuseWithCharge created {} (+{}): AC={}, HP={}, STR={}", result->GetItem()->Name, donorLevel, result->GetItem()->AC, result->GetItem()->HP, result->GetItem()->AStr).c_str());
							delete result; // preview only
						} else {
							c->Message(Chat::Red, "Failed to fuse item with charge");
						}
					}
				}
		} else {
			c->Message(Chat::White, "Usage: #tune itemscale spelldmg <intValue> <level>");
			c->Message(Chat::White, "       #tune itemscale heal <wisValue> <level>");
		}
	}
	/*
		Category A: YOU are the attacker and your target is the defender
		Category D: YOU are the defender and your target is the attacker
	*/

	Mob *attacker = c;
	Mob *defender = c->GetTarget();

	if (!defender) {
		c->Message(Chat::White, "[#Tune] - Error no target selected. [#Tune help]");
		return;
	}

	//Use if checkings on engaged targets.
	Mob *ttarget = attacker->GetTarget();
	if (ttarget) {
		defender = ttarget;
	}

	if (!strcasecmp(sep->arg[1], "stats")) {

		if (!strcasecmp(sep->arg[2], "A")) {
			c->TuneGetStats(defender, attacker);
		} else if (!strcasecmp(sep->arg[2], "D")) {
			c->TuneGetStats(attacker, defender);
		} else {
			c->TuneGetStats(defender, attacker);
		}
		return;
	}

	if (!strcasecmp(sep->arg[1], "FindATK")) {
		float pct_mitigation = Strings::ToFloat(sep->arg[3]);
		int   interval       = Strings::ToInt(sep->arg[4]);
		int   max_loop       = Strings::ToInt(sep->arg[5]);
		int   ac_override    = Strings::ToInt(sep->arg[6]);
		int   info_level     = Strings::ToInt(sep->arg[7]);

		if (!pct_mitigation) {
			c->Message(Chat::White, "[#Tune] - Error must enter the desired percent mitigation on defender.");
			c->Message(
				Chat::White,
				"...Returns recommended ATK adjustment (+/-) on ATTACKER that will result in a specific average AC mitigation pct on DEFENDER. "
			);
			c->Message(
				Chat::White,
				"...#tune FindATK [A/D] [pct mitigation] [interval][loop_max][AC override][Info Level]"
			);
			c->Message(

				Chat::White,
				"...Example: Find the amount of ATK stat I need to add to the targeted NPC so that it hits me for 50 pct damage on average."
			);
			c->Message(Chat::White, "...Example: #tune FindATK D 50");
			return;
		}

		if (!interval) {
			interval = 10;
		}
		if (!max_loop) {
			max_loop = 1000;
		}
		if (!ac_override) {
			ac_override = 0;
		}
		if (!info_level) {
			info_level = 0;
		}

		if (!strcasecmp(sep->arg[2], "A")) {
			c->TuneGetATKByPctMitigation(
				defender,
				attacker,
				pct_mitigation,
				interval,
				max_loop,
				ac_override,
				info_level
			);
		}
		else if (!strcasecmp(sep->arg[2], "D")) {
			c->TuneGetATKByPctMitigation(
				attacker,
				defender,
				pct_mitigation,
				interval,
				max_loop,
				ac_override,
				info_level
			);
		}
		else {
			c->Message(Chat::White, "#Tune - Error no category selcted. [#Tune help]");
			c->Message(
				Chat::White,
				"Usage #tune FindATK [A/B] [pct mitigation] [interval][loop_max][AC Overwride][Info Level] "
			);
			c->Message(Chat::White, "...Usage: [A/D] You must input either A or D.");
			c->Message(Chat::White, "...Category [A] : YOU are the ATTACKER. YOUR TARGET is the DEFENDER.");
			c->Message(Chat::White, "...Category [D] : YOU are the DEFENDER. YOUR TARGET is the ATTACKER.");
			c->Message(Chat::White, "...If TARGET is in combat, DEFENDER is the TARGETs TARGET.");
			c->Message(Chat::White, "... ");
			c->Message(
				Chat::White,
				"...Example: Find the amount of ATK stat I need to add to the targeted NPC so that it hits me for 50 pct damage on average."
			);
			c->Message(Chat::White, "...Example: #tune FindATK D 50");
		}
		return;
	}

	if (!strcasecmp(sep->arg[1], "FindAC")) {
		float pct_mitigation = Strings::ToFloat(sep->arg[3]);
		int   interval       = Strings::ToInt(sep->arg[4]);
		int   max_loop       = Strings::ToInt(sep->arg[5]);
		int   atk_override   = Strings::ToInt(sep->arg[6]);
		int   info_level     = Strings::ToInt(sep->arg[7]);

		if (!pct_mitigation) {
			c->Message(Chat::White, "#Tune - Error must enter the desired percent mitigation on defender.");
			c->Message(
				Chat::White,
				"...Returns recommended AC adjustment(+/-) on DEFENDER for a specific average AC mitigation pct from ATTACKER. "
			);
			c->Message(
				Chat::White,
				"...#tune FindAC  [A/D] [pct mitigation] [interval][loop_max][ATK override][Info Level] "
			);
			c->Message(
				Chat::White,
				"...Example: Find the amount of AC stat I need to add to the targeted NPC so that I hit it for 70 pct damage on average."
			);
			c->Message(Chat::White, "...Example: #tune FindAC D 70");
			return;
		}

		if (!interval) {
			interval = 10;
		}
		if (!max_loop) {
			max_loop = 1000;
		}
		if (!atk_override) {
			atk_override = 0;
		}
		if (!info_level) {
			info_level = 0;
		}

		if (!strcasecmp(sep->arg[2], "A")) {
			c->TuneGetACByPctMitigation(
				defender,
				attacker,
				pct_mitigation,
				interval,
				max_loop,
				atk_override,
				info_level
			);
		}
		else if (!strcasecmp(sep->arg[2], "D")) {
			c->TuneGetACByPctMitigation(
				attacker,
				defender,
				pct_mitigation,
				interval,
				max_loop,
				atk_override,
				info_level
			);
		}
		else {
			c->Message(Chat::White, "#Tune - Error no category selcted. [#Tune help]");
			c->Message(
				Chat::White,
				"Usage #tune FindATK [A/B] [pct mitigation] [interval][loop_max][AC Overwride][Info Level] "
			);
			c->Message(Chat::White, "...Usage: [A/D] You must input either A or D.");
			c->Message(Chat::White, "...Category [A] : YOU are the ATTACKER. YOUR TARGET is the DEFENDER.");
			c->Message(Chat::White, "...Category [D] : YOU are the DEFENDER. YOUR TARGET is the ATTACKER.");
			c->Message(Chat::White, "...If TARGET is in combat, DEFENDER is the TARGETs TARGET.");
			c->Message(Chat::White, "... ");
			c->Message(
				Chat::White,
				"...Example: Find the amount of AC stat I need to add to the targeted NPC so that I hit it for 70 pct damage on average."
			);
			c->Message(Chat::White, "...Example: #tune FindAC D 70");
		}

		return;
	}

	if (!strcasecmp(sep->arg[1], "FindAccuracy")) {
		float hit_chance     = Strings::ToFloat(sep->arg[3]);
		int   interval       = Strings::ToInt(sep->arg[4]);
		int   max_loop       = Strings::ToInt(sep->arg[5]);
		int   avoid_override = Strings::ToInt(sep->arg[6]);
		int   info_level     = Strings::ToInt(sep->arg[7]);

		if (!hit_chance) {
			c->Message(Chat::White, "#Tune - Error must enter the desired hit chance on defender.");
			c->Message(
				Chat::White,
				"...Returns recommended Accuracy adjustment (+/-) on ATTACKER that will result in a specific hit chance pct on DEFENDER. "
			);
			c->Message(
				Chat::White,
				"...#tune FindAccuracy  [A/D] [hit chance] [interval][loop_max][Avoidance override][Info Level]"
			);
			c->Message(
				Chat::White,
				"...Example: Find the amount of Accuracy stat I need to add to the targeted NPC so that it has a 60 pct hit chance against me."
			);
			c->Message(Chat::White, "...Example: #tune FindAccuracy D 60");
			return;
		}



		if (!interval) {
			interval = 10;
		}
		if (!max_loop) {
			max_loop = 1000;
		}
		if (!avoid_override) {
			avoid_override = 0;
		}
		if (!info_level) {
			info_level = 0;
		}

		if (!strcasecmp(sep->arg[2], "A")) {
			c->TuneGetAccuracyByHitChance(
				defender,
				attacker,
				hit_chance,
				interval,
				max_loop,
				avoid_override,
				info_level
			);
		}
		else if (!strcasecmp(sep->arg[2], "D")) {
			c->TuneGetAccuracyByHitChance(
				attacker,
				defender,
				hit_chance,
				interval,
				max_loop,
				avoid_override,
				info_level
			);
		}
		else {
			c->Message(Chat::White, "#Tune - Error no category selcted. [#Tune help]");
			c->Message(
				Chat::White,
				"...#tune FindAccuracy  [A/D] [hit chance] [interval][loop_max][Avoidance override][Info Level]"
			);
			c->Message(Chat::White, "...Usage: [A/D] You must input either A or D.");
			c->Message(Chat::White, "...Category [A] : YOU are the ATTACKER. YOUR TARGET is the DEFENDER.");
			c->Message(Chat::White, "...Category [D] : YOU are the DEFENDER. YOUR TARGET is the ATTACKER.");
			c->Message(Chat::White, "...If TARGET is in combat, DEFENDER is the TARGETs TARGET.");
			c->Message(Chat::White, "... ");
			c->Message(
				Chat::White,
				"...Example: Find the amount of Accuracy stat I need to add to the targeted NPC so that it has a 60 pct hit chance against me."
			);
			c->Message(Chat::White, "...Example: #tune FindAccuracy D 60");
		}

		return;
	}

	if (!strcasecmp(sep->arg[1], "FindAvoidance")) {
		float hit_chance   = Strings::ToFloat(sep->arg[3]);
		int   interval     = Strings::ToInt(sep->arg[4]);
		int   max_loop     = Strings::ToInt(sep->arg[5]);
		int   acc_override = Strings::ToInt(sep->arg[6]);
		int   info_level   = Strings::ToInt(sep->arg[7]);

		if (!hit_chance) {
			c->Message(Chat::White, "#Tune - Error must enter the desired hit chance on defender.");
			c->Message(
				Chat::White,
				"...Returns recommended Avoidance adjustment (+/-) on DEFENDER for in a specific hit chance pct from ATTACKER. "
			);
			c->Message(
				Chat::White,
				"...#tune FindAvoidance [A/D] [hit chance] [interval][loop_max][Accuracy override][Info Level] "
			);
			c->Message(
				Chat::White,
				"...Example: Find the amount of Avoidance stat I need to add to the targeted NPC so that I have a 30 pct hit chance against it."
			);
			c->Message(Chat::White, "...Example: #tune FindAvoidance D 30");
			return;
		}
		if (!interval) {
			interval = 10;
		}
		if (!max_loop) {
			max_loop = 1000;
		}
		if (!acc_override) {
			acc_override = 0;
		}
		if (!info_level) {
			info_level = 0;
		}

		if (!strcasecmp(sep->arg[2], "A")) {
			c->TuneGetAvoidanceByHitChance(
				defender,
				attacker,
				hit_chance,
				interval,
				max_loop,
				acc_override,
				info_level
			);
		}
		else if (!strcasecmp(sep->arg[2], "D")) {
			c->TuneGetAvoidanceByHitChance(
				attacker,
				defender,
				hit_chance,
				interval,
				max_loop,
				acc_override,
				info_level
			);
		}
		else {
			c->Message(Chat::White, "#Tune - Error no category selcted. [#Tune help]");
			c->Message(
				Chat::White,
				"...#tune FindAvoidance [A/D] [hit chance] [interval][loop_max][Accuracy override][Info Level] "
			);
			c->Message(Chat::White, "...Usage: [A/D] You must input either A or D.");
			c->Message(Chat::White, "...Category [A] : YOU are the ATTACKER. YOUR TARGET is the DEFENDER.");
			c->Message(Chat::White, "...Category [D] : YOU are the DEFENDER. YOUR TARGET is the ATTACKER.");
			c->Message(Chat::White, "...If TARGET is in combat, DEFENDER is the TARGETs TARGET.");
			c->Message(Chat::White, "... ");
			c->Message(
				Chat::White,
				"...Example: Find the amount of Avoidance stat I need to add to the targeted NPC so that I have a 30 pct hit chance against it."
			);
			c->Message(Chat::White, "...Example: #tune FindAvoidance D 30");
		}

		return;
	}

	c->Message(Chat::White, "#Tune - Error no command [#Tune help]");
	return;
}

