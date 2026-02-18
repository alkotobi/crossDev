// Windows file dialog implementation
#include "../../../include/platform.h"
#include "../platform_impl.h"
#include <string>
#include <windows.h>
#include <commdlg.h>
#include <vector>

#ifdef PLATFORM_WINDOWS

namespace platform {

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

bool showOpenFileDialog(void* windowHandle, const std::string& title, const std::string& filter, std::string& selectedPath) {
    OPENFILENAMEW ofn;
    wchar_t szFile[260] = { 0 };
    
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    
    // Set parent window if provided. Use NULL when invoked from child window (Settings etc.)
    // to avoid modality/focus issues - dialog will be application-modal instead.
    if (windowHandle) {
        // Get HWND from WindowData (platform window handle structure)
        struct WindowData {
            HWND hwnd;
            bool visible;
            std::string title;
        };
        WindowData* windowData = static_cast<WindowData*>(windowHandle);
        if (windowData && windowData->hwnd && IsWindow(windowData->hwnd)) {
            ofn.hwndOwner = windowData->hwnd;
        }
    }
    
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile) / sizeof(wchar_t);
    
    // Set filter
    std::wstring filterStr = L"HTML Files (*.html)\0*.html\0All Files (*.*)\0*.*\0\0";
    if (!filter.empty()) {
        std::wstring wfilter = stringToWString(filter);
        // Convert filter format (replace | with \0)
        std::vector<wchar_t> filterVec;
        for (size_t i = 0; i < wfilter.length(); ++i) {
            if (wfilter[i] == L'|') {
                filterVec.push_back(L'\0');
            } else {
                filterVec.push_back(wfilter[i]);
            }
        }
        filterVec.push_back(L'\0');
        filterVec.push_back(L'\0');
        filterStr = std::wstring(filterVec.data(), filterVec.size());
    }
    
    // Allocate memory for filter string
    std::vector<wchar_t> filterBuffer(filterStr.begin(), filterStr.end());
    filterBuffer.push_back(L'\0');
    ofn.lpstrFilter = filterBuffer.data();
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    
    // Set title
    std::wstring wtitle;
    if (!title.empty()) {
        wtitle = stringToWString(title);
    } else {
        wtitle = L"Open HTML File";
    }
    ofn.lpstrTitle = wtitle.c_str();
    
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    
    if (GetOpenFileNameW(&ofn) == TRUE) {
        selectedPath = wStringToString(szFile);
        return true;
    }
    
    return false;
}

} // namespace platform

#endif // PLATFORM_WINDOWS
