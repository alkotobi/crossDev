// macOS window implementation
#import <Cocoa/Cocoa.h>
#import <objc/runtime.h>
#include "../../../include/platform.h"
#include "../platform_impl.h"
#include <string>

// Forward declaration
namespace platform {
    NSApplication* getApplication();
    void initApplication();
}

#ifdef PLATFORM_MACOS

// Resize callback storage (must be at global scope for Objective-C)
typedef void (*ResizeCallback)(int width, int height, void* userData);
typedef void (*MoveCallback)(int x, int y, void* userData);

// Objective-C declarations must be at global scope, not inside C++ namespace
typedef void (*CloseCallback)(void* userData);
typedef void (*FocusCallback)(void* userData);
typedef void (*StateCallback)(const char* state, void* userData);
typedef void (*FileDropCallback)(const std::string& pathsJson, void* userData);

// Content view with flipped coordinates (y=0 at top) to match our top-left convention
@interface FlippedView : NSView
@end
@implementation FlippedView
- (BOOL)isFlipped { return YES; }
@end

@interface WindowResizeDelegate : NSObject <NSWindowDelegate, NSDraggingDestination>
@property (assign) ResizeCallback resizeCallback;
@property (assign) void* resizeUserData;
@property (assign) MoveCallback moveCallback;
@property (assign) void* moveUserData;
@property (assign) CloseCallback closeCallback;
@property (assign) void* closeUserData;
@property (assign) FocusCallback focusCallback;
@property (assign) void* focusUserData;
@property (assign) FocusCallback blurCallback;
@property (assign) void* blurUserData;
@property (assign) FileDropCallback fileDropCallback;
@property (assign) void* fileDropUserData;
@property (assign) StateCallback stateCallback;
@property (assign) void* stateUserData;
@property (assign) BOOL lastZoomedState;
@end

@implementation WindowResizeDelegate
- (void)windowDidResize:(NSNotification *)notification {
    NSWindow *window = notification.object;
    if (self.resizeCallback) {
        NSRect frame = [window contentRectForFrameRect:[window frame]];
        self.resizeCallback((int)frame.size.width, (int)frame.size.height, self.resizeUserData);
    }
    if (self.stateCallback) {
        BOOL zoomed = [window isZoomed];
        if (zoomed != self.lastZoomedState) {
            self.lastZoomedState = zoomed;
            self.stateCallback(zoomed ? "maximize" : "restore", self.stateUserData);
        }
    }
}
- (void)windowDidMove:(NSNotification *)notification {
    if (self.moveCallback) {
        NSWindow *window = notification.object;
        NSRect frame = [window frame];
        NSScreen *screen = [window screen] ?: [NSScreen mainScreen];
        NSRect screenFrame = screen ? [screen frame] : NSZeroRect;
        int x = (int)frame.origin.x;
        int y = (int)(screenFrame.size.height - frame.origin.y - frame.size.height);
        self.moveCallback(x, y, self.moveUserData);
    }
}
- (void)windowWillClose:(NSNotification *)notification {
    if (self.closeCallback) {
        void (*callback)(void*) = self.closeCallback;
        void* userData = self.closeUserData;
        self.closeCallback = nullptr;
        self.closeUserData = nullptr;
        // Defer to avoid deallocating window while in its delegate callback
        dispatch_async(dispatch_get_main_queue(), ^{
            if (callback) {
                callback(userData);  // userData may be nullptr (main window quit)
            }
        });
    }
}
- (void)windowDidBecomeKey:(NSNotification *)notification {
    if (self.focusCallback) {
        self.focusCallback(self.focusUserData);
    }
}
- (void)windowDidResignKey:(NSNotification *)notification {
    if (self.blurCallback) {
        self.blurCallback(self.blurUserData);
    }
}
- (void)windowDidMiniaturize:(NSNotification *)notification {
    if (self.stateCallback) {
        self.stateCallback("minimize", self.stateUserData);
    }
}
- (void)windowDidDeminiaturize:(NSNotification *)notification {
    if (self.stateCallback) {
        self.stateCallback("restore", self.stateUserData);
    }
}
- (NSDragOperation)draggingEntered:(id<NSDraggingInfo>)sender {
    if (self.fileDropCallback && [sender.draggingPasteboard.types containsObject:NSPasteboardTypeFileURL]) {
        return NSDragOperationCopy;
    }
    return NSDragOperationNone;
}
- (BOOL)performDragOperation:(id<NSDraggingInfo>)sender {
    if (!self.fileDropCallback) return NO;
    NSPasteboard *pboard = [sender draggingPasteboard];
    NSArray *urls = [pboard readObjectsForClasses:@[[NSURL class]] options:@{NSPasteboardURLReadingFileURLsOnlyKey: @YES}];
    if (!urls || urls.count == 0) return NO;
    NSMutableArray *paths = [NSMutableArray array];
    for (NSURL *url in urls) {
        if (url.fileURL) {
            [paths addObject:url.path];
        }
    }
    if (paths.count == 0) return NO;
    NSData *jsonData = [NSJSONSerialization dataWithJSONObject:paths options:0 error:nil];
    NSString *jsonStr = jsonData ? [[NSString alloc] initWithData:jsonData encoding:NSUTF8StringEncoding] : @"[]";
    std::string pathsJson = jsonStr ? [jsonStr UTF8String] : "[]";
    self.fileDropCallback(pathsJson, self.fileDropUserData);
    return YES;
}
@end

