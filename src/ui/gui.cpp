#include "ui/gui.h"

#include "config/config.h"
#include "core/game_state.h"
#include "features/hack_logic.h"

#include <cstdio>
#include <gl/GL.h>
#include <winuser.h>

void GUI::Init(HWND window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;

    ImGui::StyleColorsDark();

    ImGui_ImplWin32_Init(window);
    ImGui_ImplOpenGL2_Init();

    bInitialized = true;
}

void GUI::Render() {
    if (!bInitialized) return;

    ImGuiIO& io = ImGui::GetIO();
    io.MouseDrawCursor = bMenuOpen;

    if (bMenuOpen) {
        ClipCursor(nullptr);
        ReleaseCapture();
        SetCursor(LoadCursor(nullptr, IDC_ARROW));
    }

    static bool wasMenuOpen = false;
    if (bMenuOpen && !wasMenuOpen) {
        ClipCursor(nullptr);
        ReleaseCapture();
        SetCursor(LoadCursor(nullptr, IDC_ARROW));
    }
    wasMenuOpen = bMenuOpen;

    ImGui_ImplOpenGL2_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    if (bMenuOpen) {
        const ImVec2 menuSize(760.0f, 430.0f);
        const ImVec2 menuPos((io.DisplaySize.x - menuSize.x) * 0.5f, (io.DisplaySize.y - menuSize.y) * 0.5f);
        ImGui::SetNextWindowPos(menuPos, ImGuiCond_Appearing);
        ImGui::SetNextWindowSize(menuSize, ImGuiCond_FirstUseEver);

        ImGui::Begin(AppConfig::menuTitle.c_str(), &bMenuOpen, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);

        const float panelWidth = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5f;
        const float panelHeight = ImGui::GetContentRegionAvail().y - 40.0f;

        ImGui::BeginChild("CheatPanel", ImVec2(panelWidth, panelHeight), true);
        ImGui::Text("Cheats");
        ImGui::Separator();

        ImGui::Checkbox("Aimbot", &Settings::bAimbot);
        if (Settings::bAimbot) {
            const char* priorityItems[] = { "Nearest Crosshair", "Nearest Distance", "Lowest HP" };
            ImGui::Combo("Priority", &Settings::aimbotPriorityMode, priorityItems, IM_ARRAYSIZE(priorityItems));
            ImGui::SliderFloat("FOV", &Settings::aimbotFOV, 1.0f, 180.0f);
            ImGui::SliderFloat("Smooth", &Settings::aimbotSmooth, 1.0f, 10.0f);
            ImGui::Checkbox("Target Teammates", &Settings::aimbotTargetTeammates);
            ImGui::Checkbox("Target Enemies", &Settings::aimbotTargetEnemies);
        }

        ImGui::Separator();

        ImGui::Checkbox("ESP", &Settings::bESP);
        if (Settings::bESP) {
            ImGui::Checkbox("Snaplines", &Settings::bSnaplines);
            ImGui::Checkbox("Show Teammates", &Settings::espShowTeammates);
            ImGui::Checkbox("Show Enemies", &Settings::espShowEnemies);
        }

        ImGui::Separator();
        if (ImGui::Button("Unload DLL")) {
            bUnloadRequested = true;
        }
        ImGui::TextWrapped("Hotkeys: INSERT (menu), DELETE/END (unload)");
        ImGui::EndChild();

        ImGui::SameLine();

        ImGui::BeginChild("DebugPanel", ImVec2(0.0f, panelHeight), true);
        ImGui::Text("Debug");
        ImGui::Separator();
        ImGui::Text("ModuleBase: 0x%p", (void*)DebugState::moduleBase);
        ImGui::Text("LocalPlayer: 0x%p", (void*)DebugState::localPlayerPtr);
        ImGui::Text("LocalPlayerOffsetUsed: 0x%p", (void*)DebugState::localPlayerOffsetUsed);
        ImGui::Text("EntityListBase: 0x%p", (void*)DebugState::entityListBase);
        ImGui::Text("PlayerCount: %d", DebugState::playerCount);
        ImGui::Text("Entities: scanned=%d valid=%d w2s=%d",
            DebugState::entitiesScanned,
            DebugState::entitiesValid,
            DebugState::entitiesW2S);
        ImGui::Text("ESP Filtered: %d", DebugState::entitiesEspFiltered);
        ImGui::Text("Aimbot Filtered: %d", DebugState::entitiesAimbotFiltered);
        ImGui::Text("Team data: %s (local=%d, offset=0x%p)",
            DebugState::teamDataAvailable ? "available" : "unavailable",
            DebugState::localTeamId,
            (void*)Offsets::Team);
        ImGui::Text("ViewMatrix: %s", DebugState::viewMatrixLooksValid ? "looks valid" : "invalid");
        ImGui::Text("ResolvedMatrixAddr: 0x%p", (void*)DebugState::viewMatrixAddr);
        ImGui::Text("Candidate Rel: 0x%p (%s)",
            (void*)DebugState::viewMatrixCandidateRelative,
            DebugState::viewMatrixRelativeReadable ? "readable" : "unreadable");
        ImGui::Text("Candidate Abs: 0x%p (%s)",
            (void*)DebugState::viewMatrixCandidateAbsolute,
            DebugState::viewMatrixAbsoluteReadable ? "readable" : "unreadable");
        ImGui::Text("CameraAxes: %s", DebugState::usingSwappedCameraAxes ? "swapped (X=Yaw?)" : "standard");
        ImGui::Text("Aimbot Priority: %d", Settings::aimbotPriorityMode);
        ImGui::Text("Best target FOV: %.2f", DebugState::bestTargetFov);

        char debugBuffer[1024];
        std::snprintf(
            debugBuffer,
            sizeof(debugBuffer),
            "ModuleBase=0x%p\nLocalPlayer=0x%p\nLocalPlayerOffsetUsed=0x%p\nEntityListBase=0x%p\nPlayerCount=%d\nEntitiesScanned=%d\nEntitiesValid=%d\nEntitiesW2S=%d\nEntitiesEspFiltered=%d\nEntitiesAimbotFiltered=%d\nTeamDataAvailable=%s\nLocalTeamId=%d\nTeamOffset=0x%p\nViewMatrixLooksValid=%s\nResolvedMatrixAddr=0x%p\nViewMatrixCandidateRelative=0x%p (%s)\nViewMatrixCandidateAbsolute=0x%p (%s)\nCameraAxes=%s\nAimbotPriorityMode=%d\nBestTargetFov=%.2f",
            (void*)DebugState::moduleBase,
            (void*)DebugState::localPlayerPtr,
            (void*)DebugState::localPlayerOffsetUsed,
            (void*)DebugState::entityListBase,
            DebugState::playerCount,
            DebugState::entitiesScanned,
            DebugState::entitiesValid,
            DebugState::entitiesW2S,
            DebugState::entitiesEspFiltered,
            DebugState::entitiesAimbotFiltered,
            DebugState::teamDataAvailable ? "true" : "false",
            DebugState::localTeamId,
            (void*)Offsets::Team,
            DebugState::viewMatrixLooksValid ? "true" : "false",
            (void*)DebugState::viewMatrixAddr,
            (void*)DebugState::viewMatrixCandidateRelative,
            DebugState::viewMatrixRelativeReadable ? "readable" : "unreadable",
            (void*)DebugState::viewMatrixCandidateAbsolute,
            DebugState::viewMatrixAbsoluteReadable ? "readable" : "unreadable",
            DebugState::usingSwappedCameraAxes ? "swapped" : "standard",
            Settings::aimbotPriorityMode,
            DebugState::bestTargetFov);

        if (ImGui::Button("Copy Debug To Clipboard")) {
            ImGui::SetClipboardText(debugBuffer);
        }
        ImGui::EndChild();

        ImGui::End();
    }

    RunHackLogic();

    ImGui::Render();
    ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
}

void GUI::Shutdown() {
    if (bInitialized) {
        ImGui_ImplOpenGL2_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        bInitialized = false;
    }
}