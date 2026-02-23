#include "ui/gui_theme.h"

#include <string>
#include <windows.h>

namespace {
    bool FileExists(const std::string& path) {
        const DWORD attrs = GetFileAttributesA(path.c_str());
        return attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY);
    }
}

void GUI::UITheme::LoadFonts(ImGuiIO& io, Resources& resources) {
    char windowsPath[MAX_PATH]{};
    const UINT len = GetWindowsDirectoryA(windowsPath, MAX_PATH);
    if (len == 0 || len >= MAX_PATH) {
        return;
    }

    const std::string fontsDir = std::string(windowsPath, len) + "\\Fonts\\";

    const std::string mainCandidates[] = {
        fontsDir + "segoeui.ttf",
        fontsDir + "tahoma.ttf",
        fontsDir + "arial.ttf"
    };

    const std::string titleCandidates[] = {
        fontsDir + "segoeuib.ttf",
        fontsDir + "tahomabd.ttf",
        fontsDir + "arialbd.ttf"
    };

    for (const auto& path : mainCandidates) {
        if (FileExists(path)) {
            resources.mainFont = io.Fonts->AddFontFromFileTTF(path.c_str(), 16.0f);
            if (resources.mainFont) {
                break;
            }
        }
    }

    for (const auto& path : titleCandidates) {
        if (FileExists(path)) {
            resources.titleFont = io.Fonts->AddFontFromFileTTF(path.c_str(), 18.0f);
            if (resources.titleFont) {
                break;
            }
        }
    }

    if (resources.mainFont) {
        io.FontDefault = resources.mainFont;
    }
}

void GUI::UITheme::Apply() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 5.0f;
    style.ChildRounding = 5.0f;
    style.FrameRounding = 3.0f;
    style.PopupRounding = 3.0f;
    style.GrabRounding = 2.0f;
    style.ScrollbarRounding = 2.0f;
    style.TabRounding = 3.0f;
    style.WindowPadding = ImVec2(12.0f, 10.0f);
    style.FramePadding = ImVec2(8.0f, 5.0f);
    style.ItemSpacing = ImVec2(8.0f, 7.0f);
    style.ItemInnerSpacing = ImVec2(6.0f, 6.0f);
    style.IndentSpacing = 20.0f;
    style.WindowBorderSize = 1.0f;
    style.ChildBorderSize = 1.0f;
    style.FrameBorderSize = 1.0f;

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_Text] = ImVec4(0.90f, 0.92f, 0.93f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.54f, 0.56f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.07f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.10f, 0.10f, 0.11f, 0.98f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.09f, 0.99f);
    colors[ImGuiCol_Border] = ImVec4(0.20f, 0.20f, 0.21f, 0.95f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.12f, 0.12f, 0.13f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.16f, 0.16f, 0.17f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.18f, 0.18f, 0.19f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.13f, 0.14f, 0.15f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.18f, 0.20f, 0.19f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.24f, 0.30f, 0.24f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.13f, 0.14f, 0.15f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.19f, 0.22f, 0.20f, 1.00f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.22f, 0.28f, 0.22f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.36f, 0.86f, 0.36f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.33f, 0.78f, 0.33f, 0.90f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.40f, 0.92f, 0.40f, 1.00f);
    colors[ImGuiCol_Tab] = ImVec4(0.11f, 0.11f, 0.12f, 1.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.19f, 0.22f, 0.20f, 1.00f);
    colors[ImGuiCol_TabActive] = ImVec4(0.16f, 0.20f, 0.17f, 1.00f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.10f, 0.10f, 0.11f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.13f, 0.14f, 0.15f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.08f, 0.08f, 0.09f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.10f, 0.10f, 0.11f, 1.00f);
    colors[ImGuiCol_Separator] = ImVec4(0.26f, 0.26f, 0.27f, 1.00f);
}
