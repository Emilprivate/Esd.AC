#include "core/hooking/hook.h"

#include "config/config.h"
#include "ui/gui.h"

#include <MinHook.h>
#include <gl/GL.h>
#include <winuser.h>

typedef BOOL(__stdcall* twglSwapBuffers)(HDC hDc);
twglSwapBuffers owglSwapBuffers = nullptr;

typedef BOOL(WINAPI* tSetCursorPos)(int X, int Y);
tSetCursorPos oSetCursorPos = nullptr;

typedef BOOL(WINAPI* tClipCursor)(const RECT* lpRect);
tClipCursor oClipCursor = nullptr;

WNDPROC oWndProc = nullptr;
HWND window = nullptr;
void* gWglSwapBuffersAddr = nullptr;
void* gSetCursorPosAddr = nullptr;
void* gClipCursorAddr = nullptr;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

BOOL WINAPI hkSetCursorPos(int X, int Y) {
    if (GUI::bMenuOpen && GetForegroundWindow() == window) {
        return TRUE;
    }
    return oSetCursorPos(X, Y);
}

BOOL WINAPI hkClipCursor(const RECT* lpRect) {
    if (GUI::bMenuOpen && GetForegroundWindow() == window && lpRect != nullptr) {
        return TRUE;
    }
    return oClipCursor(lpRect);
}

LRESULT __stdcall hkWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_KEYDOWN && static_cast<int>(wParam) == AppConfig::menuToggleKey) {
        GUI::bMenuOpen = !GUI::bMenuOpen;
        return 1;
    }

    if (uMsg == WM_KEYDOWN &&
        (static_cast<int>(wParam) == AppConfig::unloadPrimaryKey ||
            static_cast<int>(wParam) == AppConfig::unloadSecondaryKey)) {
        GUI::bUnloadRequested = true;
        return 1;
    }

    if (GUI::bMenuOpen) {
        ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);

        switch (uMsg) {
            case WM_INPUT:
            case WM_MOUSEMOVE:
            case WM_LBUTTONDOWN:
            case WM_LBUTTONUP:
            case WM_LBUTTONDBLCLK:
            case WM_RBUTTONDOWN:
            case WM_RBUTTONUP:
            case WM_RBUTTONDBLCLK:
            case WM_MBUTTONDOWN:
            case WM_MBUTTONUP:
            case WM_MBUTTONDBLCLK:
            case WM_MOUSEWHEEL:
            case WM_MOUSEHWHEEL:
            case WM_XBUTTONDOWN:
            case WM_XBUTTONUP:
            case WM_XBUTTONDBLCLK:
            case WM_KEYDOWN:
            case WM_KEYUP:
            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
            case WM_CHAR:
                return 1;
            default:
                break;
        }
    }
    return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
}

BOOL __stdcall hkwglSwapBuffers(HDC hDc) {
    if (GUI::bShouldUnload) {
        if (GUI::bInitialized) {
            GUI::Shutdown();
        }
        return owglSwapBuffers(hDc);
    }

    if (!GUI::bInitialized) {
        window = WindowFromDC(hDc);
        oWndProc = (WNDPROC)SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)hkWndProc);
        GUI::Init(window);
    }

    GUI::Render();

    return owglSwapBuffers(hDc);
}

bool Hook::Init() {
    if (MH_Initialize() != MH_OK) return false;

    HMODULE hMod = GetModuleHandleA("opengl32.dll");
    if (!hMod) return false;

    HMODULE hUser32 = GetModuleHandleA("user32.dll");
    if (!hUser32) return false;

    gWglSwapBuffersAddr = GetProcAddress(hMod, "wglSwapBuffers");
    if (!gWglSwapBuffersAddr) return false;

    gSetCursorPosAddr = GetProcAddress(hUser32, "SetCursorPos");
    if (!gSetCursorPosAddr) return false;

    gClipCursorAddr = GetProcAddress(hUser32, "ClipCursor");
    if (!gClipCursorAddr) return false;

    if (MH_CreateHook(gWglSwapBuffersAddr, &hkwglSwapBuffers, reinterpret_cast<void**>(&owglSwapBuffers)) != MH_OK) return false;
    if (MH_EnableHook(gWglSwapBuffersAddr) != MH_OK) return false;

    if (MH_CreateHook(gSetCursorPosAddr, &hkSetCursorPos, reinterpret_cast<void**>(&oSetCursorPos)) != MH_OK) return false;
    if (MH_EnableHook(gSetCursorPosAddr) != MH_OK) return false;

    if (MH_CreateHook(gClipCursorAddr, &hkClipCursor, reinterpret_cast<void**>(&oClipCursor)) != MH_OK) return false;
    if (MH_EnableHook(gClipCursorAddr) != MH_OK) return false;

    return true;
}

void Hook::Remove() {
    if (gWglSwapBuffersAddr) {
        MH_DisableHook(gWglSwapBuffersAddr);
        MH_RemoveHook(gWglSwapBuffersAddr);
    }
    if (gSetCursorPosAddr) {
        MH_DisableHook(gSetCursorPosAddr);
        MH_RemoveHook(gSetCursorPosAddr);
    }
    if (gClipCursorAddr) {
        MH_DisableHook(gClipCursorAddr);
        MH_RemoveHook(gClipCursorAddr);
    }
    MH_Uninitialize();

    if (oWndProc && window && IsWindow(window)) {
        SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)oWndProc);
    }

    oWndProc = nullptr;
    window = nullptr;
    owglSwapBuffers = nullptr;
    oSetCursorPos = nullptr;
    oClipCursor = nullptr;
    gWglSwapBuffersAddr = nullptr;
    gSetCursorPosAddr = nullptr;
    gClipCursorAddr = nullptr;
}