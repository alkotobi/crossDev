// Linux window implementation
#include "../../../include/platform.h"
#include "../platform_impl.h"
#include "linux_window_events.h"
#include "nlohmann/json.hpp"
#include <string>
#include <map>
#include <vector>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <gtk/gtk.h>

namespace platform {
    Display* getDisplay();
    int getScreen();
    void initApplication();
}

#ifdef PLATFORM_LINUX

namespace platform {

typedef void (*ResizeCallback)(int width, int height, void* userData);
typedef void (*MoveCallback)(int x, int y, void* userData);
typedef void (*FocusCallback)(void* userData);
typedef void (*StateCallback)(const char* state, void* userData);
typedef void (*MenuItemCallback)(const std::string& itemId, void* userData);

struct WindowData {
    Display* display;
    Window window;
    GtkWidget* gtkWindow;
    bool visible;
    std::string title;
    ResizeCallback resizeCallback;
    void* resizeUserData;
    MoveCallback moveCallback;
    void* moveUserData;
    FocusCallback focusCallback;
    FocusCallback blurCallback;
    void* focusUserData;
    void* blurUserData;
    StateCallback stateCallback;
    void* stateUserData;
    MenuItemCallback menuItemCallback;
    void* menuUserData;
    GtkWidget* menuBar;
};

static std::map<Window, WindowData*> g_windowMap;

void* createWindow(int x, int y, int width, int height, const std::string& title, void* userData) {
    initApplication();
    
    if (!getDisplay()) {
        return nullptr;
    }
    
    WindowData* data = new WindowData;
    data->display = getDisplay();
    data->title = title;
    data->visible = false;
    data->gtkWindow = nullptr;
    data->resizeCallback = nullptr;
    data->resizeUserData = nullptr;
    data->moveCallback = nullptr;
    data->moveUserData = nullptr;
    data->focusCallback = nullptr;
    data->blurCallback = nullptr;
    data->focusUserData = nullptr;
    data->blurUserData = nullptr;
    data->stateCallback = nullptr;
    data->stateUserData = nullptr;
    data->menuItemCallback = nullptr;
    data->menuUserData = nullptr;
    data->menuBar = nullptr;
    
    Window root = RootWindow(data->display, getScreen());
    
    XSetWindowAttributes attrs;
    attrs.event_mask = ExposureMask | KeyPressMask | ButtonPressMask | StructureNotifyMask | FocusChangeMask | PropertyChangeMask;
    attrs.background_pixel = WhitePixel(data->display, getScreen());
    attrs.border_pixel = BlackPixel(data->display, getScreen());
    
    unsigned long attrmask = CWBackPixel | CWBorderPixel | CWEventMask;
    
    data->window = XCreateWindow(
        data->display,
        root,
        x, y, width, height,
        1,
        DefaultDepth(data->display, getScreen()),
        InputOutput,
        DefaultVisual(data->display, getScreen()),
        attrmask,
        &attrs
    );
    
    XStoreName(data->display, data->window, title.c_str());
    XMapWindow(data->display, data->window);
    g_windowMap[data->window] = data;
    
    return data;
}

void destroyWindow(void* handle) {
    if (handle) {
        WindowData* data = static_cast<WindowData*>(handle);
        if (data->display && data->window) {
            g_windowMap.erase(data->window);
            XDestroyWindow(data->display, data->window);
        }
        delete data;
    }
}

void showWindow(void* handle) {
    if (handle) {
        WindowData* data = static_cast<WindowData*>(handle);
        if (data->display && data->window) {
            XMapRaised(data->display, data->window);
            XFlush(data->display);
            data->visible = true;
        }
    }
}

void hideWindow(void* handle) {
    if (handle) {
        WindowData* data = static_cast<WindowData*>(handle);
        if (data->display && data->window) {
            XUnmapWindow(data->display, data->window);
            XFlush(data->display);
            data->visible = false;
        }
    }
}

void setWindowTitle(void* handle, const std::string& title) {
    if (handle) {
        WindowData* data = static_cast<WindowData*>(handle);
        data->title = title;
        if (data->display && data->window) {
            XStoreName(data->display, data->window, title.c_str());
            XFlush(data->display);
        }
    }
}

void maximizeWindow(void* handle) {
    if (!handle) return;
    WindowData* data = static_cast<WindowData*>(handle);
    if (!data->display || !data->window) return;
    Atom wmState = XInternAtom(data->display, "_NET_WM_STATE", False);
    Atom maxHorz = XInternAtom(data->display, "_NET_WM_STATE_MAXIMIZED_HORZ", False);
    Atom maxVert = XInternAtom(data->display, "_NET_WM_STATE_MAXIMIZED_VERT", False);
    if (wmState != None && maxHorz != None && maxVert != None) {
        Atom states[2] = { maxHorz, maxVert };
        XChangeProperty(data->display, data->window, wmState, XA_ATOM, 32,
            PropModeReplace, (unsigned char*)states, 2);
        XFlush(data->display);
    }
}

bool isWindowVisible(void* handle) {
    if (handle) {
        WindowData* data = static_cast<WindowData*>(handle);
        if (data->display && data->window) {
            XWindowAttributes attrs;
            if (XGetWindowAttributes(data->display, data->window, &attrs)) {
                return data->visible && (attrs.map_state == IsViewable);
            }
        }
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

void setWindowFileDropCallback(void*, void (*)(const std::string&, void*), void*) {
    // Linux: Xdnd support would require significant X11 implementation - stub for now
}

void setWindowMoveCallback(void* windowHandle, void (*callback)(int x, int y, void* userData), void* userData) {
    if (windowHandle && callback) {
        WindowData* data = static_cast<WindowData*>(windowHandle);
        data->moveCallback = callback;
        data->moveUserData = userData;
    }
}

void dispatchConfigureEvent(Display* dpy, XEvent* ev) {
    if (!dpy || !ev || ev->type != ConfigureNotify) return;
    Window xwin = ev->xconfigure.window;
    auto it = g_windowMap.find(xwin);
    if (it == g_windowMap.end()) return;
    WindowData* data = it->second;
    int x = ev->xconfigure.x;
    int y = ev->xconfigure.y;
    int w = ev->xconfigure.width;
    int h = ev->xconfigure.height;
    if (data->resizeCallback) {
        data->resizeCallback(w, h, data->resizeUserData);
    }
    if (data->moveCallback) {
        data->moveCallback(x, y, data->moveUserData);
    }
}

void setWindowCloseCallback(void*, void (*)(void*), void*) {
    // Linux: Close handling would need WM_DELETE_WINDOW - stub for now
}

void setWindowFocusCallback(void* windowHandle, void (*callback)(void*), void* userData) {
    if (windowHandle && callback) {
        WindowData* data = static_cast<WindowData*>(windowHandle);
        data->focusCallback = callback;
        data->focusUserData = userData;
    }
}

void setWindowBlurCallback(void* windowHandle, void (*callback)(void*), void* userData) {
    if (windowHandle && callback) {
        WindowData* data = static_cast<WindowData*>(windowHandle);
        data->blurCallback = callback;
        data->blurUserData = userData;
    }
}

void setWindowStateCallback(void* windowHandle, void (*callback)(const char*, void*), void* userData) {
    if (windowHandle) {
        WindowData* data = static_cast<WindowData*>(windowHandle);
        data->stateCallback = callback;
        data->stateUserData = userData;
    }
}

void dispatchWindowStateEvent(Display* dpy, XEvent* ev) {
    if (!dpy || !ev) return;
    if (ev->type == UnmapNotify) {
        Window xwin = ev->xunmap.window;
        auto it = g_windowMap.find(xwin);
        if (it != g_windowMap.end() && it->second->stateCallback) {
            it->second->stateCallback("minimize", it->second->stateUserData);
        }
        return;
    }
    if (ev->type == MapNotify) {
        Window xwin = ev->xmap.window;
        auto it = g_windowMap.find(xwin);
        if (it != g_windowMap.end() && it->second->stateCallback) {
            it->second->stateCallback("restore", it->second->stateUserData);
        }
        return;
    }
    if (ev->type == PropertyNotify) {
        Atom netWmState = XInternAtom(dpy, "_NET_WM_STATE", False);
        if (ev->xproperty.atom != netWmState) return;
        Window xwin = ev->xproperty.window;
        auto it = g_windowMap.find(xwin);
        if (it == g_windowMap.end() || !it->second->stateCallback) return;
        Atom type;
        int format;
        unsigned long nitems, bytes;
        unsigned char* prop = nullptr;
        if (XGetWindowProperty(dpy, xwin, netWmState, 0, 1024, False, XA_ATOM, &type, &format, &nitems, &bytes, &prop) != Success || !prop) return;
        Atom maxH = XInternAtom(dpy, "_NET_WM_STATE_MAXIMIZED_HORZ", False);
        Atom maxV = XInternAtom(dpy, "_NET_WM_STATE_MAXIMIZED_VERT", False);
        Atom* atoms = (Atom*)prop;
        bool maximized = false;
        for (unsigned long i = 0; i < nitems; i++) {
            if (atoms[i] == maxH || atoms[i] == maxV) { maximized = true; break; }
        }
        XFree(prop);
        it->second->stateCallback(maximized ? "maximize" : "restore", it->second->stateUserData);
    }
}

static void (*s_appActivateCb)(void*) = nullptr;
static void (*s_appDeactivateCb)(void*) = nullptr;
static void* s_appActivateUd = nullptr;
static void* s_appDeactivateUd = nullptr;
static bool s_appFocused = false;

void registerAppFocusCallbacks(void (*activate)(void*), void* activateUd,
                               void (*deactivate)(void*), void* deactivateUd) {
    s_appActivateCb = activate;
    s_appActivateUd = activateUd;
    s_appDeactivateCb = deactivate;
    s_appDeactivateUd = deactivateUd;
}

void dispatchFocusEvent(Display* dpy, XEvent* ev) {
    if (!dpy || !ev) return;
    if (ev->type != FocusIn && ev->type != FocusOut) return;
    Window xwin = ev->xfocus.window;
    auto it = g_windowMap.find(xwin);
    if (it == g_windowMap.end()) return;
    WindowData* data = it->second;
    if (ev->type == FocusIn) {
        if (data->focusCallback) data->focusCallback(data->focusUserData);
        if (!s_appFocused && s_appActivateCb) {
            s_appFocused = true;
            s_appActivateCb(s_appActivateUd);
        }
    } else if (ev->type == FocusOut) {
        if (data->blurCallback) data->blurCallback(data->blurUserData);
        Window currentFocus;
        int revert;
        XGetInputFocus(dpy, &currentFocus, &revert);
        bool focusInOurApp = false;
        if (currentFocus != None) {
            if (g_windowMap.count(currentFocus) != 0) {
                focusInOurApp = true;
            } else {
                Window w = currentFocus;
                while (w) {
                    Window root, parent;
                    Window* children;
                    unsigned int n;
                    if (!XQueryTree(dpy, w, &root, &parent, &children, &n)) break;
                    if (children) XFree(children);
                    if (g_windowMap.count(parent) != 0) { focusInOurApp = true; break; }
                    if (parent == root) break;
                    w = parent;
                }
            }
        }
        if (s_appFocused && !focusInOurApp && s_appDeactivateCb) {
            s_appFocused = false;
            s_appDeactivateCb(s_appDeactivateUd);
        }
    }
}

struct MenuActivateData {
    void (*callback)(const std::string&, void*);
    void* userData;
    std::string itemId;
};

static void onMenuItemActivate(GtkMenuItem* item, gpointer userData) {
    (void)item;
    MenuActivateData* data = static_cast<MenuActivateData*>(userData);
    if (data && data->callback) {
        data->callback(data->itemId, data->userData);
    }
}

static std::vector<MenuActivateData*> s_menuActivateData;

static void addMenuItems(GtkMenu* menu, const nlohmann::json& items,
                         void (*callback)(const std::string&, void*), void* userData,
                         std::vector<MenuActivateData*>* outData = nullptr) {
    for (const auto& item : items) {
        std::string idStr = item.value("id", "");
        std::string label = item.value("label", "");
        if (idStr == "-" || label == "-") {
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
            continue;
        }
        if (item.contains("items")) {
            GtkWidget* subItem = gtk_menu_item_new_with_label(label.c_str());
            GtkWidget* subMenu = gtk_menu_new();
            addMenuItems(GTK_MENU(subMenu), item["items"], callback, userData, outData);
            gtk_menu_item_set_submenu(GTK_MENU_ITEM(subItem), subMenu);
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), subItem);
        } else {
            GtkWidget* mi = gtk_menu_item_new_with_label(label.c_str());
            MenuActivateData* ad = new MenuActivateData{callback, userData, idStr};
            if (outData) outData->push_back(ad);
            else s_menuActivateData.push_back(ad);
            g_signal_connect(mi, "activate", G_CALLBACK(onMenuItemActivate), ad);
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), mi);
        }
    }
}

