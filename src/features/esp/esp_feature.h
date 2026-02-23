#pragma once

#include "core/game_state.h"

#include <string>

namespace EspFeature {
    bool TryDrawEntityEsp(
        const Player* entity,
        const float* viewMatrix,
        int screenWidth,
        int screenHeight,
        bool isTeammate,
        bool hasTeamData,
        int entityHealth,
        int entityArmor,
        const std::string& entityName,
        float entityDistance,
        bool showName,
        bool showHealth,
        bool showArmor,
        bool showDistance,
        bool showHealthValue,
        bool showArmorValue,
        bool useTeamColors,
        int infoPosition,
        bool boxFilled,
        float boxThickness,
        float boxFillAlpha,
        bool drawSnaplines,
        int& entitiesW2S);
}