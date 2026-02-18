// Windows input field implementation
#include "../../../include/platform.h"
#include "../platform_impl.h"
#include "windows_common.h"
#include <string>
#include <windows.h>
#include <commctrl.h>

namespace platform {
    HINSTANCE getInstance();
}

// Helper function to convert std::string to std::wstring
static std::wstring stringToWString(const std::string& str) {
    if (str.empty()) return std::wstring();
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

// Helper function to convert std::wstring to std::string
static std::string wStringToString(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

#ifdef PLATFORM_WINDOWS

namespace platform {

void* createInputField(void* parentHandle, int x, int y, int width, int height, const std::string& placeholder) {
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
    
    std::wstring wplaceholder = stringToWString(placeholder);
    HWND hwnd = CreateWindowW(
        L"EDIT",
        wplaceholder.c_str(),
        WS_VISIBLE | WS_CHILD | WS_BORDER | ES_LEFT | ES_AUTOHSCROLL,
        x, y, width, height,
        parentHwnd,
        nullptr,
        getInstance(),
        nullptr
    );
    
    if (!hwnd) {
        return nullptr;
    }
    
    // Set placeholder text as hint (requires Windows Vista+)
    #ifndef EM_SETCUEBANNER
    #define EM_SETCUEBANNER (ECM_FIRST + 1)
    #endif
    SendMessageW(hwnd, EM_SETCUEBANNER, TRUE, (LPARAM)wplaceholder.c_str());
    
    return (void*)hwnd;
}

void destroyInputField(void* inputHandle) {
    if (inputHandle) {
        HWND hwnd = (HWND)inputHandle;
        DestroyWindow(hwnd);
    }
}

void setInputText(void* inputHandle, const std::string& text) {
    if (inputHandle) {
        HWND hwnd = (HWND)inputHandle;
        std::wstring wtext = stringToWString(text);
        SetWindowTextW(hwnd, wtext.c_str());
    }
}

std::string getInputText(void* inputHandle) {
    if (inputHandle) {
        HWND hwnd = (HWND)inputHandle;
        int length = GetWindowTextLengthW(hwnd);
        if (length > 0) {
            std::wstring wtext(length + 1, L'\0');
            GetWindowTextW(hwnd, &wtext[0], length + 1);
            wtext.resize(length);
            return wStringToString(wtext);
        }
    }
    return "";
}

} // namespace platform

#endif // PLATFORM_WINDOWS
