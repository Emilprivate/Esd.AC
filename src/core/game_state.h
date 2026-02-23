#pragma once

#include <cstdint>
#include "core/math/geom.h"

namespace Offsets {
    // Runtime offsets loaded from config.json
    inline uintptr_t LocalPlayer = 0;
    inline uintptr_t LocalPlayerAlt = 0;
    inline uintptr_t EntityList = 0;
    inline uintptr_t Fov = 0;
    inline uintptr_t ViewMatrix = 0;
    inline uintptr_t PlayerCount = 0;
    inline uintptr_t Team = 0;
}

class Player {
public:
    char pad_0000[4];
    float headX;
    float headY;
    float headZ;
    char pad_0010[24];
    float posZ;
    float posX;
    float posY;
    float camPitch;
    float camYaw;
    char pad_003C[4];
    float viewYaw;
    float viewPitch;
    char pad_0048[164];
    int32_t health;
    int32_t armor;
    char pad_00F4[273];
    char name[16];
    char pad_0215[283];

    Vector3 GetHeadPos() const {
        return {headX, headY, headZ};
    }

    Vector3 GetPos() const {
        return {posX, posY, posZ};
    }

    Vector3 GetViewAngles() const {
        return {camYaw, camPitch, 0.0f};
    }

    Vector3 GetViewAnglesSwapped() const {
        return {camPitch, camYaw, 0.0f};
    }

    void SetViewAngles(const Vector3& angles) {
        camYaw = angles.x;
        camPitch = angles.y;
    }

    void SetViewAnglesSwapped(const Vector3& angles) {
        camPitch = angles.x;
        camYaw = angles.y;
    }

    bool IsValid() const {
        return (health > 0 && health <= 100);
    }
};

namespace Settings {
    inline bool bAimbot = false;
    inline bool bESP = false;
    inline bool bSnaplines = false;
    inline bool espShowTeammates = false;
    inline bool espShowEnemies = true;
    inline bool aimbotTargetTeammates = false;
    inline bool aimbotTargetEnemies = true;
    inline int aimbotPriorityMode = 0; // 0=crosshair, 1=distance, 2=lowest HP
    inline float aimbotFOV = 10.0f;
    inline float aimbotSmooth = 1.0f;
}

namespace DebugState {
    inline uintptr_t moduleBase = 0;
    inline uintptr_t localPlayerPtr = 0;
    inline uintptr_t localPlayerOffsetUsed = 0;
    inline uintptr_t entityListBase = 0;
    inline uintptr_t viewMatrixAddr = 0;
    inline uintptr_t viewMatrixCandidateRelative = 0;
    inline uintptr_t viewMatrixCandidateAbsolute = 0;
    inline bool viewMatrixRelativeReadable = false;
    inline bool viewMatrixAbsoluteReadable = false;
    inline int playerCount = 0;
    inline int entitiesScanned = 0;
    inline int entitiesValid = 0;
    inline int entitiesW2S = 0;
    inline bool viewMatrixLooksValid = false;
    inline bool usingAngleProjection = false;
    inline bool usingSwappedCameraAxes = false;
    inline bool teamDataAvailable = false;
    inline int localTeamId = -1;
    inline int entitiesEspFiltered = 0;
    inline int entitiesAimbotFiltered = 0;
    inline float gameFov = 90.0f;
    inline float bestTargetFov = -1.0f;
}