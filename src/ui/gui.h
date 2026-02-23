#pragma once

#include <windows.h>
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_opengl2.h"

namespace GUI {
    inline bool bInitialized = false;
    inline bool bMenuOpen = false;
    inline bool bShouldUnload = false;
    inline bool bUnloadRequested = false;

    void Init(HWND window);
    void Render();
    void Shutdown();
}