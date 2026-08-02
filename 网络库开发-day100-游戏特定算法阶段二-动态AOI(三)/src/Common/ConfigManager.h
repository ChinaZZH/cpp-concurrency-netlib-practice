#pragma once

#include <string>
#include <vector>
#include "SimpleIni.h"

class ConfigManager {
public:
    static ConfigManager& getInstance() {
        static ConfigManager instance;
        return instance;
    }

    bool loadConfig(const std::string& filename) {
        filename_ = filename;
        ini_.Reset();
        SI_Error rc = ini_.LoadFile(filename_.c_str());
        if (rc != SI_OK) return false;
        loaded_ = true;
        return true;
    }

    std::string getString(const std::string& section, const std::string& key, const std::string& defaultValue = "") const {
        if (!loaded_) return defaultValue;
        const char* val = ini_.GetValue(section.c_str(), key.c_str(), defaultValue.c_str());
        return val ? val : defaultValue;
    }

    int getInt(const std::string& section, const std::string& key, int defaultValue = 0) const {
        if (!loaded_) return defaultValue;
        return (int)ini_.GetLongValue(section.c_str(), key.c_str(), defaultValue);
    }

    long getLong(const std::string& section, const std::string& key, long defaultValue = 0) const {
        if (!loaded_) return defaultValue;
        return ini_.GetLongValue(section.c_str(), key.c_str(), defaultValue);
    }

    double getDouble(const std::string& section, const std::string& key, double defaultValue = 0.0) const {
        if (!loaded_) return defaultValue;
        return ini_.GetDoubleValue(section.c_str(), key.c_str(), defaultValue);
    }

    bool getBool(const std::string& section, const std::string& key, bool defaultValue = false) const {
        if (!loaded_) return defaultValue;
        return ini_.GetBoolValue(section.c_str(), key.c_str(), defaultValue);
    }

    bool exists(const std::string& section, const std::string& key) const {
        if (!loaded_) return false;
        const char* val = ini_.GetValue(section.c_str(), key.c_str(), nullptr);
        return val != nullptr;
    }

    std::vector<std::string> getSections() const {
        std::vector<std::string> result;
        if (!loaded_) return result;
        CSimpleIniA::TNamesDepend sections;
        ini_.GetAllSections(sections);
        for (auto& sec : sections) result.emplace_back(sec.pItem);
        return result;
    }

    std::vector<std::string> getKeys(const std::string& section) const {
        std::vector<std::string> result;
        if (!loaded_) return result;
        CSimpleIniA::TNamesDepend keys;
        ini_.GetAllKeys(section.c_str(), keys);
        for (auto& key : keys) result.emplace_back(key.pItem);
        return result;
    }

private:
    ConfigManager() : loaded_(false) {}
    ~ConfigManager() = default;
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    CSimpleIniA ini_;
    std::string filename_;
    bool loaded_;
};