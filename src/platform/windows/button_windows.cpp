// Windows button implementation
#include "../../../include/platform.h"
#include "../../../include/window.h"
#include "../platform_impl.h"
#include "windows_common.h"
#include <string>
#include <windows.h>
#include <shellapi.h>

namespace platform {
    HINSTANCE getInstance();
    const wchar_t* getClassName();
}

// Helper function to convert std::string to std::wstring
static std::wstring stringToWString(const std::string& str) {
    if (str.empty()) return std::wstring();
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

#ifdef PLATFORM_WINDOWS

namespace platform {

struct ButtonData {
    HWND hwnd;
    void* userData;
    void (*callback)(void*);
};

// Global button map for callback handling
std::map<HWND, ButtonData*> g_buttonMap;

void* createButton(void* parentHandle, int x, int y, int width, int height, const std::string& label, void* userData) {
    if (!parentHandle) {
        return nullptr;
    }
    
    HWND parentHwnd = nullptr;
    
    // Get parent HWND - could be from WindowData or direct HWND (from Container)
    if (IsWindow((HWND)parentHandle)) {
        parentHwnd = (HWND)parentHandle;
    } else {
        // Try to get from WindowData
        WindowData* windowData = static_cast<WindowData*>(parentHandle);
        if (windowData && windowData->hwnd) {
            parentHwnd = windowData->hwnd;
        } else {
            return nullptr;
        }
    }
    
    ButtonData* buttonData = new ButtonData;
    buttonData->userData = userData;
    buttonData->callback = nullptr;
    
    std::wstring wlabel = stringToWString(label);
    buttonData->hwnd = CreateWindowExW(
        0,
        L"BUTTON",
        wlabel.c_str(),
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        x, y, width, height,
        parentHwnd,
        nullptr,
        getInstance(),
        nullptr
    );
    
    // Store button in map for callback lookup
    g_buttonMap[buttonData->hwnd] = buttonData;
    
    return buttonData;
}

void destroyButton(void* buttonHandle) {
    if (buttonHandle) {
        ButtonData* data = static_cast<ButtonData*>(buttonHandle);
        if (data->hwnd) {
            g_buttonMap.erase(data->hwnd);
            DestroyWindow(data->hwnd);
        }
        delete data;
    }
}

void setButtonCallback(void* buttonHandle, void (*callback)(void*)) {
    if (buttonHandle) {
        ButtonData* data = static_cast<ButtonData*>(buttonHandle);
        data->callback = callback;
    }
}

// Window procedure - handles button callbacks and window messages
LRESULT CALLBACK MainWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_ACTIVATEAPP) {
        platform::notifyAppActivate(wParam != 0);
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    if (uMsg == WM_SETTINGCHANGE) {
        platform::notifyThemeChange();
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    if (uMsg == WM_ACTIVATE) {
        auto it = platform::g_windowMap.find(hwnd);
        if (it != platform::g_windowMap.end()) {
            WindowData* windowData = it->second;
            if (LOWORD(wParam) != WA_INACTIVE) {
                if (windowData->focusCallback && windowData->focusUserData) {
                    windowData->focusCallback(windowData->focusUserData);
                }
            } else {
                if (windowData->blurCallback && windowData->blurUserData) {
                    windowData->blurCallback(windowData->blurUserData);
                }
            }
        }
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    if (uMsg == WM_DROPFILES) {
        auto it = platform::g_windowMap.find(hwnd);
        if (it != platform::g_windowMap.end()) {
            WindowData* windowData = it->second;
            if (windowData->fileDropCallback) {
                HDROP hDrop = (HDROP)wParam;
                UINT count = DragQueryFileW(hDrop, 0xFFFFFFFF, nullptr, 0);
                std::string pathsJson = "[";
                wchar_t pathBuf[MAX_PATH];
                for (UINT i = 0; i < count; ++i) {
                    if (DragQueryFileW(hDrop, i, pathBuf, MAX_PATH)) {
                        int len = WideCharToMultiByte(CP_UTF8, 0, pathBuf, -1, nullptr, 0, nullptr, nullptr);
                        if (len > 0) {
                            std::string path(len, 0);
                            WideCharToMultiByte(CP_UTF8, 0, pathBuf, -1, &path[0], len, nullptr, nullptr);
                            path.resize(len - 1);
                            if (i > 0) pathsJson += ",";
                            pathsJson += "\"";
                            for (char c : path) {
                                if (c == '\\') pathsJson += "\\\\";
                                else if (c == '"') pathsJson += "\\\"";
                                else pathsJson += c;
                            }
                            pathsJson += "\"";
                        }
                    }
                }
                pathsJson += "]";
                DragFinish(hDrop);
                windowData->fileDropCallback(pathsJson, windowData->fileDropUserData);
            }
        }
        return 0;
    }
    if (uMsg == WM_MOVE) {
        auto it = platform::g_windowMap.find(hwnd);
        if (it != platform::g_windowMap.end()) {
            WindowData* windowData = it->second;
            if (windowData->moveCallback) {
                int x = (short)LOWORD(lParam);
                int y = (short)HIWORD(lParam);
                windowData->moveCallback(x, y, windowData->moveUserData);
            }
        }
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    if (uMsg == WM_DEFERRED_WEBVIEW_MESSAGE) {
        platform::processDeferredWebViewMessage(lParam);
        return 0;
    }
    if (uMsg == WM_DEFERRED_RESIZE) {
        auto it = platform::g_windowMap.find(hwnd);
        if (it != platform::g_windowMap.end()) {
            WindowData* data = it->second;
            if (data->resizeCallback && data->resizeUserData) {
                RECT client;
                if (GetClientRect(hwnd, &client)) {
                    int w = client.right - client.left;
                    int h = client.bottom - client.top;
                    if (w > 0 && h > 0) {
                        data->resizeCallback(w, h, data->resizeUserData);
                    }
                }
            }
        }
        return 0;
    }
    if (uMsg == WM_TIMER && wParam == IDT_DEFERRED_RESIZE) {
        KillTimer(hwnd, IDT_DEFERRED_RESIZE);
        auto it = platform::g_windowMap.find(hwnd);
        if (it != platform::g_windowMap.end()) {
            WindowData* data = it->second;
            if (data->resizeCallback && data->resizeUserData) {
                RECT client;
                if (GetClientRect(hwnd, &client)) {
                    int w = client.right - client.left;
                    int h = client.bottom - client.top;
                    if (w > 0 && h > 0) {
                        data->resizeCallback(w, h, data->resizeUserData);
                    }
                }
            }
        }
        return 0;
    }
    if (uMsg == WM_SIZE) {
        auto it = platform::g_windowMap.find(hwnd);
        if (it != platform::g_windowMap.end()) {
            WindowData* windowData = it->second;
            if (windowData->stateCallback) {
                WPARAM w = wParam;
                if (w == SIZE_MINIMIZED) windowData->stateCallback("minimize", windowData->stateUserData);
                else if (w == SIZE_MAXIMIZED) windowData->stateCallback("maximize", windowData->stateUserData);
                else if (w == SIZE_RESTORED) windowData->stateCallback("restore", windowData->stateUserData);
            }
            if (windowData->resizeCallback) {
                int width = LOWORD(lParam);
                int height = HIWORD(lParam);
                if (wParam != SIZE_MINIMIZED) {
                    windowData->resizeCallback(width, height, windowData->resizeUserData);
                }
            }
        }
        return 0;
    }
    if (uMsg == WM_COMMAND) {
        HWND buttonHwnd = (HWND)lParam;
        if (lParam != 0) {
            auto it = g_buttonMap.find(buttonHwnd);
            if (it != g_buttonMap.end() && it->second->callback) {
                it->second->callback(it->second->userData);
                return 0;
            }
        } else {
            UINT menuId = LOWORD(wParam);
            auto wit = platform::g_windowMap.find(hwnd);
            if (wit != platform::g_windowMap.end()) {
                WindowData* wd = wit->second;
                auto mit = wd->contextMenuIdToItemId.find(menuId);
                if (mit != wd->contextMenuIdToItemId.end() && wd->contextMenuCallback) {
                    wd->contextMenuCallback(mit->second, wd->contextMenuUserData);
                    return 0;
                }
                mit = wd->menuIdToItemId.find(menuId);
                if (mit != wd->menuIdToItemId.end() && wd->menuItemCallback) {
                    wd->menuItemCallback(mit->second, wd->menuUserData);
                    return 0;
                }
            }
        }
    }
    if (uMsg == WM_DESTROY) {
        auto it = platform::g_windowMap.find(hwnd);
        if (it != platform::g_windowMap.end()) {
            WindowData* windowData = it->second;
            
            if (windowData == platform::g_mainWindow) {
                // Main window - quit the application
                PostQuitMessage(0);
            } else if (windowData->closeCallback && windowData->closeUserData) {
                // Child WebViewWindow - use close callback to delete it properly
                windowData->beingDestroyed = true;
                windowData->closeCallback(windowData->closeUserData);
            }
        }
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

} // namespace platform

#endif // PLATFORM_WINDOWS
