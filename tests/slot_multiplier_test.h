#pragma once

#include "../cppunit/testcase.h"
#include "../../common/item_scaling_config.h"
#include "../../common/item_data.h"
#include "../../common/item_instance.h"

class SlotMultiplierTest : public Test::TestCase {
public:
    SlotMultiplierTest() : Test::TestCase("SlotMultiplierTest") {}
    void run() override {
        // Load sample config
        ItemScaling::Config::Get().Load("game_design/infinite_progression/item_scaling.json");

        // Setup a base item with HP/AC and chest slot
        EQ::ItemData baseItem{};
        baseItem.ID = 10001;
        baseItem.Slots = (1 << EQ::invslot::slotChest);
        baseItem.HP = 100;
        baseItem.AC = 10;
        baseItem.Damage = 5;
        baseItem.Attack = 2;

        // Save current values
        double originalHPSlot = ItemScaling::Config::Get().GetSlotMultiplierByMask(baseItem.Slots, "HP");

        // Set HP slot multiplier to 1.0 and compute ScaleItem result (level 100 -> exp=10000)
        ItemScaling::Config::Get().slot_multipliers_by_stat["Chest"]["HP"] = 1.0;
        EQ::ItemInstance inst1(&baseItem, 0);
        inst1.SetExp(10000);
        inst1.SetScaling(true);
        inst1.ScaleItem();
        int hpA = inst1.GetScaledItem()->HP;

        // Set HP slot multiplier to 1.5 and re-scale
        ItemScaling::Config::Get().slot_multipliers_by_stat["Chest"]["HP"] = 1.5;
        EQ::ItemInstance inst2(&baseItem, 0);
        inst2.SetExp(10000);
        inst2.SetScaling(true);
        inst2.ScaleItem();
        int hpB = inst2.GetScaledItem()->HP;

        if (hpA <= 0) throw std::runtime_error("SlotMultiplierTest: unexpected hpA <= 0");
        double expectedB = std::round(hpA * 1.5);
        if (hpB != static_cast<int>(expectedB)) {
            throw std::runtime_error("SlotMultiplierTest: ScaleItem HP slot multiplier not applied as expected (" + std::to_string(hpA) + " -> " + std::to_string(hpB) + ")");
        }

        // Dynamic scale tests: set HP slot multiplier back to 1.0, compute dynamic scaled
        ItemScaling::Config::Get().slot_multipliers_by_stat["Chest"]["HP"] = 1.0;
        EQ::ItemInstance dinst1(&baseItem, 0);
        dinst1.ScaleDynamicItem(100);
        int dhpA = dinst1.GetScaledItem()->HP;

        // Now set slot multiplier to 1.5 and scale
        ItemScaling::Config::Get().slot_multipliers_by_stat["Chest"]["HP"] = 1.5;
        EQ::ItemInstance dinst2(&baseItem, 0);
        dinst2.ScaleDynamicItem(100);
        int dhpB = dinst2.GetScaledItem()->HP;

        if (dhpA <= 0) throw std::runtime_error("SlotMultiplierTest: unexpected dhpA <= 0");
        double dexpectedB = std::round(dhpA * 1.5);
        if (dhpB != static_cast<int>(dexpectedB)) {
            throw std::runtime_error("SlotMultiplierTest: ScaleDynamicItem HP slot multiplier not applied as expected (" + std::to_string(dhpA) + " -> " + std::to_string(dhpB) + ")");
        }

        // restore original
        ItemScaling::Config::Get().slot_multipliers_by_stat["Chest"]["HP"] = originalHPSlot;
    }
};
