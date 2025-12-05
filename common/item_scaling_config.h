// Item scaling configuration and helper functions
#pragma once

#include <vector>
#include <utility>
#include <string>
#include <map>
#include <cstdint>

namespace ItemScaling {

struct CurvePoint {
    int level;
    double value; // value-per-10-attribute-points by default
};

class Config {
public:
    static Config &Get();
    void Load(const std::string &path = "");

    // SpellDmg from INT behavior
    bool spell_dmg_from_int_enabled = true;
    std::string spell_dmg_from_int_mode = "divisor"; // "divisor" or "curve"
    int spell_dmg_from_int_divisor = 10; // default: 1 per 10 INT
    std::vector<CurvePoint> spell_dmg_from_int_curve; // piecewise points (level,value) if mode=="curve"

    // Heal from WIS
    bool heal_from_wis_enabled = true;
    std::string heal_from_wis_mode = "divisor";
    int heal_from_wis_divisor = 10;
    std::vector<CurvePoint> heal_from_wis_curve;

    // Attribute preference multipliers (presence vs absence)
    struct AttributeConfig {
        double presence_mult = 1.0;
        double absence_mult = 1.0;
        std::vector<CurvePoint> curve; // optional per-level curve
    };

    AttributeConfig attr_AStr;
    AttributeConfig attr_ASta;
    AttributeConfig attr_AAgi;
    AttributeConfig attr_ADex;
    AttributeConfig attr_AInt;
    AttributeConfig attr_AWis;
    AttributeConfig attr_ACha;

    // Global attribute curve controls the base scaling multiplier for all attributes
    std::vector<CurvePoint> global_attribute_curve;

    // Attribute pool distribution mode and static budget
    // mode: "auto" | "static" | "none" (fallback to legacy multiplicative behavior)
    std::string attribute_budget_mode = "auto";
    int attribute_static_budget = 0; // only used when mode=="static"

    // Weapon & mod2 curves: maps stat name to curve points
    std::map<std::string, std::vector<CurvePoint>> mod2_curves;
    std::vector<CurvePoint> weapon_damage_curve;
    std::vector<CurvePoint> weapon_attack_curve;
    // Per-slot multipliers (e.g., Chest: { Attributes:1.5, HP:1.2, AC:1.2 })
    // Maps slot name to a map of statKey->multiplier. Example: slot_multipliers_by_stat["Chest"]["Attributes"] = 1.5
    std::map<std::string, std::map<std::string, double>> slot_multipliers_by_stat;

    // Evaluate curve (linear interpolation). If points empty, returns 1.0
    double EvaluateCurve(const std::vector<CurvePoint> &curve, int level) const;

    // Compute derived values
    int ComputeSpellDmgFromInt(int attrValue, int level) const;
    int ComputeHealFromWis(int attrValue, int level) const;

    // Attribute preference helper
    double GetAttributePresenceMultiplier(const std::string &attrName, bool present, int level = 1) const;
    // Slot multiplier helpers
    // Get slot multiplier for a specific slot (by name) and statKey, default statKey="Attributes"
    double GetSlotMultiplierByName(const std::string &slotName, const std::string &statKey = "Attributes") const;
    // Get slot multiplier by using the item slot mask
    double GetSlotMultiplierByMask(uint32_t slotMask, const std::string &statKey = "Attributes") const;
    // Class multiplier helper
    double GetClassMultiplier(const std::string &className, const std::string &statKey) const; // deprecated; returns 1.0
    // Evaluate configured curves
    double GetGlobalAttrCurve(int level) const;
    double GetWeaponDamageCurve(int level) const;
    double GetWeaponAttackCurve(int level) const;
    double GetMod2Curve(const std::string &mod2Key, int level) const;
    std::string GetAttributeBudgetMode() const { return attribute_budget_mode; }
    int GetAttributeStaticBudget() const { return attribute_static_budget; }

private:
    Config();
    ~Config();
};

} // namespace ItemScaling
