// Linux application lifecycle implementation
#include "../../../include/platform.h"
#include "../platform_impl.h"
#include "linux_window_events.h"
#include <X11/Xlib.h>
#include <gtk/gtk.h>
#include <cstdio>
#include <cstring>
#include <string>

#ifdef PLATFORM_LINUX

namespace platform {

static Display* g_display = nullptr;
static int g_screen = 0;

void initApplication() {
    if (!g_display) {
        g_display = XOpenDisplay(nullptr);
        if (g_display) {
            g_screen = DefaultScreen(g_display);
        }
    }
    // Initialize GTK for WebKit
    if (!gtk_is_initialized()) {
        gtk_init(nullptr, nullptr);
    }
}

void runApplication() {
    if (!g_display) {
        return;
    }
    
    XEvent event;
    while (true) {
        XNextEvent(g_display, &event);
        platform::dispatchFocusEvent(g_display, &event);
        platform::dispatchConfigureEvent(g_display, &event);
        platform::dispatchWindowStateEvent(g_display, &event);
        if (event.type == KeyPress) {
            KeySym keysym = XLookupKeysym(&event.xkey, 0);
            if (keysym == XK_Escape) {
                break;
            }
        }
        
        if (event.type == ClientMessage) {
            break;
        }
    }
}

void quitApplication() {
    if (g_display) {
        XCloseDisplay(g_display);
        g_display = nullptr;
    }
}

static void (*s_activateCb)(void*) = nullptr;
static void (*s_deactivateCb)(void*) = nullptr;
static void* s_activateUd = nullptr;
static void* s_deactivateUd = nullptr;

void setAppActivateCallback(void (*cb)(void*), void* ud) {
    s_activateCb = cb;
    s_activateUd = ud;
    platform::registerAppFocusCallbacks(s_activateCb, s_activateUd, s_deactivateCb, s_deactivateUd);
}

void setAppDeactivateCallback(void (*cb)(void*), void* ud) {
    s_deactivateCb = cb;
    s_deactivateUd = ud;
    platform::registerAppFocusCallbacks(s_activateCb, s_activateUd, s_deactivateCb, s_deactivateUd);
}

static void (*s_themeChangeCb)(const char*, void*) = nullptr;
static void* s_themeChangeUd = nullptr;

static const char* getLinuxTheme() {
    FILE* fp = popen("gsettings get org.gnome.desktop.interface color-scheme 2>/dev/null || gsettings get org.gnome.desktop.interface gtk-theme 2>/dev/null", "r");
    if (!fp) return "light";
    char buf[128];
    if (!fgets(buf, sizeof(buf), fp)) {
        pclose(fp);
        return "light";
    }
    pclose(fp);
    std::string s(buf);
    if (s.find("prefer-dark") != std::string::npos) return "dark";
    if (s.find("prefer-light") != std::string::npos) return "light";
    if (s.find("dark") != std::string::npos || s.find("Dark") != std::string::npos) return "dark";
    return "light";
}

void setThemeChangeCallback(void (*callback)(const char* theme, void* userData), void* userData) {
    s_themeChangeCb = callback;
    s_themeChangeUd = userData;
    if (callback) {
        callback(getLinuxTheme(), userData);
    }
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
        if (argv[i][0] != '-') s_appOpenFileCb(std::string(argv[i]), s_appOpenFileUd);
    }
}
    return g_display;
}

int getScreen() {
    return g_screen;
}

} // namespace platform

#endif // PLATFORM_LINUX
