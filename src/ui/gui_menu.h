#pragma once

#include "imgui.h"
#include "ui/gui_theme.h"

namespace GUI::UIMenu {
    ImVec2 GetMinimumMenuSize(const ImGuiIO& io);
    void Render(const ImGuiIO& io, UITheme::Resources& resources, ImVec2& menuSize);
}
