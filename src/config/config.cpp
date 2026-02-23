#include "config/config.h"

#include "core/game_state.h"

#include <windows.h>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <string>

namespace {
    std::string g_loadedConfigPath;

    std::string ReadWholeFile(const std::string& path) {
        std::ifstream file(path, std::ios::in | std::ios::binary);
        if (!file) {
            return {};
        }

        file.seekg(0, std::ios::end);
        const std::streamoff size = file.tellg();
        if (size <= 0) {
            return {};
        }

        std::string out;
        out.resize(static_cast<size_t>(size));
        file.seekg(0, std::ios::beg);
        file.read(out.data(), size);
        return out;
    }

    std::string GetModuleDirectory(HMODULE moduleHandle) {
        char pathBuffer[MAX_PATH]{};
        const DWORD pathLen = GetModuleFileNameA(moduleHandle, pathBuffer, MAX_PATH);
        if (pathLen == 0 || pathLen >= MAX_PATH) {
            return {};
        }

        std::string path(pathBuffer, pathLen);
        const size_t slashPos = path.find_last_of("\\/");
        if (slashPos == std::string::npos) {
            return {};
        }

        return path.substr(0, slashPos);
    }

    bool ExtractRawValue(const std::string& json, const std::string& key, std::string& outRaw) {
        const std::string quotedKey = "\"" + key + "\"";
        size_t keyPos = json.find(quotedKey);
        if (keyPos == std::string::npos) {
            return false;
        }

        size_t colonPos = json.find(':', keyPos + quotedKey.size());
        if (colonPos == std::string::npos) {
            return false;
        }

        size_t valueStart = colonPos + 1;
        while (valueStart < json.size() && std::isspace(static_cast<unsigned char>(json[valueStart]))) {
            ++valueStart;
        }

        if (valueStart >= json.size()) {
            return false;
        }

        if (json[valueStart] == '"') {
            size_t valueEnd = valueStart + 1;
            while (valueEnd < json.size()) {
                if (json[valueEnd] == '"' && json[valueEnd - 1] != '\\') {
                    break;
                }
                ++valueEnd;
            }

            if (valueEnd >= json.size()) {
                return false;
            }

            outRaw = json.substr(valueStart + 1, valueEnd - valueStart - 1);
            return true;
        }

        size_t valueEnd = valueStart;
        while (valueEnd < json.size() && json[valueEnd] != ',' && json[valueEnd] != '}' && json[valueEnd] != ']') {
            ++valueEnd;
        }

        outRaw = json.substr(valueStart, valueEnd - valueStart);
        outRaw.erase(std::remove_if(outRaw.begin(), outRaw.end(), [](unsigned char c) { return std::isspace(c); }), outRaw.end());
        return !outRaw.empty();
    }

    bool ExtractSection(const std::string& json, const std::string& sectionName, std::string& outSection) {
        const std::string quotedKey = "\"" + sectionName + "\"";
        size_t keyPos = json.find(quotedKey);
        if (keyPos == std::string::npos) {
            return false;
        }

        size_t colonPos = json.find(':', keyPos + quotedKey.size());
        if (colonPos == std::string::npos) {
            return false;
        }

        size_t sectionStart = json.find('{', colonPos);
        if (sectionStart == std::string::npos) {
            return false;
        }

        int depth = 0;
        size_t i = sectionStart;
        for (; i < json.size(); ++i) {
            if (json[i] == '{') {
                ++depth;
            } else if (json[i] == '}') {
                --depth;
                if (depth == 0) {
                    break;
                }
            }
        }

        if (i >= json.size()) {
            return false;
        }

        outSection = json.substr(sectionStart, i - sectionStart + 1);
        return true;
    }

    bool ParseBool(const std::string& raw, bool& out) {
        if (raw == "true") {
            out = true;
            return true;
        }
        if (raw == "false") {
            out = false;
            return true;
        }
        return false;
    }

    bool ParseFloat(const std::string& raw, float& out) {
        char* end = nullptr;
        out = std::strtof(raw.c_str(), &end);
        return end != raw.c_str() && *end == '\0';
    }

