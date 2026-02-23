#include "ui/gui_menu.h"

#include "config/config.h"
#include "core/game_state.h"
#include "ui/gui.h"

#include <cstdio>
#include <string>
#include <winuser.h>

namespace {
    std::string VirtualKeyToString(int vk) {
        const UINT scanCode = MapVirtualKeyA(static_cast<UINT>(vk), MAPVK_VK_TO_VSC);
        LONG lParam = static_cast<LONG>(scanCode << 16);

        if (vk == VK_LEFT || vk == VK_UP || vk == VK_RIGHT || vk == VK_DOWN ||
            vk == VK_PRIOR || vk == VK_NEXT || vk == VK_END || vk == VK_HOME ||
            vk == VK_INSERT || vk == VK_DELETE || vk == VK_DIVIDE || vk == VK_NUMLOCK) {
            lParam |= (1 << 24);
        }

        char keyName[64]{};
        const int keyNameLen = GetKeyNameTextA(lParam, keyName, static_cast<int>(sizeof(keyName)));
        if (keyNameLen > 0) {
            return std::string(keyName, keyNameLen);
        }

        char fallback[16]{};
        std::snprintf(fallback, sizeof(fallback), "0x%02X", vk & 0xFF);
        return fallback;
    }

    void DrawHelpTip(const char* text) {
        ImGui::SameLine();
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 24.0f);
            ImGui::TextUnformatted(text);
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
    }

    void DrawToggleRow(const char* label, bool enabled) {
        ImGui::TextUnformatted(label);
        ImGui::SameLine();
        ImGui::TextColored(enabled ? ImVec4(0.35f, 0.85f, 0.45f, 1.0f) : ImVec4(0.85f, 0.35f, 0.35f, 1.0f),
            enabled ? "ON" : "OFF");
    }

    void DrawBoneSelector(int& targetBoneMode) {
        const ImVec2 selectorSize(140.0f, 180.0f);
        const ImVec2 start = ImGui::GetCursorScreenPos();
        ImDrawList* drawList = ImGui::GetWindowDrawList();

        ImGui::InvisibleButton("BoneSelector", selectorSize);

        const float centerX = start.x + selectorSize.x * 0.5f;
        const float headY = start.y + 26.0f;
        const float chestY = start.y + 70.0f;
        const float pelvisY = start.y + 110.0f;
        const float footY = start.y + 160.0f;

        const ImU32 lineColor = IM_COL32(160, 160, 160, 200);
        const ImU32 headColor = (targetBoneMode == 0) ? IM_COL32(80, 220, 120, 255) : IM_COL32(200, 200, 200, 255);
        const ImU32 chestColor = (targetBoneMode == 1) ? IM_COL32(80, 220, 120, 255) : IM_COL32(200, 200, 200, 255);
        const ImU32 pelvisColor = (targetBoneMode == 2) ? IM_COL32(80, 220, 120, 255) : IM_COL32(200, 200, 200, 255);

        // Stick figure
        drawList->AddCircle(ImVec2(centerX, headY), 10.0f, lineColor, 16, 1.5f);
        drawList->AddLine(ImVec2(centerX, headY + 10.0f), ImVec2(centerX, pelvisY), lineColor, 1.5f);
        drawList->AddLine(ImVec2(centerX, chestY), ImVec2(centerX - 22.0f, chestY + 12.0f), lineColor, 1.5f);
        drawList->AddLine(ImVec2(centerX, chestY), ImVec2(centerX + 22.0f, chestY + 12.0f), lineColor, 1.5f);
        drawList->AddLine(ImVec2(centerX, pelvisY), ImVec2(centerX - 18.0f, footY), lineColor, 1.5f);
        drawList->AddLine(ImVec2(centerX, pelvisY), ImVec2(centerX + 18.0f, footY), lineColor, 1.5f);

        // Clickable nodes
        const ImVec2 headPos(centerX, headY);
        const ImVec2 chestPos(centerX, chestY);
        const ImVec2 pelvisPos(centerX, pelvisY);
        drawList->AddCircleFilled(headPos, 6.0f, headColor, 16);
        drawList->AddCircleFilled(chestPos, 6.0f, chestColor, 16);
        drawList->AddCircleFilled(pelvisPos, 6.0f, pelvisColor, 16);

        const ImVec2 mousePos = ImGui::GetIO().MousePos;
        if (ImGui::IsItemHovered()) {
            const float headDist = (mousePos.x - headPos.x) * (mousePos.x - headPos.x) + (mousePos.y - headPos.y) * (mousePos.y - headPos.y);
            const float chestDist = (mousePos.x - chestPos.x) * (mousePos.x - chestPos.x) + (mousePos.y - chestPos.y) * (mousePos.y - chestPos.y);
            const float pelvisDist = (mousePos.x - pelvisPos.x) * (mousePos.x - pelvisPos.x) + (mousePos.y - pelvisPos.y) * (mousePos.y - pelvisPos.y);

            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                if (headDist <= 64.0f) {
                    targetBoneMode = 0;
                } else if (chestDist <= 64.0f) {
                    targetBoneMode = 1;
                } else if (pelvisDist <= 64.0f) {
                    targetBoneMode = 2;
                }
            }
        }
    }

    void DrawTitleText(ImFont* titleFont, const char* text) {
        if (titleFont) {
            ImGui::PushFont(titleFont);
        }
        ImGui::TextUnformatted(text);
        if (titleFont) {
            ImGui::PopFont();
        }
    }

    int GetFeatureCount() {
        static const char* kFeatureNames[] = {
            "Aimbot",
            "ESP"
        };
        return static_cast<int>(IM_ARRAYSIZE(kFeatureNames));
    }

    void RenderFeatureTabs() {
        if (!ImGui::BeginTabBar("FeatureTabs")) {
            return;
        }

        if (ImGui::BeginTabItem("Aimbot")) {
            ImGui::BeginChild("AimCard", ImVec2(0.0f, 0.0f), true);
            ImGui::Checkbox("Enable Aimbot", &Settings::bAimbot);
            DrawHelpTip("Locks aim while holding right mouse button.");

            ImGui::BeginDisabled(!Settings::bAimbot);
            const char* priorityItems[] = { "Nearest Crosshair", "Nearest Distance", "Lowest HP" };
            ImGui::Combo("Target Priority", &Settings::aimbotPriorityMode, priorityItems, IM_ARRAYSIZE(priorityItems));
            DrawHelpTip("How targets are ranked inside the allowed FOV.");

            ImGui::TextUnformatted("Target Bone");
            DrawHelpTip("Click the head, chest, or pelvis node to select.");
            DrawBoneSelector(Settings::aimbotTargetBone);

            ImGui::SliderFloat("Aim FOV", &Settings::aimbotFOV, 1.0f, 180.0f, "%.1f");
            DrawHelpTip("Maximum angular distance from crosshair.");
            ImGui::SliderFloat("Aim Smooth", &Settings::aimbotSmooth, 1.0f, 10.0f, "%.1f");
            DrawHelpTip("Higher values make camera movement softer.");
            ImGui::Checkbox("Include Teammates", &Settings::aimbotTargetTeammates);
            ImGui::Checkbox("Include Enemies", &Settings::aimbotTargetEnemies);
            ImGui::EndDisabled();

            ImGui::Spacing();
            ImGui::Separator();
            DrawToggleRow("Aimbot", Settings::bAimbot);
            ImGui::EndChild();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("ESP")) {
            ImGui::BeginChild("EspCard", ImVec2(0.0f, 0.0f), true);
            ImGui::Checkbox("Enable ESP", &Settings::bESP);
            DrawHelpTip("Draws overlays for tracked players.");

            ImGui::BeginDisabled(!Settings::bESP);
            ImGui::Checkbox("Show Snaplines", &Settings::bSnaplines);
            ImGui::Checkbox("Show Teammates", &Settings::espShowTeammates);
            ImGui::Checkbox("Show Enemies", &Settings::espShowEnemies);
            ImGui::Checkbox("Show Name", &Settings::espShowName);
            ImGui::Checkbox("Show Health", &Settings::espShowHealth);
            ImGui::Checkbox("Show Armor", &Settings::espShowArmor);
            ImGui::Checkbox("Show Distance", &Settings::espShowDistance);
            ImGui::Checkbox("Health Value", &Settings::espShowHealthValue);
            ImGui::Checkbox("Armor Value", &Settings::espShowArmorValue);
            ImGui::Checkbox("Team Colors", &Settings::espUseTeamColors);

            const char* positionItems[] = { "Top", "Right", "Bottom" };
            ImGui::Combo("Info Position", &Settings::espInfoPosition, positionItems, IM_ARRAYSIZE(positionItems));

            ImGui::Separator();
            ImGui::Checkbox("Filled Box", &Settings::espBoxFilled);
            ImGui::SliderFloat("Box Thickness", &Settings::espBoxThickness, 0.5f, 6.0f, "%.1f");
            ImGui::SliderFloat("Box Fill Alpha", &Settings::espBoxFillAlpha, 0.0f, 0.9f, "%.2f");
            ImGui::EndDisabled();

            ImGui::Spacing();
            ImGui::Separator();
            DrawToggleRow("ESP", Settings::bESP);
            DrawToggleRow("Snaplines", Settings::bSnaplines && Settings::bESP);
            ImGui::EndChild();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    void RenderHotkeysTab() {
        ImGui::BeginChild("HotkeysCard", ImVec2(0.0f, 0.0f), true);
        ImGui::BulletText("Toggle Menu: %s", VirtualKeyToString(AppConfig::menuToggleKey).c_str());
        ImGui::BulletText("Unload Primary: %s", VirtualKeyToString(AppConfig::unloadPrimaryKey).c_str());
        ImGui::BulletText("Unload Secondary: %s", VirtualKeyToString(AppConfig::unloadSecondaryKey).c_str());
        ImGui::BulletText("Aimbot Hold Key: Right Mouse Button");
        ImGui::EndChild();
    }

    void RenderDiagnosticsPanel(ImFont* titleFont) {
        DrawTitleText(titleFont, "Runtime State");
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

        if (ImGui::Button("Copy Debug To Clipboard", ImVec2(-1.0f, 0.0f))) {
            ImGui::SetClipboardText(debugBuffer);
        }
    }

    void RenderActionsPanel(ImFont* titleFont) {
        DrawTitleText(titleFont, "Actions");
        ImGui::Separator();
        if (ImGui::Button("Unload DLL", ImVec2(-1.0f, 30.0f))) {
            GUI::bUnloadRequested = true;
        }
    }
}

ImVec2 GUI::UIMenu::GetMinimumMenuSize(const ImGuiIO& io) {
    const float line = ImGui::GetFrameHeightWithSpacing();
    const int featureCount = GetFeatureCount();

    const float controlsSectionRows = 14.0f + static_cast<float>(featureCount) * 3.0f;
    const float diagnosticsRows = 20.0f;
    const float contentRows = (controlsSectionRows > diagnosticsRows) ? controlsSectionRows : diagnosticsRows;

    float minWidth = 980.0f + static_cast<float>((featureCount > 2) ? (featureCount - 2) * 28 : 0);
    float minHeight = (line * contentRows) + 84.0f;

    if (minWidth > io.DisplaySize.x - 30.0f) {
        minWidth = io.DisplaySize.x - 30.0f;
    }
    if (minHeight > io.DisplaySize.y - 30.0f) {
        minHeight = io.DisplaySize.y - 30.0f;
    }

    if (minWidth < 820.0f) {
        minWidth = 820.0f;
    }
    if (minHeight < 500.0f) {
        minHeight = 500.0f;
    }

    return ImVec2(minWidth, minHeight);
}

void GUI::UIMenu::Render(const ImGuiIO& io, UITheme::Resources& resources, ImVec2& menuSize) {
    const ImVec2 minMenuSize = GetMinimumMenuSize(io);
    const ImVec2 maxMenuSize(io.DisplaySize.x - 20.0f, io.DisplaySize.y - 20.0f);

    if (menuSize.x < minMenuSize.x) menuSize.x = minMenuSize.x;
    if (menuSize.y < minMenuSize.y) menuSize.y = minMenuSize.y;
    if (menuSize.x > maxMenuSize.x) menuSize.x = maxMenuSize.x;
    if (menuSize.y > maxMenuSize.y) menuSize.y = maxMenuSize.y;

    const ImVec2 menuPos((io.DisplaySize.x - menuSize.x) * 0.5f, (io.DisplaySize.y - menuSize.y) * 0.5f);
    ImGui::SetNextWindowPos(menuPos, ImGuiCond_Appearing);
    ImGui::SetNextWindowSizeConstraints(minMenuSize, maxMenuSize);
    ImGui::SetNextWindowSize(menuSize, ImGuiCond_Always);

    ImGui::Begin(AppConfig::menuTitle.c_str(), &GUI::bMenuOpen, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    menuSize = ImGui::GetWindowSize();

    const float contentHeight = ImGui::GetContentRegionAvail().y;

    if (ImGui::BeginTable("MainLayout", 2, ImGuiTableFlags_SizingStretchProp, ImVec2(0.0f, contentHeight))) {
        ImGui::TableSetupColumn("Controls", ImGuiTableColumnFlags_WidthStretch, 1.2f);
        ImGui::TableSetupColumn("Diagnostics", ImGuiTableColumnFlags_WidthStretch, 0.9f);

        ImGui::TableNextColumn();
        ImGui::BeginChild("ControlsPanel", ImVec2(0.0f, 0.0f), true);

        if (ImGui::BeginTabBar("MainTabs")) {
            if (ImGui::BeginTabItem("Features")) {
                RenderFeatureTabs();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Hotkeys")) {
                RenderHotkeysTab();
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
        ImGui::EndChild();

        ImGui::TableNextColumn();
        const float sideSpacing = ImGui::GetStyle().ItemSpacing.y;
        const float actionsPanelHeight = 96.0f;

        ImGui::BeginChild("DiagPanel", ImVec2(0.0f, -actionsPanelHeight - sideSpacing), true);
        RenderDiagnosticsPanel(resources.titleFont);
        ImGui::EndChild();

        ImGui::BeginChild("ActionPanel", ImVec2(0.0f, 0.0f), true);
        RenderActionsPanel(resources.titleFont);
        ImGui::EndChild();

        ImGui::EndTable();
    }

    ImGui::End();
}
