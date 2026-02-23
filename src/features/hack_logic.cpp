#include "features/hack_logic.h"

#include "core/game_state.h"
#include "features/aimbot/aimbot_feature.h"
#include "features/esp/esp_feature.h"

#include "imgui.h"
#include <windows.h>
#include <string>

namespace {
    constexpr int kMaxPlayers = 32;

    bool IsReadableAddress(uintptr_t address, size_t size) {
        if (address == 0 || size == 0) {
            return false;
        }

        MEMORY_BASIC_INFORMATION mbi{};
        if (!VirtualQuery(reinterpret_cast<LPCVOID>(address), &mbi, sizeof(mbi))) {
            return false;
        }

        if (mbi.State != MEM_COMMIT) {
            return false;
        }

        if ((mbi.Protect & PAGE_GUARD) || (mbi.Protect & PAGE_NOACCESS)) {
            return false;
        }

        const bool readable =
            (mbi.Protect & PAGE_READONLY) ||
            (mbi.Protect & PAGE_READWRITE) ||
            (mbi.Protect & PAGE_WRITECOPY) ||
            (mbi.Protect & PAGE_EXECUTE_READ) ||
            (mbi.Protect & PAGE_EXECUTE_READWRITE) ||
            (mbi.Protect & PAGE_EXECUTE_WRITECOPY);

        if (!readable) {
            return false;
        }

        const uintptr_t regionEnd = reinterpret_cast<uintptr_t>(mbi.BaseAddress) + mbi.RegionSize;
        return (address + size) <= regionEnd;
    }

    bool IsWritableAddress(uintptr_t address, size_t size) {
        if (address == 0 || size == 0) {
            return false;
        }

        MEMORY_BASIC_INFORMATION mbi{};
        if (!VirtualQuery(reinterpret_cast<LPCVOID>(address), &mbi, sizeof(mbi))) {
            return false;
        }

        if (mbi.State != MEM_COMMIT) {
            return false;
        }

        if ((mbi.Protect & PAGE_GUARD) || (mbi.Protect & PAGE_NOACCESS)) {
            return false;
        }

        const bool writable =
            (mbi.Protect & PAGE_READWRITE) ||
            (mbi.Protect & PAGE_WRITECOPY) ||
            (mbi.Protect & PAGE_EXECUTE_READWRITE) ||
            (mbi.Protect & PAGE_EXECUTE_WRITECOPY);

        if (!writable) {
            return false;
        }

        const uintptr_t regionEnd = reinterpret_cast<uintptr_t>(mbi.BaseAddress) + mbi.RegionSize;
        return (address + size) <= regionEnd;
    }

    template <typename T>
    bool SafeRead(uintptr_t address, T& outValue) {
        if (!IsReadableAddress(address, sizeof(T))) {
            return false;
        }

        __try {
            outValue = *reinterpret_cast<T*>(address);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }

        return true;
    }

    template <typename T>
    bool SafeWrite(uintptr_t address, const T& value) {
        if (!IsWritableAddress(address, sizeof(T))) {
            return false;
        }

        __try {
            *reinterpret_cast<T*>(address) = value;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }

        return true;
    }

    float AbsFloat(float value) {
        return (value >= 0.0f) ? value : -value;
    }

    void ResetDebugState() {
        DebugState::moduleBase = 0;
        DebugState::localPlayerPtr = 0;
        DebugState::localPlayerOffsetUsed = 0;
        DebugState::entityListBase = 0;
        DebugState::viewMatrixAddr = 0;
        DebugState::viewMatrixCandidateRelative = 0;
        DebugState::viewMatrixCandidateAbsolute = Offsets::ViewMatrix;
        DebugState::viewMatrixRelativeReadable = false;
        DebugState::viewMatrixAbsoluteReadable = false;
        DebugState::playerCount = 0;
        DebugState::entitiesScanned = 0;
        DebugState::entitiesValid = 0;
        DebugState::entitiesW2S = 0;
        DebugState::viewMatrixLooksValid = false;
        DebugState::usingAngleProjection = false;
        DebugState::usingSwappedCameraAxes = false;
        DebugState::teamDataAvailable = false;
        DebugState::localTeamId = -1;
        DebugState::entitiesEspFiltered = 0;
        DebugState::entitiesAimbotFiltered = 0;
        DebugState::bestTargetFov = -1.0f;
    }

    bool TryReadTeamId(const Player* player, int& outTeamId) {
        outTeamId = -1;
        if (!player || Offsets::Team == 0) {
            return false;
        }

        const uintptr_t teamAddress = reinterpret_cast<uintptr_t>(player) + Offsets::Team;
        if (!SafeRead(teamAddress, outTeamId)) {
            return false;
        }

        return true;
    }


