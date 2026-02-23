#include "ui/gui.h"

#include "features/hack_logic.h"
#include "ui/gui_menu.h"
#include "ui/gui_theme.h"

#include <winuser.h>

namespace {
    GUI::UITheme::Resources gUiResources;
    ImVec2 gMenuSize(980.0f, 560.0f);
}

void GUI::Init(HWND window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;

    ImGui::StyleColorsDark();
    GUI::UITheme::LoadFonts(io, gUiResources);
    GUI::UITheme::Apply();

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
        GUI::UIMenu::Render(io, gUiResources, gMenuSize);
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