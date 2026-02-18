// iOS window implementation
#import <UIKit/UIKit.h>
#import <objc/runtime.h>
#include "../../../include/platform.h"
#include "../platform_impl.h"
#include <string>

namespace platform {
    void initApplication();
    void setMainWindow(UIWindow* window);
}

typedef void (*FocusCallback)(void* userData);

@interface WindowFocusObserver : NSObject
@property (weak) UIWindow* window;
@property (assign) FocusCallback focusCallback;
@property (assign) FocusCallback blurCallback;
@property (assign) void* focusUserData;
@property (assign) void* blurUserData;
@end
@implementation WindowFocusObserver
- (void)windowDidBecomeKey:(NSNotification*)note {
    if (self.focusCallback) self.focusCallback(self.focusUserData);
}
- (void)windowDidResignKey:(NSNotification*)note {
    if (self.blurCallback) self.blurCallback(self.blurUserData);
}
- (void)dealloc {
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}
@end

#ifdef PLATFORM_IOS

namespace platform {

void* createWindow(int x, int y, int width, int height, const std::string& title, void* userData) {
    @autoreleasepool {
        initApplication();
        
        // On iOS, we create a UIWindow that fills the screen
        // x, y, width, height are ignored as iOS manages window size
        #pragma clang diagnostic push
        #pragma clang diagnostic ignored "-Wdeprecated-declarations"
        CGRect screenBounds = [[UIScreen mainScreen] bounds];
        UIWindow* window = [[UIWindow alloc] initWithFrame:screenBounds];
        #pragma clang diagnostic pop
        
        // Create a view controller to hold our content
        UIViewController* viewController = [[UIViewController alloc] init];
        viewController.view.backgroundColor = [UIColor whiteColor];
        
        // Create a label to display the title
        UILabel* titleLabel = [[UILabel alloc] init];
        titleLabel.text = [NSString stringWithUTF8String:title.c_str()];
        titleLabel.textAlignment = NSTextAlignmentCenter;
        titleLabel.font = [UIFont boldSystemFontOfSize:24];
        titleLabel.translatesAutoresizingMaskIntoConstraints = NO;
        
        [viewController.view addSubview:titleLabel];
        
        // Center the label
        [NSLayoutConstraint activateConstraints:@[
            [titleLabel.centerXAnchor constraintEqualToAnchor:viewController.view.centerXAnchor],
            [titleLabel.centerYAnchor constraintEqualToAnchor:viewController.view.centerYAnchor]
        ]];
        
        window.rootViewController = viewController;
        setMainWindow(window);
        
        return (void*)CFBridgingRetain(window);
    }
}

void destroyWindow(void* handle) {
    @autoreleasepool {
        if (handle) {
            UIWindow* window = (__bridge_transfer UIWindow*)handle;
            window.hidden = YES;
            setMainWindow(nullptr);
        }
    }
}

void showWindow(void* handle) {
    @autoreleasepool {
        if (handle) {
            UIWindow* window = (__bridge UIWindow*)handle;
            [window makeKeyAndVisible];
        }
    }
}

void hideWindow(void* handle) {
    @autoreleasepool {
        if (handle) {
            UIWindow* window = (__bridge UIWindow*)handle;
            window.hidden = YES;
        }
    }
}

void setWindowTitle(void* handle, const std::string& title) {
    @autoreleasepool {
        if (handle) {
            UIWindow* window = (__bridge UIWindow*)handle;
            if (window.rootViewController && window.rootViewController.view) {
                // Find the label and update it
                for (UIView* subview in window.rootViewController.view.subviews) {
                    if ([subview isKindOfClass:[UILabel class]]) {
                        UILabel* label = (UILabel*)subview;
                        label.text = [NSString stringWithUTF8String:title.c_str()];
                        break;
                    }
                }
            }
        }
    }
}

void maximizeWindow(void* handle) {
    // iOS: windows fill the screen by default - no-op
    (void)handle;
}

bool isWindowVisible(void* handle) {
    @autoreleasepool {
        if (handle) {
            UIWindow* window = (__bridge UIWindow*)handle;
            return !window.hidden && window.windowLevel == UIWindowLevelNormal;
        }
        return false;
    }
}

void setWindowResizeCallback(void* windowHandle, void (*callback)(int width, int height, void* userData), void* userData) {
    // iOS windows are typically full-screen and don't resize in the traditional sense
    (void)windowHandle;
    (void)callback;
    (void)userData;
}

void setWindowMoveCallback(void*, void (*)(int, int, void*), void*) {
    // iOS: stub - windows don't move in the traditional sense
}

void setWindowFileDropCallback(void*, void (*)(const std::string&, void*), void*) {
    // iOS: stub - file drop would need custom implementation
}

void setWindowCloseCallback(void*, void (*)(void*), void*) {
    // iOS: Stub - windows typically don't have close buttons
}

void setWindowFocusCallback(void* windowHandle, void (*callback)(void*), void* userData) {
    @autoreleasepool {
        if (!windowHandle || !callback) return;
        UIWindow* window = (__bridge UIWindow*)windowHandle;
        WindowFocusObserver* obs = objc_getAssociatedObject(window, (__bridge const void*)@"focusObserver");
        if (!obs) {
            obs = [[WindowFocusObserver alloc] init];
            obs.window = window;
            objc_setAssociatedObject(window, (__bridge const void*)@"focusObserver", obs, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
            NSNotificationCenter* nc = [NSNotificationCenter defaultCenter];
            [nc addObserver:obs selector:@selector(windowDidBecomeKey:) name:UIWindowDidBecomeKeyNotification object:window];
            [nc addObserver:obs selector:@selector(windowDidResignKey:) name:UIWindowDidResignKeyNotification object:window];
        }
        obs.focusCallback = callback;
        obs.focusUserData = userData;
    }
}

void setWindowBlurCallback(void* windowHandle, void (*callback)(void*), void* userData) {
    @autoreleasepool {
        if (!windowHandle || !callback) return;
        UIWindow* window = (__bridge UIWindow*)windowHandle;
        WindowFocusObserver* obs = objc_getAssociatedObject(window, (__bridge const void*)@"focusObserver");
        if (!obs) {
            obs = [[WindowFocusObserver alloc] init];
            obs.window = window;
            objc_setAssociatedObject(window, (__bridge const void*)@"focusObserver", obs, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
            NSNotificationCenter* nc = [NSNotificationCenter defaultCenter];
            [nc addObserver:obs selector:@selector(windowDidBecomeKey:) name:UIWindowDidBecomeKeyNotification object:window];
            [nc addObserver:obs selector:@selector(windowDidResignKey:) name:UIWindowDidResignKeyNotification object:window];
        }
        obs.blurCallback = callback;
        obs.blurUserData = userData;
    }
}

void setWindowStateCallback(void* windowHandle, void (*callback)(const char*, void*), void* userData) {
    (void)windowHandle;
    (void)callback;
    (void)userData;
    // Stub: iOS uses different window model
}

} // namespace platform

#endif // PLATFORM_IOS