namespace platform {

void* createWindow(int x, int y, int width, int height, const std::string& title, void* userData) {
    @autoreleasepool {
        initApplication();
        
        // Convert screen coordinates (top-left origin) to Cocoa coordinates (bottom-left origin)
        NSScreen *mainScreen = [NSScreen mainScreen];
        NSRect screenRect = [mainScreen frame];
        NSRect windowRect = NSMakeRect(x, screenRect.size.height - y - height, width, height);
        
        NSWindow *window = [[NSWindow alloc] 
            initWithContentRect:windowRect
            styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable
            backing:NSBackingStoreBuffered
            defer:NO];
        
        // Use flipped content view so y=0 is at top (matches our top-left coordinate convention)
        NSView *contentView = [[FlippedView alloc] initWithFrame:windowRect];
        [window setContentView:contentView];
        
        NSString *nsTitle = [NSString stringWithUTF8String:title.c_str()];
        [window setTitle:nsTitle];
        
        // Center the window if coordinates are default
        if (x == 0 && y == 0) {
            [window center];
        }
        
        return (void*)CFBridgingRetain(window);
    }
}

void destroyWindow(void* handle) {
    @autoreleasepool {
        if (handle) {
            NSWindow *window = (__bridge NSWindow*)handle;
            [window close];
            CFBridgingRelease(handle);
        }
    }
}

void showWindow(void* handle) {
    @autoreleasepool {
        if (handle) {
            NSWindow *window = (__bridge NSWindow*)handle;
            [window makeKeyAndOrderFront:nil];
            NSApplication* app = getApplication();
            if (app) {
                [app activateIgnoringOtherApps:YES];
            }
        }
    }
}

void hideWindow(void* handle) {
    @autoreleasepool {
        if (handle) {
            NSWindow *window = (__bridge NSWindow*)handle;
            [window orderOut:nil];
        }
    }
}

void setWindowTitle(void* handle, const std::string& title) {
    @autoreleasepool {
        if (handle) {
            NSWindow *window = (__bridge NSWindow*)handle;
            NSString *nsTitle = [NSString stringWithUTF8String:title.c_str()];
            [window setTitle:nsTitle];
        }
    }
}

void maximizeWindow(void* handle) {
    @autoreleasepool {
        if (handle) {
            NSWindow *window = (__bridge NSWindow*)handle;
            [window zoom:nil];
        }
    }
}

bool isWindowVisible(void* handle) {
    @autoreleasepool {
        if (handle) {
            NSWindow *window = (__bridge NSWindow*)handle;
            return [window isVisible];
        }
        return false;
    }
}

void setWindowResizeCallback(void* windowHandle, ResizeCallback callback, void* userData) {
    @autoreleasepool {
        if (!windowHandle || !callback) {
            return;
        }
        
        NSWindow *window = (__bridge NSWindow*)windowHandle;
        
        // Get or create delegate
        WindowResizeDelegate *delegate = objc_getAssociatedObject(window, @"resizeDelegate");
        if (!delegate) {
            delegate = [[WindowResizeDelegate alloc] init];
            objc_setAssociatedObject(window, @"resizeDelegate", delegate, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
            [window setDelegate:delegate];
        }
        
        delegate.resizeCallback = callback;
        delegate.resizeUserData = userData;
    }
}

void setWindowMoveCallback(void* windowHandle, void (*callback)(int x, int y, void* userData), void* userData) {
    @autoreleasepool {
        if (!windowHandle || !callback) {
            return;
        }
        NSWindow *window = (__bridge NSWindow*)windowHandle;
        WindowResizeDelegate *delegate = objc_getAssociatedObject(window, @"resizeDelegate");
        if (!delegate) {
            delegate = [[WindowResizeDelegate alloc] init];
            objc_setAssociatedObject(window, @"resizeDelegate", delegate, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
            [window setDelegate:delegate];
        }
        delegate.moveCallback = callback;
        delegate.moveUserData = userData;
    }
}

void setWindowCloseCallback(void* windowHandle, void (*callback)(void* userData), void* userData) {
    @autoreleasepool {
        if (!windowHandle) {
            return;
        }
        
        NSWindow *window = (__bridge NSWindow*)windowHandle;
        
        // Get or create delegate (same as resize)
        WindowResizeDelegate *delegate = objc_getAssociatedObject(window, @"resizeDelegate");
        if (!delegate) {
            if (!callback) return;  // Nothing to clear
            delegate = [[WindowResizeDelegate alloc] init];
            objc_setAssociatedObject(window, @"resizeDelegate", delegate, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
            [window setDelegate:delegate];
        }
        
        delegate.closeCallback = callback;
        delegate.closeUserData = userData;
    }
}

void setWindowFocusCallback(void* windowHandle, void (*callback)(void* userData), void* userData) {
    @autoreleasepool {
        if (!windowHandle) return;
        NSWindow *window = (__bridge NSWindow*)windowHandle;
        WindowResizeDelegate *delegate = objc_getAssociatedObject(window, @"resizeDelegate");
        if (!delegate) {
            delegate = [[WindowResizeDelegate alloc] init];
            objc_setAssociatedObject(window, @"resizeDelegate", delegate, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
            [window setDelegate:delegate];
        }
        delegate.focusCallback = callback;
        delegate.focusUserData = userData;
    }
}

void setWindowBlurCallback(void* windowHandle, void (*callback)(void* userData), void* userData) {
    @autoreleasepool {
        if (!windowHandle) return;
        NSWindow *window = (__bridge NSWindow*)windowHandle;
        WindowResizeDelegate *delegate = objc_getAssociatedObject(window, @"resizeDelegate");
        if (!delegate) {
            delegate = [[WindowResizeDelegate alloc] init];
            objc_setAssociatedObject(window, @"resizeDelegate", delegate, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
            [window setDelegate:delegate];
        }
        delegate.blurCallback = callback;
        delegate.blurUserData = userData;
    }
}

void setWindowFileDropCallback(void* windowHandle, void (*callback)(const std::string& pathsJson, void* userData), void* userData) {
    @autoreleasepool {
        if (!windowHandle) return;
        NSWindow *window = (__bridge NSWindow*)windowHandle;
        WindowResizeDelegate *delegate = objc_getAssociatedObject(window, @"resizeDelegate");
        if (!delegate) {
            delegate = [[WindowResizeDelegate alloc] init];
            objc_setAssociatedObject(window, @"resizeDelegate", delegate, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
            [window setDelegate:delegate];
        }
        delegate.fileDropCallback = callback;
        delegate.fileDropUserData = userData;
        if (callback) {
            [window registerForDraggedTypes:@[NSPasteboardTypeFileURL]];
        } else {
            [window unregisterDraggedTypes];
        }
    }
}

void setWindowStateCallback(void* windowHandle, void (*callback)(const char* state, void* userData), void* userData) {
    @autoreleasepool {
        if (!windowHandle) return;
        NSWindow *window = (__bridge NSWindow*)windowHandle;
        WindowResizeDelegate *delegate = objc_getAssociatedObject(window, @"resizeDelegate");
        if (!delegate) {
            delegate = [[WindowResizeDelegate alloc] init];
            objc_setAssociatedObject(window, @"resizeDelegate", delegate, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
            [window setDelegate:delegate];
        }
        delegate.stateCallback = callback;
        delegate.stateUserData = userData;
        delegate.lastZoomedState = [window isZoomed];
    }
}

} // namespace platform

#endif // PLATFORM_MACOS
