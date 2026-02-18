// iOS AppDelegate - entry point for iOS app
#import <UIKit/UIKit.h>
#include "../../../include/window.h"
#include "../../../include/platform.h"

#ifdef PLATFORM_IOS

// AppDelegate - uses legacy window-based approach (no scenes)
// Setting the window property on AppDelegate disables scene-based lifecycle
@interface AppDelegate : UIResponder <UIApplicationDelegate>
@property (strong, nonatomic) UIWindow *window;
@property (assign, nonatomic) void* cppWindow; // Use void* to avoid C++ in @interface
@end

@implementation AppDelegate

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
    // Create the window using legacy approach (no scenes)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wdeprecated-declarations"
    CGRect screenBounds = [[UIScreen mainScreen] bounds];
    self.window = [[UIWindow alloc] initWithFrame:screenBounds];
    #pragma clang diagnostic pop
    
    // Create the C++ window
    @try {
        Window* window = new Window(nullptr, nullptr, 0, 0, 0, 0, "test");
        window->show();
        self.cppWindow = window; // Store as void*
    } @catch (NSException* e) {
        NSLog(@"Error creating window: %@", e.reason);
        return NO;
    }
    
    // Make the window key and visible
    [self.window makeKeyAndVisible];
    
    return YES;
}

- (void)applicationWillResignActive:(UIApplication *)application {
    // Sent when the application is about to move from active to inactive state
}

- (void)applicationDidEnterBackground:(UIApplication *)application {
    // Use this method to release shared resources
}

- (void)applicationWillEnterForeground:(UIApplication *)application {
    // Called as part of the transition from the background to the active state
}

- (void)applicationDidBecomeActive:(UIApplication *)application {
    // Restart any tasks that were paused
}

- (void)applicationWillTerminate:(UIApplication *)application {
    // Clean up
    if (self.cppWindow) {
        Window* window = static_cast<Window*>(self.cppWindow);
        delete window;
        self.cppWindow = nullptr;
    }
}

@end

// iOS main function
int main(int argc, char * argv[]) {
    @autoreleasepool {
        return UIApplicationMain(argc, argv, nil, NSStringFromClass([AppDelegate class]));
    }
}

#endif // PLATFORM_IOS
