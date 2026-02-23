#include "features/aimbot/aimbot_feature.h"

namespace {
    constexpr float kStickyTargetFovMultiplier = 0.8f;

    float NormalizeYaw(float yaw) {
        while (yaw > 180.0f) yaw -= 360.0f;
        while (yaw < -180.0f) yaw += 360.0f;
        return yaw;
    }

    float AngleDistance(const Vector3& current, const Vector3& target) {
        const float deltaYaw = NormalizeYaw(target.x - current.x);
        const float deltaPitch = target.y - current.y;
        return std::sqrt(deltaYaw * deltaYaw + deltaPitch * deltaPitch);
    }
}

void AimbotFeature::ConsiderEntityTarget(
    const Player* localPlayer,
    Player* entity,
    Player* lockedTarget,
    TargetPriority priority,
    float maxFov,
    float& bestScore,
    Selection& selection) {

    if (!localPlayer || !entity) {
        return;
    }

    const Vector3 localHeadPos = localPlayer->GetHeadPos();
    const Vector3 localViewAnglesStandard = localPlayer->GetViewAngles();
    const Vector3 localViewAnglesSwapped = localPlayer->GetViewAnglesSwapped();
    const Vector3 entityHeadPos = entity->GetHeadPos();

    const Vector3 targetAngle = CalcAngle(localHeadPos, entityHeadPos);
    const float fovStandard = AngleDistance(localViewAnglesStandard, targetAngle);
    const float fovSwapped = AngleDistance(localViewAnglesSwapped, targetAngle);
    const bool useSwapped = (fovSwapped < fovStandard);
    float fov = useSwapped ? fovSwapped : fovStandard;

    if (fov > maxFov) {
        return;
    }

    float score = fov;
    switch (priority) {
        case TargetPriority::NearestDistance: {
            score = localHeadPos.Distance(entityHeadPos);
            break;
        }
        case TargetPriority::LowestHP: {
            score = static_cast<float>(entity->health);
            break;
        }
        case TargetPriority::NearestCrosshair:
        default:
            score = fov;
            break;
    }

    if (entity == lockedTarget) {
        score *= kStickyTargetFovMultiplier;
    }

    if (score < bestScore) {
        bestScore = score;
        selection.bestTarget = entity;
        selection.bestAngle = targetAngle;
        selection.useSwappedAxes = useSwapped;
        selection.bestFov = fov;
    }
}

void AimbotFeature::ApplyAim(Player* localPlayer, const Selection& selection, float smoothAmount) {
    if (!localPlayer || !selection.bestTarget) {
        return;
    }

    Vector3 angle = selection.bestAngle;
    const Vector3 currentAngles = selection.useSwappedAxes
        ? localPlayer->GetViewAnglesSwapped()
        : localPlayer->GetViewAngles();

    if (smoothAmount > 1.0f) {
        Vector3 delta = angle - currentAngles;
        if (delta.x > 180.0f) delta.x -= 360.0f;
        if (delta.x < -180.0f) delta.x += 360.0f;
        angle = currentAngles + (delta / smoothAmount);
    }

    if (selection.useSwappedAxes) {
        localPlayer->SetViewAnglesSwapped(angle);
    } else {
        localPlayer->SetViewAngles(angle);
    }
}