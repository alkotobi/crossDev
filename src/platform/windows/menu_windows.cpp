// Windows menu implementation (main menu bar + context menu)
#include "../../../include/platform.h"
#include "../platform_impl.h"
#include "windows_common.h"
#include "nlohmann/json.hpp"
#include <string>
#include <windows.h>

#ifdef PLATFORM_WINDOWS

namespace platform {

static std::wstring stringToWString(const std::string& str) {
    if (str.empty()) return std::wstring();
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

// Format shortcut for display (menu expects Ctrl on Windows, Cmd on Mac)
static std::wstring formatShortcutForDisplay(const std::string& shortcut) {
    if (shortcut.empty()) return L"";
    std::wstring ws = stringToWString(shortcut);
    return ws;
}

static UINT s_nextMenuId = 10000;
static const UINT CONTEXT_MENU_ID_BASE = 20000;

// Add menu items to a popup menu; returns next available ID
static UINT addMenuItems(HMENU popup, const nlohmann::json& items,
                         std::map<UINT, std::string>& idMap,
                         UINT idBase, UINT& nextId) {
    for (const auto& item : items) {
        std::string idStr = item.value("id", "");
        std::string label = item.value("label", "");
        if (idStr == "-" || label == "-") {
            AppendMenuW(popup, MF_SEPARATOR, 0, nullptr);
            continue;
        }
        if (item.contains("items")) {
            HMENU subMenu = CreatePopupMenu();
            nextId = addMenuItems(subMenu, item["items"], idMap, idBase, nextId);
            std::wstring wlabel = stringToWString(label);
            AppendMenuW(popup, MF_POPUP | MF_STRING, (UINT_PTR)subMenu, wlabel.c_str());
        } else {
            std::string shortcut = item.value("shortcut", "");
            std::wstring wlabel = stringToWString(label);
            std::wstring wkey = formatShortcutForDisplay(shortcut);
            UINT id = nextId++;
            idMap[id] = idStr;
            if (!wkey.empty()) {
                AppendMenuW(popup, MF_STRING, id, (wlabel + L"\t" + wkey).c_str());
            } else {
                AppendMenuW(popup, MF_STRING, id, wlabel.c_str());
            }
        }
    }
    return nextId;
}

void setWindowMainMenu(void* windowHandle, const std::string& menuJson,
                      void (*itemCallback)(const std::string& itemId, void* userData), void* userData) {
    if (!windowHandle || !itemCallback || menuJson.empty()) return;
    WindowData* data = static_cast<WindowData*>(windowHandle);
    if (!data->hwnd) return;
    try {
        auto menus = nlohmann::json::parse(menuJson);
        if (!menus.is_array()) return;
        HMENU mainMenu = CreateMenu();
        data->menuIdToItemId.clear();
        UINT nextId = s_nextMenuId;
        for (const auto& top : menus) {
            std::string label = top.value("label", "");
            if (label.empty()) continue;
            HMENU subMenu = CreatePopupMenu();
            if (top.contains("items")) {
                nextId = addMenuItems(subMenu, top["items"], data->menuIdToItemId, s_nextMenuId, nextId);
            }
            std::wstring wlabel = stringToWString(label);
            AppendMenuW(mainMenu, MF_POPUP | MF_STRING, (UINT_PTR)subMenu, wlabel.c_str());
        }
        s_nextMenuId = nextId;
        data->menuItemCallback = itemCallback;
        data->menuUserData = userData;
        SetMenu(data->hwnd, mainMenu);
    } catch (...) {}
}

void showContextMenu(void* parentHandle, int x, int y, const std::string& itemsJson,
                    void (*itemCallback)(const std::string& itemId, void* userData), void* userData) {
    if (!parentHandle || !itemCallback || itemsJson.empty()) return;
    HWND hwnd = nullptr;
    WindowData* data = nullptr;
    if (IsWindow((HWND)parentHandle)) {
        hwnd = (HWND)parentHandle;
        auto it = g_windowMap.find(hwnd);
        if (it != g_windowMap.end()) data = it->second;
    } else {
        data = static_cast<WindowData*>(parentHandle);
        if (data && data->hwnd) hwnd = data->hwnd;
    }
    if (!hwnd) return;
    try {
        auto items = nlohmann::json::parse(itemsJson);
        if (!items.is_array()) return;
        HMENU menu = CreatePopupMenu();
        std::map<UINT, std::string> idMap;
        UINT nextId = CONTEXT_MENU_ID_BASE;
        addMenuItems(menu, items, idMap, CONTEXT_MENU_ID_BASE, nextId);
        if (data) {
            data->contextMenuIdToItemId = std::move(idMap);
            data->contextMenuCallback = itemCallback;
            data->contextMenuUserData = userData;
        }
        POINT pt = { x, y };
        ClientToScreen(hwnd, &pt);
        TrackPopupMenu(menu, TPM_LEFTALIGN | TPM_TOPALIGN, pt.x, pt.y, 0, hwnd, nullptr);
        DestroyMenu(menu);
        if (data) {
            data->contextMenuCallback = nullptr;
            data->contextMenuUserData = nullptr;
            data->contextMenuIdToItemId.clear();
        }
    } catch (...) {}
}

} // namespace platform

#endif // PLATFORM_WINDOWS