    bool ParseInt(const std::string& raw, int& out) {
        char* end = nullptr;
        const long parsed = std::strtol(raw.c_str(), &end, 0);
        if (end == raw.c_str() || *end != '\0') {
            return false;
        }

        out = static_cast<int>(parsed);
        return true;
    }

    bool ParseUintPtr(const std::string& raw, uintptr_t& out) {
        char* end = nullptr;
        const unsigned long long parsed = std::strtoull(raw.c_str(), &end, 0);
        if (end == raw.c_str() || *end != '\0') {
            return false;
        }

        out = static_cast<uintptr_t>(parsed);
        return true;
    }

    template <typename TParser, typename TValue>
    void TryApplyValue(const std::string& json, const std::string& key, TParser parser, TValue& destination) {
        std::string raw;
        if (!ExtractRawValue(json, key, raw)) {
            return;
        }

        TValue parsed{};
        if (parser(raw, parsed)) {
            destination = parsed;
        }
    }

    void ApplyConfig(const std::string& json) {
        std::string title;
        if (ExtractRawValue(json, "menuTitle", title) && !title.empty()) {
            AppConfig::menuTitle = title;
        }

        std::string section;

        if (ExtractSection(json, "settings", section)) {
            TryApplyValue(section, "bAimbot", ParseBool, Settings::bAimbot);
            TryApplyValue(section, "bESP", ParseBool, Settings::bESP);
            TryApplyValue(section, "bSnaplines", ParseBool, Settings::bSnaplines);
            TryApplyValue(section, "espShowTeammates", ParseBool, Settings::espShowTeammates);
            TryApplyValue(section, "espShowEnemies", ParseBool, Settings::espShowEnemies);
            TryApplyValue(section, "espShowName", ParseBool, Settings::espShowName);
            TryApplyValue(section, "espShowHealth", ParseBool, Settings::espShowHealth);
            TryApplyValue(section, "espShowArmor", ParseBool, Settings::espShowArmor);
            TryApplyValue(section, "espUseTeamColors", ParseBool, Settings::espUseTeamColors);
            TryApplyValue(section, "espInfoPosition", ParseInt, Settings::espInfoPosition);
            TryApplyValue(section, "espShowDistance", ParseBool, Settings::espShowDistance);
            TryApplyValue(section, "espShowHealthValue", ParseBool, Settings::espShowHealthValue);
            TryApplyValue(section, "espShowArmorValue", ParseBool, Settings::espShowArmorValue);
            TryApplyValue(section, "espBoxFilled", ParseBool, Settings::espBoxFilled);
            TryApplyValue(section, "espBoxThickness", ParseFloat, Settings::espBoxThickness);
            TryApplyValue(section, "espBoxFillAlpha", ParseFloat, Settings::espBoxFillAlpha);
            TryApplyValue(section, "aimbotTargetTeammates", ParseBool, Settings::aimbotTargetTeammates);
            TryApplyValue(section, "aimbotTargetEnemies", ParseBool, Settings::aimbotTargetEnemies);
            TryApplyValue(section, "aimbotPriorityMode", ParseInt, Settings::aimbotPriorityMode);
            TryApplyValue(section, "aimbotTargetBone", ParseInt, Settings::aimbotTargetBone);
            TryApplyValue(section, "aimbotFOV", ParseFloat, Settings::aimbotFOV);
            TryApplyValue(section, "aimbotSmooth", ParseFloat, Settings::aimbotSmooth);

            if (Settings::aimbotPriorityMode < 0 || Settings::aimbotPriorityMode > 2) {
                Settings::aimbotPriorityMode = 0;
            }

            if (Settings::aimbotTargetBone < 0 || Settings::aimbotTargetBone > 2) {
                Settings::aimbotTargetBone = 0;
            }

            if (Settings::espInfoPosition < 0 || Settings::espInfoPosition > 2) {
                Settings::espInfoPosition = 0;
            }
            if (Settings::espBoxThickness < 0.5f) {
                Settings::espBoxThickness = 0.5f;
            }
            if (Settings::espBoxThickness > 6.0f) {
                Settings::espBoxThickness = 6.0f;
            }
            if (Settings::espBoxFillAlpha < 0.0f) {
                Settings::espBoxFillAlpha = 0.0f;
            }
            if (Settings::espBoxFillAlpha > 0.9f) {
                Settings::espBoxFillAlpha = 0.9f;
            }
        }

        if (ExtractSection(json, "hotkeys", section)) {
            TryApplyValue(section, "menuToggleKey", ParseInt, AppConfig::menuToggleKey);
            TryApplyValue(section, "unloadPrimaryKey", ParseInt, AppConfig::unloadPrimaryKey);
            TryApplyValue(section, "unloadSecondaryKey", ParseInt, AppConfig::unloadSecondaryKey);
        }

        // Source of truth for all runtime offsets
        if (ExtractSection(json, "offsets", section)) {
            TryApplyValue(section, "LocalPlayer", ParseUintPtr, Offsets::LocalPlayer);
            TryApplyValue(section, "LocalPlayerAlt", ParseUintPtr, Offsets::LocalPlayerAlt);
            TryApplyValue(section, "EntityList", ParseUintPtr, Offsets::EntityList);
            TryApplyValue(section, "Fov", ParseUintPtr, Offsets::Fov);
            TryApplyValue(section, "ViewMatrix", ParseUintPtr, Offsets::ViewMatrix);
            TryApplyValue(section, "PlayerCount", ParseUintPtr, Offsets::PlayerCount);
            TryApplyValue(section, "Team", ParseUintPtr, Offsets::Team);
            TryApplyValue(section, "CameraX", ParseUintPtr, Offsets::CameraX);
            TryApplyValue(section, "CameraY", ParseUintPtr, Offsets::CameraY);
            TryApplyValue(section, "Health", ParseUintPtr, Offsets::Health);
            TryApplyValue(section, "Armor", ParseUintPtr, Offsets::Armor);
            TryApplyValue(section, "Name", ParseUintPtr, Offsets::Name);
            TryApplyValue(section, "AmmoPistol", ParseUintPtr, Offsets::AmmoPistol);
            TryApplyValue(section, "AmmoShotgun", ParseUintPtr, Offsets::AmmoShotgun);
            TryApplyValue(section, "AmmoSubmachine", ParseUintPtr, Offsets::AmmoSubmachine);
            TryApplyValue(section, "AmmoSniper", ParseUintPtr, Offsets::AmmoSniper);
            TryApplyValue(section, "AmmoAssaultRifle", ParseUintPtr, Offsets::AmmoAssaultRifle);
            TryApplyValue(section, "AmmoGrenade", ParseUintPtr, Offsets::AmmoGrenade);
            TryApplyValue(section, "FastFireShotgun", ParseUintPtr, Offsets::FastFireShotgun);
            TryApplyValue(section, "FastFireSniper", ParseUintPtr, Offsets::FastFireSniper);
            TryApplyValue(section, "FastFireAssaultRifle", ParseUintPtr, Offsets::FastFireAssaultRifle);
            TryApplyValue(section, "AutoShoot", ParseUintPtr, Offsets::AutoShoot);
        }
    }
}

bool AppConfig::LoadConfigFromDisk() {
    HMODULE thisModule = nullptr;
    if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCSTR>(&AppConfig::LoadConfigFromDisk),
        &thisModule)) {
        return false;
    }

    const std::string moduleDir = GetModuleDirectory(thisModule);
    if (moduleDir.empty()) {
        return false;
    }

    const std::string configPath = moduleDir + "\\config.json";
    const std::string json = ReadWholeFile(configPath);
    if (json.empty()) {
        return false;
    }

    ApplyConfig(json);
    g_loadedConfigPath = configPath;
    return true;
}

const std::string& AppConfig::GetLoadedConfigPath() {
    return g_loadedConfigPath;
}