void setWindowMainMenu(void* windowHandle, const std::string& menuJson,
                       void (*itemCallback)(const std::string& itemId, void* userData), void* userData) {
    if (!windowHandle || !itemCallback || menuJson.empty()) return;
    WindowData* data = static_cast<WindowData*>(windowHandle);
    if (!data->gtkWindow) return;
    try {
        auto menus = nlohmann::json::parse(menuJson);
        if (!menus.is_array()) return;
        for (MenuActivateData* ad : s_menuActivateData) delete ad;
        s_menuActivateData.clear();
        if (data->menuBar) {
            gtk_widget_destroy(data->menuBar);
        }
        data->menuItemCallback = itemCallback;
        data->menuUserData = userData;
        data->menuBar = gtk_menu_bar_new();
        for (const auto& top : menus) {
            std::string label = top.value("label", "");
            if (label.empty()) continue;
            GtkWidget* topItem = gtk_menu_item_new_with_label(label.c_str());
            GtkWidget* subMenu = gtk_menu_new();
            if (top.contains("items")) {
                addMenuItems(GTK_MENU(subMenu), top["items"], itemCallback, userData, nullptr);
            }
            gtk_menu_item_set_submenu(GTK_MENU_ITEM(topItem), subMenu);
            gtk_menu_shell_append(GTK_MENU_SHELL(data->menuBar), topItem);
        }
        GtkWidget* child = gtk_bin_get_child(GTK_BIN(data->gtkWindow));
        if (child) {
            gtk_container_remove(GTK_CONTAINER(data->gtkWindow), child);
        }
        GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
        gtk_box_pack_start(GTK_BOX(vbox), data->menuBar, FALSE, FALSE, 0);
        if (child) {
            gtk_box_pack_start(GTK_BOX(vbox), child, TRUE, TRUE, 0);
        }
        gtk_container_add(GTK_CONTAINER(data->gtkWindow), vbox);
        gtk_widget_show_all(vbox);
    } catch (...) {}
}

