// Windows platform common definitions
#ifndef WINDOWS_COMMON_H
#define WINDOWS_COMMON_H

#include <windows.h>
#include <string>
#include <map>

#ifdef PLATFORM_WINDOWS

// Deferred resize: posted from show/maximize so layout runs in next message loop
#define WM_DEFERRED_RESIZE (WM_USER + 1)
// Deferred WebView message: run after WebMessageReceived returns (avoids reentrancy - required for modal file dialog)
#define WM_DEFERRED_WEBVIEW_MESSAGE (WM_USER + 2)
#define IDT_DEFERRED_RESIZE 1

namespace platform {

typedef void (*ResizeCallback)(int width, int height, void* userData);
typedef void (*MoveCallback)(int x, int y, void* userData);
typedef void (*FileDropCallback)(const std::string& pathsJson, void* userData);

typedef void (*CloseCallback)(void* userData);
typedef void (*FocusCallback)(void* userData);
typedef void (*StateCallback)(const char* state, void* userData);
typedef void (*MenuItemCallback)(const std::string& itemId, void* userData);

struct WindowData {
    HWND hwnd;
    bool visible;
    std::string title;
    bool isMainWindow;  // Track if this is the main application window
    void* userData;     // Store Window* pointer for cleanup
    ResizeCallback resizeCallback;  // Callback for window resize
    void* resizeUserData;  // User data for resize callback
    MoveCallback moveCallback;  // Callback when window moves
    void* moveUserData;
    FileDropCallback fileDropCallback;
    void* fileDropUserData;
    CloseCallback closeCallback;    // Callback when user closes window (X button)
    void* closeUserData;            // WebViewWindow* to delete when closed
    bool beingDestroyed;            // True when in WM_DESTROY - skip DestroyWindow in destroyWindow
    FocusCallback focusCallback;
    FocusCallback blurCallback;
    void* focusUserData;
    void* blurUserData;
    StateCallback stateCallback;
    void* stateUserData;
    MenuItemCallback menuItemCallback;
    void* menuUserData;
    std::map<UINT, std::string> menuIdToItemId;
    MenuItemCallback contextMenuCallback;
    void* contextMenuUserData;
    std::map<UINT, std::string> contextMenuIdToItemId;
};

// Forward declarations
extern std::map<HWND, WindowData*> g_windowMap;
extern WindowData* g_mainWindow;

// Called from MainWindowProc on WM_ACTIVATEAPP
void notifyAppActivate(bool activated);
// Called from MainWindowProc on WM_DEFERRED_WEBVIEW_MESSAGE (WebView2 reentrancy workaround for modal dialogs)
void processDeferredWebViewMessage(LPARAM lParam);
// Called from MainWindowProc on WM_SETTINGCHANGE (theme change)
void notifyThemeChange();

} // namespace platform

#endif // PLATFORM_WINDOWS

#endif // WINDOWS_COMMON_H
