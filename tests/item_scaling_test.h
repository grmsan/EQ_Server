#pragma once

#include "../../common/item_scaling_config.h"
#include "../cppunit/testcase.h"

class ItemScalingTest : public Test::TestCase {
public:
    ItemScalingTest() : Test::TestCase("ItemScalingTest") {}
    void run() override {
        // Load the sample JSON from the repo
        ItemScaling::Config::Get().Load("game_design/infinite_progression/item_scaling.json");

        // With the sample JSON, SpellDmgFromInt curve at level 10 should be 40 per 10 attribute
        int derived = ItemScaling::Config::Get().ComputeSpellDmgFromInt(10, 10);
        if (derived != 40) {
            throw std::runtime_error("ItemScalingTest SpellDmgFromInt(10,10) != 40");
        }

        // Attribute preference multipliers present vs absent
        double pm_present = ItemScaling::Config::Get().GetAttributePresenceMultiplier("ADex", true, 10);
        double pm_absent  = ItemScaling::Config::Get().GetAttributePresenceMultiplier("ADex", false, 10);
        if (pm_present <= pm_absent) {
            throw std::runtime_error("ItemScalingTest ADex presence multiplier should be > absence multiplier");
        }

        // global attribute curve & weapon curve tests
        double attrScale = ItemScaling::Config::Get().GetGlobalAttrCurve(10);
        if (attrScale <= 0.0) {
            throw std::runtime_error("ItemScalingTest Attribute curve returned non-positive value");
        }
        double wdam = ItemScaling::Config::Get().GetWeaponDamageCurve(10);
        if (wdam <= 0.0) {
            throw std::runtime_error("ItemScalingTest Weapon Damage curve returned non-positive value");
        }
        // AttributePool config
        std::string mode = ItemScaling::Config::Get().GetAttributeBudgetMode();
        if (mode != "auto") {
            throw std::runtime_error("ItemScalingTest expected AttributePool.mode == auto");
        }
        int staticBudget = ItemScaling::Config::Get().GetAttributeStaticBudget();
        if (staticBudget != 10) {
            throw std::runtime_error("ItemScalingTest expected AttributePool.static_budget == 10");
        }

        // Simulate a simple allocation preview (mimic #tune itemscale preview behavior)
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

        int bases[7] = {30, 0, 0, 0, 0, 0, 0};
        int level = 100;
        int rawScaled[7]; double weight[7]; int totalPool = 0;
        for (int i = 0; i < 7; ++i) {
            rawScaled[i] = CalculateTieredStat(bases[i], level, 1, 1);
            bool present = bases[i] > 0;
            weight[i] = ItemScaling::Config::Get().GetAttributePresenceMultiplier((i==0?"AStr":(i==1?"ASta":(i==2?"AAgi":(i==3?"ADex":(i==4?"AInt":(i==5?"AWis":"ACha")))))), present, level);
            totalPool += rawScaled[i];
        }
        std::string modeStr = ItemScaling::Config::Get().GetAttributeBudgetMode();
        if (modeStr != "auto") {
            throw std::runtime_error("ItemScalingTest expects attribute_pool.mode to be auto in the sample JSON");
        }
        // basic sanity: total pool > 0 for this example
        if (totalPool <= 0) throw std::runtime_error("ItemScalingTest expected non-zero total attribute pool for sample bases");

        // Compute allocations using configured weights (auto mode) and ensure sums match
        double totalWeight = 0.0; for (int i = 0; i < 7; ++i) totalWeight += weight[i];
        if (totalWeight <= 0.0) throw std::runtime_error("ItemScalingTest total weight invalid");
        int alloc[7]; int remaining = totalPool;
        for (int i = 0; i < 7; ++i) {
            double share = (weight[i] / totalWeight) * totalPool;
            alloc[i] = static_cast<int>(std::round(share));
            if (i != 6) remaining -= alloc[i];
            else alloc[i] = remaining;
        }
        int sumAlloc = 0; for (int i = 0; i < 7; ++i) sumAlloc += alloc[i];
        if (sumAlloc != totalPool) throw std::runtime_error("ItemScalingTest allocation sum mismatch in auto mode");

        // Static mode: set a small static budget and verify allocations sum to that budget
        ItemScaling::Config::Get().attribute_budget_mode = "static";
        ItemScaling::Config::Get().attribute_static_budget = 20;
        int staticPool = ItemScaling::Config::Get().GetAttributeStaticBudget();
        if (staticPool != 20) throw std::runtime_error("ItemScalingTest failed to set static budget");
        // recompute alloc using same weights but now the total pool should be staticPool
        remaining = staticPool;
        for (int i = 0; i < 7; ++i) {
            double share = (weight[i] / totalWeight) * staticPool;
            alloc[i] = static_cast<int>(std::round(share));
            if (i != 6) remaining -= alloc[i]; else alloc[i] = remaining;
        }
        sumAlloc = 0; for (int i = 0; i < 7; ++i) sumAlloc += alloc[i];
        if (sumAlloc != staticPool) throw std::runtime_error("ItemScalingTest allocation sum mismatch in static mode");

            // slot multiplier test: set Chest multiplier and ensure the pool scales
            ItemScaling::Config::Get().slot_multipliers["Chest"] = 1.5;
            int chestMask = (1 << EQ::invslot::slotChest);
            double chestMult = ItemScaling::Config::Get().GetSlotMultiplierByMask(chestMask);
            if (chestMult <= 1.0) throw std::runtime_error("ItemScalingTest expected chest multiplier > 1.0");
            int scaledPool = static_cast<int>(std::round(totalPool * chestMult));
            if (scaledPool <= totalPool) throw std::runtime_error("ItemScalingTest expected scaled pool to be larger than base pool for chest multiplier");

            // Test HP/AC per-slot multiplier lookup
            ItemScaling::Config::Get().slot_multipliers_by_stat["Chest"]["HP"] = 1.2;
            ItemScaling::Config::Get().slot_multipliers_by_stat["Chest"]["AC"] = 1.2;
            double hpMult = ItemScaling::Config::Get().GetSlotMultiplierByMask(chestMask, "HP");
            double acMult = ItemScaling::Config::Get().GetSlotMultiplierByMask(chestMask, "AC");
            if (hpMult <= 1.0 || acMult <= 1.0) throw std::runtime_error("ItemScalingTest expected chest HP/AC multiplier > 1.0");
    }
};