void showContextMenu(void* parentHandle, int x, int y, const std::string& itemsJson,
                    void (*itemCallback)(const std::string& itemId, void* userData), void* userData) {
    (void)x;
    (void)y;
    if (!parentHandle || !itemCallback || itemsJson.empty()) return;
    GtkWidget* widget = nullptr;
    if (GTK_IS_WIDGET(parentHandle)) {
        widget = (GtkWidget*)parentHandle;
    } else {
        WindowData* data = static_cast<WindowData*>(parentHandle);
        if (data && data->gtkWindow) widget = data->gtkWindow;
    }
    if (!widget) return;
    try {
        auto items = nlohmann::json::parse(itemsJson);
        if (!items.is_array()) return;
        std::vector<MenuActivateData*> ctxData;
        GtkWidget* menu = gtk_menu_new();
        addMenuItems(GTK_MENU(menu), items, itemCallback, userData, &ctxData);
        g_signal_connect(menu, "selection-done", G_CALLBACK(+[](GtkMenu*, gpointer p) {
            auto* vec = static_cast<std::vector<MenuActivateData*>*>(p);
            for (MenuActivateData* ad : *vec) delete ad;
        }), &ctxData);
        gtk_widget_show_all(menu);
        gtk_menu_popup_at_pointer(GTK_MENU(menu), nullptr);
    } catch (...) {}
}

} // namespace platform

#endif // PLATFORM_LINUX