    bool ResolveViewMatrix(uintptr_t moduleBase, float*& outMatrixPtr, bool& outLooksValid) {
        outMatrixPtr = nullptr;
        outLooksValid = false;

        const uintptr_t candidates[2] = {
            moduleBase + Offsets::ViewMatrix,
            Offsets::ViewMatrix
        };

        for (uintptr_t candidate : candidates) {
            if (!IsReadableAddress(candidate, sizeof(float) * 16)) {
                continue;
            }

            float m0 = 0.0f;
            float m5 = 0.0f;
            float m10 = 0.0f;
            if (!SafeRead(candidate + 0 * sizeof(float), m0) ||
                !SafeRead(candidate + 5 * sizeof(float), m5) ||
                !SafeRead(candidate + 10 * sizeof(float), m10)) {
                continue;
            }

            outMatrixPtr = reinterpret_cast<float*>(candidate);
            outLooksValid = (AbsFloat(m0) > 0.0001f || AbsFloat(m5) > 0.0001f || AbsFloat(m10) > 0.0001f);
            return true;
        }

        return false;
    }

    bool ResolveLocalPlayer(uintptr_t moduleBase, Player*& outLocalPlayer) {
        outLocalPlayer = nullptr;

        if (SafeRead(moduleBase + Offsets::LocalPlayer, outLocalPlayer) &&
            outLocalPlayer &&
            IsReadableAddress(reinterpret_cast<uintptr_t>(outLocalPlayer), sizeof(Player))) {
            DebugState::localPlayerOffsetUsed = Offsets::LocalPlayer;
            return true;
        }

        outLocalPlayer = nullptr;
        if (SafeRead(moduleBase + Offsets::LocalPlayerAlt, outLocalPlayer) &&
            outLocalPlayer &&
            IsReadableAddress(reinterpret_cast<uintptr_t>(outLocalPlayer), sizeof(Player))) {
            DebugState::localPlayerOffsetUsed = Offsets::LocalPlayerAlt;
            return true;
        }

        outLocalPlayer = nullptr;
        return false;
    }

    bool ResolvePlayerCount(uintptr_t moduleBase, int& outPlayerCount) {
        outPlayerCount = 0;
        if (!SafeRead(moduleBase + Offsets::PlayerCount, outPlayerCount)) {
            return false;
        }

        return outPlayerCount > 1 && outPlayerCount <= kMaxPlayers;
    }

    Player* GetEntityByIndex(uintptr_t moduleBase, int index, uintptr_t& resolvedListBase) {
        uintptr_t listPtr = 0;
        SafeRead(moduleBase + Offsets::EntityList, listPtr);

        if (listPtr) {
            Player* byPointerList = nullptr;
            if (!SafeRead(listPtr + index * 4, byPointerList)) {
                byPointerList = nullptr;
            }
            if (byPointerList) {
                resolvedListBase = listPtr;
                return byPointerList;
            }
        }

        const uintptr_t directListBase = moduleBase + Offsets::EntityList;
        Player* byDirectList = nullptr;
        if (!SafeRead(directListBase + index * 4, byDirectList)) {
            byDirectList = nullptr;
        }
        if (byDirectList) {
            resolvedListBase = directListBase;
            return byDirectList;
        }

        return nullptr;
    }

    bool ReadIntOffset(const Player* player, uintptr_t offset, int fallbackValue, int& outValue) {
        if (!player) {
            return false;
        }

        if (offset == 0) {
            outValue = fallbackValue;
            return true;
        }

        const uintptr_t address = reinterpret_cast<uintptr_t>(player) + offset;
        if (!SafeRead(address, outValue)) {
            outValue = fallbackValue;
            return false;
        }

        return true;
    }

    std::string ReadNameOffset(const Player* player) {
        if (!player) {
            return "";
        }

        constexpr size_t kNameMaxLen = 16;
        struct NameBuffer { char value[kNameMaxLen]; };

        NameBuffer buffer{};
        bool readOk = false;
        if (Offsets::Name != 0) {
            const uintptr_t address = reinterpret_cast<uintptr_t>(player) + Offsets::Name;
            readOk = SafeRead(address, buffer);
        }

        if (!readOk) {
            for (size_t i = 0; i < kNameMaxLen; ++i) {
                buffer.value[i] = player->name[i];
            }
        }

        size_t length = 0;
        while (length < kNameMaxLen && buffer.value[length] != '\0') {
            ++length;
        }

        return std::string(buffer.value, length);
    }
}

