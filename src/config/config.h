#pragma once

#include <string>

namespace AppConfig {
    inline std::string menuTitle = "AssaultCube Hack";
    inline int menuToggleKey = 0x2D;
    inline int unloadPrimaryKey = 0x2E;
    inline int unloadSecondaryKey = 0x23;

    bool LoadConfigFromDisk();
    const std::string& GetLoadedConfigPath();
}