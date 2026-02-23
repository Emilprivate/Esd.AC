#pragma once

#include "core/game_state.h"

namespace EspFeature {
    bool TryDrawEntityEsp(
        const Player* entity,
        const float* viewMatrix,
        int screenWidth,
        int screenHeight,
        bool drawSnaplines,
        int& entitiesW2S);
}