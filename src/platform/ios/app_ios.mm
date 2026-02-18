// iOS application lifecycle implementation
#import <UIKit/UIKit.h>
#include "../../../include/platform.h"
#include "../platform_impl.h"

#ifdef PLATFORM_IOS

namespace platform {

static UIApplication* g_app = nullptr;
static UIWindow* g_mainWindow = nullptr;

void initApplication() {
    @autoreleasepool {
        if (!g_app) {
            g_app = [UIApplication sharedApplication];
        }
    }
}

void runApplication() {
    // On iOS, UIApplicationMain handles the event loop
    // This is called from the main function via UIApplicationMain
    // So this function may not be called directly
    @autoreleasepool {
        if (g_app) {
            // The run loop is managed by UIKit
        }
    }
}

void quitApplication() {
    @autoreleasepool {
        // On iOS, apps don't typically quit programmatically
        // But we can hide the window
        if (g_mainWindow) {
            g_mainWindow.hidden = YES;
        }
    }
}

void setAppActivateCallback(void (*)(void*), void*) {}
void setAppDeactivateCallback(void (*)(void*), void*) {}
void setThemeChangeCallback(void (*)(const char*, void*), void*) {}
void setKeyShortcutCallback(void (*)(const std::string&, void*), void*) {}
void setAppOpenFileCallback(void (*)(const std::string&, void*), void*) {}
void deliverOpenFilePaths(int, const char**) {}

UIApplication* getApplication() {
    return g_app;
}

void setMainWindow(UIWindow* window) {
    g_mainWindow = window;
}

} // namespace platform

#endif // PLATFORM_IOS
