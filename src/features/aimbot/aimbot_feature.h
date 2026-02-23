#pragma once

#include "core/game_state.h"

namespace AimbotFeature {
    enum class TargetPriority : int {
        NearestCrosshair = 0,
        NearestDistance = 1,
        LowestHP = 2
    };

    struct Selection {
        Player* bestTarget = nullptr;
        Vector3 bestAngle{};
        bool useSwappedAxes = false;
        float bestFov = 0.0f;
    };

    void ConsiderEntityTarget(
        const Player* localPlayer,
        Player* entity,
        Player* lockedTarget,
        TargetPriority priority,
        float maxFov,
        float& bestScore,
        Selection& selection);

    void ApplyAim(Player* localPlayer, const Selection& selection, float smoothAmount);
}