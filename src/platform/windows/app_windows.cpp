// Windows application lifecycle implementation
#include "../../../include/platform.h"
#include "../platform_impl.h"
#include "windows_common.h"
#include <windows.h>
#include <string>
#include <iostream>
#include <io.h>
#include <fcntl.h>
#include <objbase.h>
#include <winreg.h>

#ifdef PLATFORM_WINDOWS

namespace platform {

// Helper function to convert std::string to std::wstring
// Note: Currently unused but kept for potential future use
// static std::wstring stringToWString(const std::string& str) {
//     if (str.empty()) return std::wstring();
//     int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
//     std::wstring wstrTo(size_needed, 0);
//     MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
//     return wstrTo;
// }

static HINSTANCE g_hInstance = nullptr;
static const wchar_t* g_className = L"NativeWindowClass";
static bool g_comInitialized = false;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

void initApplication() {
    if (!g_hInstance) {
        g_hInstance = GetModuleHandle(nullptr);
        
        // Initialize COM for WebView2 (required for WebView2 to work)
        HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        if (SUCCEEDED(hr)) {
            g_comInitialized = true;
            std::wcout << L"COM initialized successfully" << std::endl;
        } else if (hr == RPC_E_CHANGED_MODE) {
            // COM was already initialized with a different mode - this is OK
            std::wcout << L"COM already initialized with different mode" << std::endl;
        } else {
            std::wcout << L"COM initialization warning: 0x" << std::hex << hr << std::dec << std::endl;
        }
        
        // Allocate console for debug output (optional - can be removed for release)
        #ifdef _DEBUG
        AllocConsole();
        FILE* pCout;
        FILE* pCerr;
        FILE* pCin;
        freopen_s(&pCout, "CONOUT$", "w", stdout);
        freopen_s(&pCerr, "CONOUT$", "w", stderr);
        freopen_s(&pCin, "CONIN$", "r", stdin);
        std::cout.clear();
        std::cerr.clear();
        std::cin.clear();
        #endif
        
        WNDCLASSW wc = {};
        wc.lpfnWndProc = WindowProc;
        wc.hInstance = g_hInstance;
        wc.lpszClassName = g_className;
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        
        RegisterClassW(&wc);
    }
}

void runApplication() {
    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void quitApplication() {
    // Uninitialize COM if we initialized it
    if (g_comInitialized) {
        CoUninitialize();
        g_comInitialized = false;
    }
    PostQuitMessage(0);
}

static void (*s_appActivateCb)(void*) = nullptr;
static void (*s_appDeactivateCb)(void*) = nullptr;
static void* s_appActivateUd = nullptr;
static void* s_appDeactivateUd = nullptr;

void setAppActivateCallback(void (*cb)(void*), void* ud) {
    s_appActivateCb = cb;
    s_appActivateUd = ud;
}

void setAppDeactivateCallback(void (*cb)(void*), void* ud) {
    s_appDeactivateCb = cb;
    s_appDeactivateUd = ud;
}

void notifyAppActivate(bool activated) {
    if (activated && s_appActivateCb) s_appActivateCb(s_appActivateUd);
    else if (!activated && s_appDeactivateCb) s_appDeactivateCb(s_appDeactivateUd);
}

static void (*s_themeChangeCb)(const char*, void*) = nullptr;
static void* s_themeChangeUd = nullptr;

void setThemeChangeCallback(void (*cb)(const char* theme, void* userData), void* ud) {
    s_themeChangeCb = cb;
    s_themeChangeUd = ud;
    if (cb) {
        notifyThemeChange();  // Fire once with current theme
    }
}

void notifyThemeChange() {
    if (!s_themeChangeCb) return;
    const char* theme = "light";
    HKEY hKey = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
                      0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD value = 1;
        DWORD size = sizeof(DWORD);
        if (RegQueryValueExW(hKey, L"AppsUseLightTheme", nullptr, nullptr, (LPBYTE)&value, &size) == ERROR_SUCCESS) {
            theme = (value != 0) ? "light" : "dark";
        }
        RegCloseKey(hKey);
    }
    s_themeChangeCb(theme, s_themeChangeUd);
}

static void (*s_appOpenFileCb)(const std::string&, void*) = nullptr;
static void* s_appOpenFileUd = nullptr;

void setKeyShortcutCallback(void (*)(const std::string&, void*), void*) {}
void setAppOpenFileCallback(void (*cb)(const std::string&, void*), void* ud) {
    s_appOpenFileCb = cb;
    s_appOpenFileUd = ud;
}
void deliverOpenFilePaths(int argc, const char* argv[]) {
    if (!s_appOpenFileCb) return;
    for (int i = 1; i < argc && argv[i]; ++i) {
        if (argv[i][0] != '-') {
            s_appOpenFileCb(std::string(argv[i]), s_appOpenFileUd);
        }
    }
}

HINSTANCE getInstance() {
    return g_hInstance;
}

const wchar_t* getClassName() {
    return g_className;
}

// Forward declaration - implemented in button_windows.cpp
LRESULT CALLBACK MainWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    return MainWindowProc(hwnd, uMsg, wParam, lParam);
}

} // namespace platform

#endif // PLATFORM_WINDOWS
