#include "item_scaling_config.h"
#include "json_config.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include "emu_constants.h"

using EQ::JsonConfigFile;

namespace ItemScaling {

Config::Config() {}
Config::~Config() {}

Config &Config::Get() {
    static Config cfg;
    static bool loaded = false;
    if (!loaded) {
        cfg.Load();
        loaded = true;
    }

    // Note: Config::Get() does not directly parse JSON. Use Load() to refresh config.
    return cfg;
}

void Config::Load(const std::string &path) {
    // default path to game_design configuration
    std::string file = path;
    if (file.empty()) {
        file = "game_design/infinite_progression/item_scaling.json";
    }
    JsonConfigFile cfg = JsonConfigFile::Load(file);

    // SpellDmg from INT
    if (!cfg.RawHandle().isNull() && cfg.RawHandle().isMember("SpellDmgFromInt")) {
        auto &node = cfg.RawHandle()["SpellDmgFromInt"];
        try {
            if (node.isMember("enabled")) spell_dmg_from_int_enabled = node["enabled"].asBool();
            if (node.isMember("mode")) spell_dmg_from_int_mode = node["mode"].asString();
            if (node.isMember("divisor")) spell_dmg_from_int_divisor = node["divisor"].asInt();
            if (node.isMember("curve") && node["curve"].isMember("points")) {
                spell_dmg_from_int_curve.clear();
                for (auto &pt : node["curve"]["points"]) {
                    if (pt.isArray() && pt.size() == 2) {
                        CurvePoint cp;
                        cp.level = pt[0].asInt();
                        cp.value = pt[1].asDouble();
                        spell_dmg_from_int_curve.push_back(cp);
                    }
                }
                // sort points
                std::sort(spell_dmg_from_int_curve.begin(), spell_dmg_from_int_curve.end(), [](const CurvePoint&a, const CurvePoint&b){return a.level < b.level;});
            }
        } catch (...) { /* fallback to defaults */ }
    }

    // Heal from WIS
    if (!cfg.RawHandle().isNull() && cfg.RawHandle().isMember("HealFromWis")) {
        auto &node = cfg.RawHandle()["HealFromWis"];
        try {
            if (node.isMember("enabled")) heal_from_wis_enabled = node["enabled"].asBool();
            if (node.isMember("mode")) heal_from_wis_mode = node["mode"].asString();
            if (node.isMember("divisor")) heal_from_wis_divisor = node["divisor"].asInt();
            if (node.isMember("curve") && node["curve"].isMember("points")) {
                heal_from_wis_curve.clear();
                for (auto &pt : node["curve"]["points"]) {
                    if (pt.isArray() && pt.size() == 2) {
                        CurvePoint cp;
                        cp.level = pt[0].asInt();
                        cp.value = pt[1].asDouble();
                        heal_from_wis_curve.push_back(cp);
                    }
                }
                std::sort(heal_from_wis_curve.begin(), heal_from_wis_curve.end(), [](const CurvePoint&a, const CurvePoint&b){return a.level < b.level;});
            }
        } catch (...) { }
    }

    // Attribute preferences
    if (!cfg.RawHandle().isNull() && cfg.RawHandle().isMember("AttributePreferences")) {
        auto &node = cfg.RawHandle()["AttributePreferences"];
        auto parseAttr = [&](const std::string &key, AttributeConfig &out) {
            try {
                if (node.isMember(key)) {
                    auto &n = node[key];
                    if (n.isMember("presence_mult")) out.presence_mult = n["presence_mult"].asDouble();
                    if (n.isMember("absence_mult")) out.absence_mult = n["absence_mult"].asDouble();
                    if (n.isMember("curve") && n["curve"].isMember("points")) {
                        out.curve.clear();
                        for (auto &pt : n["curve"]["points"]) {
                            if (pt.isArray() && pt.size() == 2) {
                                CurvePoint cp;
                                cp.level = pt[0].asInt();
                                cp.value = pt[1].asDouble();
                                out.curve.push_back(cp);
                            }
                        }
                        std::sort(out.curve.begin(), out.curve.end(), [](const CurvePoint &a, const CurvePoint &b){ return a.level < b.level; });
                    }
                }
            } catch (...) {}
        };
        parseAttr("AStr", attr_AStr);
        parseAttr("ASta", attr_ASta);
        parseAttr("AAgi", attr_AAgi);
        parseAttr("ADex", attr_ADex);
        parseAttr("AInt", attr_AInt);
        parseAttr("AWis", attr_AWis);
        parseAttr("ACha", attr_ACha);
    }

    // Parse global attribute curve
    if (!cfg.RawHandle().isNull() && cfg.RawHandle().isMember("AttributeCurve")) {
        auto &node = cfg.RawHandle()["AttributeCurve"];
        if (node.isMember("points")) {
            global_attribute_curve.clear();
            for (auto &pt : node["points"]) {
                if (pt.isArray() && pt.size() == 2) {
                    CurvePoint cp;
                    cp.level = pt[0].asInt();
                    cp.value = pt[1].asDouble();
                    global_attribute_curve.push_back(cp);
                }
            }
            std::sort(global_attribute_curve.begin(), global_attribute_curve.end(), [](const CurvePoint&a,const CurvePoint&b){ return a.level < b.level; });
        }
    }

    // Weapon curves
    if (!cfg.RawHandle().isNull() && cfg.RawHandle().isMember("WeaponCurves")) {
        auto &node = cfg.RawHandle()["WeaponCurves"];
        if (node.isMember("Damage") && node["Damage"].isMember("points")) {
            weapon_damage_curve.clear();
            for (auto &pt : node["Damage"]["points"]) {
                if (pt.isArray() && pt.size() == 2) {
                    CurvePoint cp{pt[0].asInt(), pt[1].asDouble()};
                    weapon_damage_curve.push_back(cp);
                }
            }
            std::sort(weapon_damage_curve.begin(), weapon_damage_curve.end(), [](const CurvePoint&a,const CurvePoint&b){ return a.level < b.level; });
        }
        if (node.isMember("Attack") && node["Attack"].isMember("points")) {
            weapon_attack_curve.clear();
            for (auto &pt : node["Attack"]["points"]) {
                if (pt.isArray() && pt.size() == 2) {
                    CurvePoint cp{pt[0].asInt(), pt[1].asDouble()};
                    weapon_attack_curve.push_back(cp);
                }
            }
            std::sort(weapon_attack_curve.begin(), weapon_attack_curve.end(), [](const CurvePoint&a,const CurvePoint&b){ return a.level < b.level; });
        }
    }

    // Mod2 curves
    if (!cfg.RawHandle().isNull() && cfg.RawHandle().isMember("Mod2Curves")) {
        auto &node = cfg.RawHandle()["Mod2Curves"];
        for (auto it = node.begin(); it != node.end(); ++it) {
            std::string key = it.key().asString();
            auto &mapNode = *it;
            if (mapNode.isMember("points")) {
                std::vector<CurvePoint> pts;
                for (auto &pt : mapNode["points"]) {
                    if (pt.isArray() && pt.size() == 2) {
                        pts.push_back(CurvePoint{pt[0].asInt(), pt[1].asDouble()});
                    }
                }
                std::sort(pts.begin(), pts.end(), [](const CurvePoint&a,const CurvePoint&b){ return a.level < b.level; });
                mod2_curves[key] = pts;
            }
        }
    }

    // Mod2CurvesExtras - additional mod2 stats
    if (!cfg.RawHandle().isNull() && cfg.RawHandle().isMember("Mod2CurvesExtras")) {
        auto &node = cfg.RawHandle()["Mod2CurvesExtras"];
        for (auto it = node.begin(); it != node.end(); ++it) {
            std::string key = it.key().asString();
            auto &n = *it;
            if (n.isMember("points")) {
                std::vector<CurvePoint> pts;
                for (auto &pt : n["points"]) {
                    if (pt.isArray() && pt.size() == 2) {
                        pts.push_back(CurvePoint{pt[0].asInt(), pt[1].asDouble()});
                    }
                }
                std::sort(pts.begin(), pts.end(), [](const CurvePoint&a,const CurvePoint&b){ return a.level < b.level; });
                mod2_curves[key] = pts;
            }
        }
    }

    // PrimaryCurves (hp, ac, mana, endur) add to mod2_curves for easy reuse
    if (!cfg.RawHandle().isNull() && cfg.RawHandle().isMember("PrimaryCurves")) {
        auto &node = cfg.RawHandle()["PrimaryCurves"];
        for (auto it = node.begin(); it != node.end(); ++it) {
            std::string key = it.key().asString();
            auto &n = *it;
            if (n.isMember("points")) {
                std::vector<CurvePoint> pts;
                for (auto &pt : n["points"]) {
                    if (pt.isArray() && pt.size() == 2) {
                        pts.push_back(CurvePoint{pt[0].asInt(), pt[1].asDouble()});
                    }
                }
                std::sort(pts.begin(), pts.end(), [](const CurvePoint&a,const CurvePoint&b){ return a.level < b.level; });
                mod2_curves[key] = pts;
            }
        }
    }

    // Attribute pool config
    if (!cfg.RawHandle().isNull() && cfg.RawHandle().isMember("AttributePool")) {
        auto &node = cfg.RawHandle()["AttributePool"];
        try {
            if (node.isMember("mode")) attribute_budget_mode = node["mode"].asString();
            if (node.isMember("static_budget")) attribute_static_budget = node["static_budget"].asInt();
        } catch (...) {}
    }

    // Slot multipliers: support either numeric (for 'Attributes') or object mapping statKey->value
    if (!cfg.RawHandle().isNull() && cfg.RawHandle().isMember("SlotMultipliers")) {
        auto &node = cfg.RawHandle()["SlotMultipliers"];
        for (auto it = node.begin(); it != node.end(); ++it) {
            std::string key = it.key().asString();
            try {
                if ((*it).isNumeric()) {
                    // legacy: numeric value represents Attributes multiplier
                    slot_multipliers_by_stat[key]["Attributes"] = (*it).asDouble();
                } else if ((*it).isObject()) {
                    for (auto jt = (*it).begin(); jt != (*it).end(); ++jt) {
                        std::string statKey = jt.key().asString();
                        try {
                            double val = jt->asDouble();
                            slot_multipliers_by_stat[key][statKey] = val;
                        } catch (...) {}
                    }
                }
            } catch (...) {}
        }
    }

    // Click-cast time reduction curve
    if (!cfg.RawHandle().isNull() && cfg.RawHandle().isMember("ClickCastTimeReduction")) {
        auto &node = cfg.RawHandle()["ClickCastTimeReduction"];
        try {
            if (node.isMember("points")) {
                click_cast_reduction_curve.clear();
                for (auto &pt : node["points"]) {
                    if (pt.isArray() && pt.size() == 2) {
                        click_cast_reduction_curve.push_back(CurvePoint{pt[0].asInt(), pt[1].asDouble()});
                    }
                }
                std::sort(click_cast_reduction_curve.begin(), click_cast_reduction_curve.end(),
                    [](const CurvePoint&a, const CurvePoint&b){ return a.level < b.level; });
            }
        } catch (...) {}
    }

    // Write a debug entry in logs/inf
    std::ofstream logfile("logs/inf/item_scaling_config.log", std::ios::app);
    if (logfile.is_open()) {
                logfile << "SlotMultipliers:" << std::endl;
                for (auto &slotKv : slot_multipliers_by_stat) {
                    logfile << "  Slot: " << slotKv.first << std::endl;
                    for (auto &kv2 : slotKv.second) {
                        logfile << "    " << kv2.first << " = " << kv2.second << std::endl;
                    }
                }
        logfile << "ItemScaling::Config loaded from " << file << " SpellDmg mode=" << spell_dmg_from_int_mode << " divisor=" << spell_dmg_from_int_divisor << " points=" << spell_dmg_from_int_curve.size() << " Heal mode=" << heal_from_wis_mode << " divisor=" << heal_from_wis_divisor << " points=" << heal_from_wis_curve.size() << std::endl;
        logfile << "AttributePreferences:" << std::endl;
        logfile << "  AStr presence=" << attr_AStr.presence_mult << " absence=" << attr_AStr.absence_mult << std::endl;
        logfile << "  ASta presence=" << attr_ASta.presence_mult << " absence=" << attr_ASta.absence_mult << std::endl;
        logfile << "  ADex presence=" << attr_ADex.presence_mult << " absence=" << attr_ADex.absence_mult << std::endl;
        logfile << "  AAgi presence=" << attr_AAgi.presence_mult << " absence=" << attr_AAgi.absence_mult << std::endl;
        logfile << "  AInt presence=" << attr_AInt.presence_mult << " absence=" << attr_AInt.absence_mult << std::endl;
        logfile << "  AWis presence=" << attr_AWis.presence_mult << " absence=" << attr_AWis.absence_mult << std::endl;
        logfile << "  ACha presence=" << attr_ACha.presence_mult << " absence=" << attr_ACha.absence_mult << std::endl;
            logfile << "AttributePool mode=" << cfg.RawHandle()["AttributePool"].get("mode", "auto").asString() << " static_budget=" << cfg.RawHandle()["AttributePool"].get("static_budget", 0).asInt() << std::endl;
        // No class multipliers - this project uses AAs or specialized rules for class-specific tuning
        logfile.close();
    }
}

double Config::EvaluateCurve(const std::vector<CurvePoint> &curve, int level) const {
    if (curve.empty()) return 1.0; // default scale factor (per 10 attr)
    if (level <= curve.front().level) return curve.front().value;
    if (level >= curve.back().level) return curve.back().value;
    // linear interpolate between the two surrounding points
    for (size_t i = 0; i + 1 < curve.size(); ++i) {
        if (level >= curve[i].level && level <= curve[i+1].level) {
            double a = curve[i].value;
            double b = curve[i+1].value;
            double t = double(level - curve[i].level) / double(curve[i+1].level - curve[i].level);
            return a + (b - a) * t;
        }
    }
    return curve.back().value; // fallback
}

int Config::ComputeSpellDmgFromInt(int attrValue, int level) const {
    if (!spell_dmg_from_int_enabled || attrValue <= 0) return 0;
    if (spell_dmg_from_int_mode == "divisor") {
        return attrValue / spell_dmg_from_int_divisor; // integer division
    }
    // mode: curve -- curve values represent derived amount per 10 attribute points
    double per10 = EvaluateCurve(spell_dmg_from_int_curve, level);
    return static_cast<int>(std::floor((attrValue * per10) / 10.0));
}

int Config::ComputeHealFromWis(int attrValue, int level) const {
    if (!heal_from_wis_enabled || attrValue <= 0) return 0;
    if (heal_from_wis_mode == "divisor") {
        return attrValue / heal_from_wis_divisor;
    }
    double per10 = EvaluateCurve(heal_from_wis_curve, level);
    return static_cast<int>(std::floor((attrValue * per10) / 10.0));
}

double Config::GetAttributePresenceMultiplier(const std::string &attrName, bool present, int level) const {
    const AttributeConfig* ac = nullptr;
    if (attrName == "AStr") ac = &attr_AStr;
    else if (attrName == "ASta") ac = &attr_ASta;
    else if (attrName == "AAgi") ac = &attr_AAgi;
    else if (attrName == "ADex") ac = &attr_ADex;
    else if (attrName == "AInt") ac = &attr_AInt;
    else if (attrName == "AWis") ac = &attr_AWis;
    else if (attrName == "ACha") ac = &attr_ACha;
    if (!ac) return 1.0;
    double mult = present ? ac->presence_mult : ac->absence_mult;
    if (!ac->curve.empty()) {
        double curveVal = EvaluateCurve(ac->curve, level);
        mult = mult * curveVal;
    }
    return mult;
}

double Config::GetClassMultiplier(const std::string &className, const std::string &statKey) const {
    // Deprecated - we no longer use class multipliers. Provide 1.0 as a neutral multiplier.
    (void) className;
    (void) statKey;
    return 1.0;
}

double Config::GetGlobalAttrCurve(int level) const {
    return EvaluateCurve(global_attribute_curve, level);
}

double Config::GetWeaponDamageCurve(int level) const {
    return EvaluateCurve(weapon_damage_curve, level);
}

double Config::GetWeaponAttackCurve(int level) const {
    return EvaluateCurve(weapon_attack_curve, level);
}

double Config::GetMod2Curve(const std::string &mod2Key, int level) const {
    auto it = mod2_curves.find(mod2Key);
    if (it == mod2_curves.end()) return 1.0;
    return EvaluateCurve(it->second, level);
}

double Config::GetSlotMultiplierByName(const std::string &slotName, const std::string &statKey) const {
    auto it = slot_multipliers_by_stat.find(slotName);
    if (it == slot_multipliers_by_stat.end()) return 1.0;
    auto it2 = it->second.find(statKey);
    if (it2 == it->second.end()) {
        // Fallback: if statKey-specific multiplier not found, return Attributes key if set, else 1.0.
        auto it3 = it->second.find("Attributes");
        if (it3 != it->second.end()) return it3->second;
        return 1.0;
    }
    return it2->second;
}

double Config::GetSlotMultiplierByMask(uint32_t slotMask, const std::string &statKey) const {
    // Priority list: Chest, Head, Legs, Arms, Hands, Feet, Primary, Secondary, Range, Wrist, Back, Neck, Ears, Ring, Face
    using namespace EQ::invslot;
    struct S { int16 slot; const char* name; } pri[] = {
        { slotChest, "Chest" },
        { slotHead, "Head" },
        { slotLegs, "Legs" },
        { slotArms, "Arms" },
        { slotHands, "Hands" },
        { slotFeet, "Feet" },
        { slotPrimary, "Primary" },
        { slotSecondary, "Secondary" },
        { slotRange, "Range" },
        { slotWrist1, "Wrist" },
        { slotBack, "Back" },
        { slotNeck, "Neck" },
        { slotEar1, "Ear" },
        { slotEar2, "Ear" },
        { slotFinger1, "Ring" },
        { slotFinger2, "Ring" },
        { slotFace, "Face" },
    };
    for (auto &s : pri) {
        if (slotMask & (1u << s.slot)) {
            return GetSlotMultiplierByName(s.name, statKey);
        }
    }
    return 1.0;
}

double Config::GetClickCastReductionFraction(int level) const {
    if (click_cast_reduction_curve.empty()) {
        // Fallback linear formula: 0.67% reduction per level, capped at 100%
        double fraction = std::min(1.0, level * 0.0067);
        return fraction;
    }
    // Clamp returned value to [0.0, 1.0]
    double v = EvaluateCurve(click_cast_reduction_curve, level);
    return std::max(0.0, std::min(1.0, v));
}

} // namespace ItemScaling
