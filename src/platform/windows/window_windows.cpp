// Windows window implementation
#include "../../../include/platform.h"
#include "../platform_impl.h"
#include "windows_common.h"
#include <string>

// Forward declarations
namespace platform {
    HINSTANCE getInstance();
    const wchar_t* getClassName();
    void initApplication();
    LRESULT CALLBACK MainWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
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

// Global map to track all windows (exported for use in button_windows.cpp)
std::map<HWND, WindowData*> g_windowMap;
WindowData* g_mainWindow = nullptr;  // Track the main window

void* createWindow(int x, int y, int width, int height, const std::string& title, void* userData) {
    initApplication();
    
    WindowData* data = new WindowData;
    data->title = title;
    data->visible = false;
    data->isMainWindow = (g_mainWindow == nullptr);  // First window is the main window
    data->userData = userData;  // Store Window* pointer for cleanup
    data->resizeCallback = nullptr;
    data->resizeUserData = nullptr;
    data->moveCallback = nullptr;
    data->moveUserData = nullptr;
    data->fileDropCallback = nullptr;
    data->fileDropUserData = nullptr;
    data->closeCallback = nullptr;
    data->closeUserData = nullptr;
    data->beingDestroyed = false;
    data->focusCallback = nullptr;
    data->blurCallback = nullptr;
    data->focusUserData = nullptr;
    data->blurUserData = nullptr;
    data->stateCallback = nullptr;
    data->stateUserData = nullptr;
    data->menuItemCallback = nullptr;
    data->menuUserData = nullptr;
    data->contextMenuCallback = nullptr;
    data->contextMenuUserData = nullptr;
    
    std::wstring wtitle = stringToWString(title);
    HWND hwnd = CreateWindowExW(
        0,
        getClassName(),
        wtitle.c_str(),
        WS_OVERLAPPEDWINDOW,
        x, y, width, height,
        nullptr,
        nullptr,
        getInstance(),
        nullptr
    );
    
    if (!hwnd) {
        delete data;
        return nullptr;
    }
    
    data->hwnd = hwnd;
    g_windowMap[hwnd] = data;
    
    if (data->isMainWindow) {
        g_mainWindow = data;
    }
    
    return data;
}

void destroyWindow(void* handle) {
    if (handle) {
        WindowData* data = static_cast<WindowData*>(handle);
        if (data->hwnd) {
            g_windowMap.erase(data->hwnd);
            if (g_mainWindow == data) {
                g_mainWindow = nullptr;
            }
            if (!data->beingDestroyed) {
                DestroyWindow(data->hwnd);
            }
        }
        delete data;
    }
}

void showWindow(void* handle) {
    if (handle) {
        WindowData* data = static_cast<WindowData*>(handle);
        ShowWindow(data->hwnd, SW_SHOW);
        UpdateWindow(data->hwnd);
        data->visible = true;
        // Bring to foreground (needed for "Return to Car Management" from child window)
        BringWindowToTop(data->hwnd);
        SetWindowPos(data->hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        if (!SetForegroundWindow(data->hwnd)) {
            // AttachThreadInput workaround when switching from child window
            HWND fg = GetForegroundWindow();
            if (fg && fg != data->hwnd) {
                DWORD foreThread = GetWindowThreadProcessId(fg, NULL);
                DWORD currThread = GetCurrentThreadId();
                if (foreThread != currThread) {
                    AttachThreadInput(currThread, foreThread, TRUE);
                    SetForegroundWindow(data->hwnd);
                    AttachThreadInput(currThread, foreThread, FALSE);
                }
            }
        }
        // Defer resize to next message loop so window is fully laid out
        if (data->resizeCallback && data->resizeUserData) {
            PostMessage(data->hwnd, WM_DEFERRED_RESIZE, 0, 0);
            SetTimer(data->hwnd, IDT_DEFERRED_RESIZE, 150, nullptr);  // Retry after WebView2 init
        }
    }
}

void maximizeWindow(void* handle) {
    if (handle) {
        WindowData* data = static_cast<WindowData*>(handle);
        ShowWindow(data->hwnd, SW_MAXIMIZE);
        UpdateWindow(data->hwnd);
        // Defer resize so maximized layout is complete before we measure
        if (data->resizeCallback && data->resizeUserData) {
            PostMessage(data->hwnd, WM_DEFERRED_RESIZE, 0, 0);
            SetTimer(data->hwnd, IDT_DEFERRED_RESIZE, 150, nullptr);  // Retry after WebView2 init
        }
    }
}

void hideWindow(void* handle) {
    if (handle) {
        WindowData* data = static_cast<WindowData*>(handle);
        ShowWindow(data->hwnd, SW_HIDE);
        data->visible = false;
    }
}

void setWindowTitle(void* handle, const std::string& title) {
    if (handle) {
        WindowData* data = static_cast<WindowData*>(handle);
        data->title = title;
        std::wstring wtitle = stringToWString(title);
        SetWindowTextW(data->hwnd, wtitle.c_str());
    }
}

bool isWindowVisible(void* handle) {
    if (handle) {
        WindowData* data = static_cast<WindowData*>(handle);
        return data->visible && IsWindowVisible(data->hwnd);
    }
    return false;
}

void setWindowResizeCallback(void* windowHandle, void (*callback)(int width, int height, void* userData), void* userData) {
    if (windowHandle && callback) {
        WindowData* data = static_cast<WindowData*>(windowHandle);
        data->resizeCallback = callback;
        data->resizeUserData = userData;
    }
}

void setWindowMoveCallback(void* windowHandle, void (*callback)(int x, int y, void* userData), void* userData) {
    if (windowHandle && callback) {
        WindowData* data = static_cast<WindowData*>(windowHandle);
        data->moveCallback = callback;
        data->moveUserData = userData;
    }
}

void setWindowFileDropCallback(void* windowHandle, void (*callback)(const std::string& pathsJson, void* userData), void* userData) {
    if (windowHandle) {
        WindowData* data = static_cast<WindowData*>(windowHandle);
        data->fileDropCallback = callback;
        data->fileDropUserData = userData;
        if (data->hwnd) {
            DragAcceptFiles(data->hwnd, callback ? TRUE : FALSE);
        }
    }
}

void setWindowCloseCallback(void* windowHandle, void (*callback)(void* userData), void* userData) {
    if (windowHandle) {
        WindowData* data = static_cast<WindowData*>(windowHandle);
        data->closeCallback = callback;
        data->closeUserData = userData;
    }
}

void setWindowFocusCallback(void* windowHandle, void (*callback)(void*), void* userData) {
    if (windowHandle) {
        WindowData* data = static_cast<WindowData*>(windowHandle);
        data->focusCallback = callback;
        data->focusUserData = userData;
    }
}

void setWindowBlurCallback(void* windowHandle, void (*callback)(void*), void* userData) {
    if (windowHandle) {
        WindowData* data = static_cast<WindowData*>(windowHandle);
        data->blurCallback = callback;
        data->blurUserData = userData;
    }
}

void setWindowStateCallback(void* windowHandle, void (*callback)(const char* state, void* userData), void* userData) {
    if (windowHandle) {
        WindowData* data = static_cast<WindowData*>(windowHandle);
        data->stateCallback = callback;
        data->stateUserData = userData;
    }
}

} // namespace platform

#endif // PLATFORM_WINDOWS
