#pragma once

#include "imgui.h"

namespace GUI::UITheme {
    struct Resources {
        ImFont* mainFont = nullptr;
        ImFont* titleFont = nullptr;
    };

    void LoadFonts(ImGuiIO& io, Resources& resources);
    void Apply();
}