void RunHackLogic() {
    ResetDebugState();

    const uintptr_t moduleBase = reinterpret_cast<uintptr_t>(GetModuleHandleA("ac_client.exe"));
    if (!moduleBase) return;

    DebugState::moduleBase = moduleBase;
    DebugState::viewMatrixCandidateRelative = moduleBase + Offsets::ViewMatrix;
    DebugState::viewMatrixRelativeReadable = IsReadableAddress(DebugState::viewMatrixCandidateRelative, sizeof(float) * 16);
    DebugState::viewMatrixAbsoluteReadable = IsReadableAddress(DebugState::viewMatrixCandidateAbsolute, sizeof(float) * 16);

    Player* localPlayer = nullptr;
    if (!ResolveLocalPlayer(moduleBase, localPlayer)) {
        return;
    }

    DebugState::localPlayerPtr = reinterpret_cast<uintptr_t>(localPlayer);
    int localHealth = 0;
    ReadIntOffset(localPlayer, Offsets::Health, localPlayer->health, localHealth);
    if (!localPlayer || localHealth <= 0 || localHealth > 100) return;

    const Vector3 localHeadPos = localPlayer->GetHeadPos();


    int localTeamId = -1;
    DebugState::teamDataAvailable = TryReadTeamId(localPlayer, localTeamId);
    DebugState::localTeamId = localTeamId;

    int playerCount = 0;
    if (!ResolvePlayerCount(moduleBase, playerCount)) return;
    DebugState::playerCount = playerCount;

    float* viewMatrix = nullptr;
    ResolveViewMatrix(moduleBase, viewMatrix, DebugState::viewMatrixLooksValid);
    DebugState::viewMatrixAddr = reinterpret_cast<uintptr_t>(viewMatrix);

    float gameFov = 90.0f;
    SafeRead(moduleBase + Offsets::Fov, gameFov);
    DebugState::gameFov = gameFov;

    const ImGuiIO& io = ImGui::GetIO();
    const int screenWidth = static_cast<int>(io.DisplaySize.x);
    const int screenHeight = static_cast<int>(io.DisplaySize.y);

    float bestTargetScore = 3.402823466e+38F;
    AimbotFeature::Selection selection{};
    static Player* lockedTarget = nullptr;

    for (int i = 1; i < playerCount; ++i) {
        DebugState::entitiesScanned++;

        uintptr_t resolvedListBase = 0;
        Player* entity = GetEntityByIndex(moduleBase, i, resolvedListBase);
        if (entity && !IsReadableAddress(reinterpret_cast<uintptr_t>(entity), sizeof(Player))) {
            continue;
        }
        int entityHealth = 0;
        ReadIntOffset(entity, Offsets::Health, entity ? entity->health : 0, entityHealth);
        if (!entity || entity == localPlayer || entityHealth <= 0 || entityHealth > 100) {
            continue;
        }
    int entityArmor = 0;
    ReadIntOffset(entity, Offsets::Armor, entity->armor, entityArmor);
    const std::string entityName = ReadNameOffset(entity);
    const float entityDistance = localHeadPos.Distance(entity->GetHeadPos());

        if (!DebugState::entityListBase && resolvedListBase) {
            DebugState::entityListBase = resolvedListBase;
        }

        DebugState::entitiesValid++;

        int entityTeamId = -1;
        const bool hasEntityTeamData = DebugState::teamDataAvailable && TryReadTeamId(entity, entityTeamId);
        const bool isTeammate = hasEntityTeamData && (entityTeamId == localTeamId);

        if (Settings::bESP && viewMatrix) {
            bool shouldDrawEsp = true;
            if (hasEntityTeamData) {
                if ((isTeammate && !Settings::espShowTeammates) ||
                    (!isTeammate && !Settings::espShowEnemies)) {
                    shouldDrawEsp = false;
                    DebugState::entitiesEspFiltered++;
                }
            }

            if (shouldDrawEsp) {
                EspFeature::TryDrawEntityEsp(
                    entity,
                    viewMatrix,
                    screenWidth,
                    screenHeight,
                    isTeammate,
                    hasEntityTeamData,
                    entityHealth,
                    entityArmor,
                    entityName,
                    entityDistance,
                    Settings::espShowName,
                    Settings::espShowHealth,
                    Settings::espShowArmor,
                    Settings::espShowDistance,
                    Settings::espShowHealthValue,
                    Settings::espShowArmorValue,
                    Settings::espUseTeamColors,
                    Settings::espInfoPosition,
                    Settings::espBoxFilled,
                    Settings::espBoxThickness,
                    Settings::espBoxFillAlpha,
                    Settings::bSnaplines,
                    DebugState::entitiesW2S);
            }
        }

        if (Settings::bAimbot) {
            bool shouldTarget = true;
            if (hasEntityTeamData) {
                if ((isTeammate && !Settings::aimbotTargetTeammates) ||
                    (!isTeammate && !Settings::aimbotTargetEnemies)) {
                    shouldTarget = false;
                    DebugState::entitiesAimbotFiltered++;
                }
            }

            if (shouldTarget) {
                AimbotFeature::ConsiderEntityTarget(
                    localPlayer,
                    entity,
                    lockedTarget,
                    Settings::aimbotTargetBone,
                    entityHealth,
                    static_cast<AimbotFeature::TargetPriority>(Settings::aimbotPriorityMode),
                    Settings::aimbotFOV,
                    bestTargetScore,
                    selection);
                DebugState::bestTargetFov = (selection.bestTarget != nullptr) ? selection.bestFov : -1.0f;
            }
        }
    }

    if (!Settings::bAimbot) {
        lockedTarget = nullptr;
    }

    if (Settings::bAimbot && selection.bestTarget && GetAsyncKeyState(VK_RBUTTON)) {
        lockedTarget = selection.bestTarget;
        DebugState::usingSwappedCameraAxes = selection.useSwappedAxes;
        AimbotFeature::ApplyAim(localPlayer, selection, Settings::aimbotSmooth);
    }

    if (!(GetAsyncKeyState(VK_RBUTTON) & 0x8000)) {
        lockedTarget = nullptr;
    }
}