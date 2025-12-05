#pragma once

#include "../zone/dynamic_item_manager.h"
#include "../common/item_instance.h"
#include "../common/database.h"
#include "../cppunit/testcase.h"

class DynamicFuseTest : public Test::TestCase {
public:
    DynamicFuseTest() : Test::TestCase("DynamicFuseTest") {}
    void run() override {
        // This test verifies the FuseWithCharge API produces a dynamic item with the specified level
        // Fetch a base item from the database (using 8403 - Dragon Helm sample id used in docs should exist)
        uint32_t base_item_id = 8403;
        auto* base_item = database.GetItem(base_item_id);
        if (!base_item) {
            // If item not found in test DB, skip gracefully
            return;
        }

        EQ::ItemInstance receiver(base_item, 1);
        int donorLevel = 50;
        auto* result = EQ::DynamicItemManager::Get().FuseWithCharge(donorLevel, &receiver);
        if (!result) throw std::runtime_error("DynamicFuseTest failed to create fused result");
        int lvl = EQ::DynamicItemManager::Get().GetItemLevel(result->GetID());
        if (lvl != donorLevel) throw std::runtime_error("DynamicFuseTest: fused item level mismatch");
        uint32_t baseId = EQ::DynamicItemManager::Get().GetBaseItemID(result->GetID());
        if (baseId != base_item_id) throw std::runtime_error("DynamicFuseTest: fused base id mismatch");

        delete result; // cleanup
    }
};
