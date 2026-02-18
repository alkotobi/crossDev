// macOS application lifecycle implementation
#import <Cocoa/Cocoa.h>
#import <Foundation/Foundation.h>
#include "../../../include/platform.h"
#include "../platform_impl.h"
#include <string>

#ifdef PLATFORM_MACOS

namespace platform {

static NSApplication* g_app = nullptr;
static void (*g_appActivateCallback)(void*) = nullptr;
static void* g_appActivateUserData = nullptr;
static void (*g_appDeactivateCallback)(void*) = nullptr;
static void* g_appDeactivateUserData = nullptr;
static void (*g_themeChangeCallback)(const char*, void*) = nullptr;
static void* g_themeChangeUserData = nullptr;
static void (*g_keyShortcutCallback)(const std::string&, void*) = nullptr;
static void* g_keyShortcutUserData = nullptr;
static void (*g_appOpenFileCallback)(const std::string&, void*) = nullptr;
static void* g_appOpenFileUserData = nullptr;
static id g_keyMonitor = nil;

static void onAppActivate() {
    if (g_appActivateCallback) g_appActivateCallback(g_appActivateUserData);
}
static void onAppDeactivate() {
    if (g_appDeactivateCallback) g_appDeactivateCallback(g_appDeactivateUserData);
}

static void onThemeChange() {
    if (!g_themeChangeCallback) return;
    NSAppearance* app = [NSApp effectiveAppearance];
    NSAppearanceName name = [app bestMatchFromAppearancesWithNames:@[NSAppearanceNameAqua, NSAppearanceNameDarkAqua]];
    const char* theme = [name isEqualToString:NSAppearanceNameDarkAqua] ? "dark" : "light";
    g_themeChangeCallback(theme, g_themeChangeUserData);
}

static NSEvent* keyShortcutMonitor(NSEvent* event) {
    if (!g_keyShortcutCallback || event.type != NSEventTypeKeyDown) return event;
    NSEventModifierFlags f = [event modifierFlags];
    if (!(f & (NSEventModifierFlagCommand | NSEventModifierFlagControl | NSEventModifierFlagOption))) return event;
    @autoreleasepool {
        NSEventModifierFlags flags = [event modifierFlags];
        NSMutableArray* mods = [NSMutableArray array];
        if (flags & NSEventModifierFlagCommand) [mods addObject:@"meta"];
        if (flags & NSEventModifierFlagShift) [mods addObject:@"shift"];
        if (flags & NSEventModifierFlagOption) [mods addObject:@"alt"];
        if (flags & NSEventModifierFlagControl) [mods addObject:@"control"];
        NSString* chars = [event characters] ?: @"";
        unsigned short keyCode = [event keyCode];
        NSDictionary* obj = @{
            @"key": [chars length] ? [chars substringToIndex:1] : @"",
            @"modifiers": mods,
            @"keyCode": @(keyCode)
        };
        NSData* data = [NSJSONSerialization dataWithJSONObject:obj options:0 error:nil];
        NSString* json = data ? [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding] : @"{}";
        std::string payload([json UTF8String]);
        g_keyShortcutCallback(payload, g_keyShortcutUserData);
    }
    return event;
}

void initApplication() {
    @autoreleasepool {
        if (!g_app) {
            g_app = [NSApplication sharedApplication];
            [g_app setActivationPolicy:NSApplicationActivationPolicyRegular];
            [[NSNotificationCenter defaultCenter] addObserverForName:NSApplicationDidBecomeActiveNotification
                object:g_app queue:nil usingBlock:^(NSNotification*){ onAppActivate(); }];
            [[NSNotificationCenter defaultCenter] addObserverForName:NSApplicationDidResignActiveNotification
                object:g_app queue:nil usingBlock:^(NSNotification*){ onAppDeactivate(); }];
            [[NSDistributedNotificationCenter defaultCenter] addObserverForName:@"AppleInterfaceThemeChangedNotification"
                object:nil queue:nil usingBlock:^(NSNotification*){ onThemeChange(); }];
        }
    }
}

void setAppActivateCallback(void (*callback)(void*), void* userData) {
    g_appActivateCallback = callback;
    g_appActivateUserData = userData;
}
void setAppDeactivateCallback(void (*callback)(void*), void* userData) {
    g_appDeactivateCallback = callback;
    g_appDeactivateUserData = userData;
}

void setThemeChangeCallback(void (*callback)(const char* theme, void* userData), void* userData) {
    g_themeChangeCallback = callback;
    g_themeChangeUserData = userData;
    if (callback) {
        onThemeChange();  // Fire once with current theme
    }
}

void runApplication() {
    @autoreleasepool {
        if (g_app) {
            [g_app run];
        }
    }
}

void quitApplication() {
    @autoreleasepool {
        if (g_keyMonitor) {
            [NSEvent removeMonitor:g_keyMonitor];
            g_keyMonitor = nil;
        }
        if (g_app) {
            [g_app stop:nil];
        }
    }
}

void setKeyShortcutCallback(void (*callback)(const std::string& payloadJson, void* userData), void* userData) {
    g_keyShortcutCallback = callback;
    g_keyShortcutUserData = userData;
    if (callback && !g_keyMonitor) {
        g_keyMonitor = [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskKeyDown
            handler:^NSEvent*(NSEvent* e) { return keyShortcutMonitor(e); }];
    } else if (!callback && g_keyMonitor) {
        [NSEvent removeMonitor:g_keyMonitor];
        g_keyMonitor = nil;
    }
}

void setAppOpenFileCallback(void (*callback)(const std::string& path, void* userData), void* userData) {
    g_appOpenFileCallback = callback;
    g_appOpenFileUserData = userData;
}

void deliverOpenFilePaths(int argc, const char* argv[]) {
    if (!g_appOpenFileCallback) return;
    for (int i = 1; i < argc; ++i) {
        if (argv[i] && argv[i][0] != '-') {
            g_appOpenFileCallback(std::string(argv[i]), g_appOpenFileUserData);
        }
    }
}

NSApplication* getApplication() {
    return g_app;
}

} // namespace platform

#endif // PLATFORM_MACOS
