#include "features/aimbot/aimbot_feature.h"

#include <algorithm>

namespace {
    constexpr float kStickyTargetFovMultiplier = 0.8f;
    constexpr float kChestOffsetFromHead = 1.35f;
    constexpr float kPelvisOffsetFromHead = 2.70f;

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

    Vector3 GetTargetBonePosition(const Player* entity, int targetBoneMode) {
        if (!entity) {
            return {};
        }

        const Vector3 head = entity->GetHeadPos();

        switch (targetBoneMode) {
            case 1: {
                return { head.x, head.y, head.z - kChestOffsetFromHead };
            }
            case 2:
                return { head.x, head.y, head.z - kPelvisOffsetFromHead };
            case 0:
            default:
                return head;
        }
    }
}

void AimbotFeature::ConsiderEntityTarget(
    const Player* localPlayer,
    Player* entity,
    Player* lockedTarget,
    int targetBoneMode,
    int entityHealth,
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
    const Vector3 targetBonePos = GetTargetBonePosition(entity, targetBoneMode);

    const Vector3 targetAngle = CalcAngle(localHeadPos, targetBonePos);
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
            score = localHeadPos.Distance(targetBonePos);
            break;
        }
        case TargetPriority::LowestHP: {
            score = static_cast<float>(entityHealth);
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