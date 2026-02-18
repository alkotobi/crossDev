// Windows container implementation
#include "../../../include/platform.h"
#include "../platform_impl.h"
#include "windows_common.h"
#include <windows.h>
#include <string>

#ifdef PLATFORM_WINDOWS

namespace platform {

void* createContainer(void* parentHandle, int x, int y, int width, int height, bool flipped) {
    (void)flipped;
    if (!parentHandle) {
        return nullptr;
    }
    
    HWND parentHwnd = nullptr;
    
    // Get parent HWND - could be from WindowData or direct HWND
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
    
    // Create a static control as container
    HWND hwnd = CreateWindowExW(
        WS_EX_CONTROLPARENT,
        L"STATIC",
        L"",
        WS_CHILD | WS_VISIBLE | SS_WHITERECT,
        x, y, width, height,
        parentHwnd,
        nullptr,
        GetModuleHandle(nullptr),
        nullptr
    );
    
    if (!hwnd) {
        return nullptr;
    }
    
    return hwnd;
}

void destroyContainer(void* containerHandle) {
    if (containerHandle && IsWindow((HWND)containerHandle)) {
        DestroyWindow((HWND)containerHandle);
    }
}

void resizeContainer(void* containerHandle, int x, int y, int width, int height) {
    if (containerHandle && IsWindow((HWND)containerHandle)) {
        SetWindowPos((HWND)containerHandle, nullptr, x, y, width, height, SWP_NOZORDER);
    }
}

void showContainer(void* containerHandle) {
    if (containerHandle && IsWindow((HWND)containerHandle)) {
        ShowWindow((HWND)containerHandle, SW_SHOW);
    }
}

void hideContainer(void* containerHandle) {
    if (containerHandle && IsWindow((HWND)containerHandle)) {
        ShowWindow((HWND)containerHandle, SW_HIDE);
    }
}

void bringContainerToFront(void* containerHandle) {
    if (containerHandle && IsWindow((HWND)containerHandle)) {
        SetWindowPos((HWND)containerHandle, HWND_TOP, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    }
}

void setContainerBackgroundColor(void* containerHandle, int red, int green, int blue) {
    if (containerHandle && IsWindow((HWND)containerHandle)) {
        HWND hwnd = (HWND)containerHandle;
        HBRUSH brush = CreateSolidBrush(RGB(red, green, blue));
        SetClassLongPtr(hwnd, GCLP_HBRBACKGROUND, (LONG_PTR)brush);
        InvalidateRect(hwnd, nullptr, TRUE);
    }
}

void setContainerBorderStyle(void* containerHandle, int borderStyle) {
    if (containerHandle && IsWindow((HWND)containerHandle)) {
        HWND hwnd = (HWND)containerHandle;
        LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);
        
        // Remove existing border styles
        style &= ~(WS_BORDER | WS_DLGFRAME);
        
        if (borderStyle == 1) { // BorderSingle
            style |= WS_BORDER;
        } else if (borderStyle == 2) { // BorderDouble
            style |= WS_DLGFRAME;
        }
        
        SetWindowLongPtr(hwnd, GWL_STYLE, style);
        SetWindowPos(hwnd, nullptr, 0, 0, 0, 0, 
                    SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
    }
}

} // namespace platform

#endif // PLATFORM_WINDOWS
