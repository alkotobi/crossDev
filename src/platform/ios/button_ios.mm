// iOS button implementation
#import <UIKit/UIKit.h>
#import <objc/runtime.h>
#include "../../../include/platform.h"
#include "../platform_impl.h"
#include <string>

#ifdef PLATFORM_IOS

// Button target class (must be outside namespace)
@interface ButtonTarget : NSObject
@property (assign) void* userData;
@property (assign) void (*callback)(void*);
- (void)buttonClicked:(id)sender;
@end

@implementation ButtonTarget
- (void)buttonClicked:(id)sender {
    if (self.callback && self.userData) {
        self.callback(self.userData);
    }
}
@end

namespace platform {

void* createButton(void* parentHandle, int x, int y, int width, int height, const std::string& label, void* userData) {
    @autoreleasepool {
        if (!parentHandle) {
            return nullptr;
        }
        
        UIView *parentView = nullptr;
        
        // Get parent view - could be UIWindow's root view or a UIView (from Container)
        if ([(__bridge id)parentHandle isKindOfClass:[UIWindow class]]) {
            UIWindow *window = (__bridge UIWindow*)parentHandle;
            UIViewController* viewController = window.rootViewController;
            if (viewController) {
                parentView = viewController.view;
            } else {
                return nullptr;
            }
        } else if ([(__bridge id)parentHandle isKindOfClass:[UIView class]]) {
            parentView = (__bridge UIView*)parentHandle;
        } else {
            return nullptr;
        }
        
        CGRect buttonFrame = CGRectMake(x, y, width, height);
        UIButton* button = [UIButton buttonWithType:UIButtonTypeSystem];
        button.frame = buttonFrame;
        [button setTitle:[NSString stringWithUTF8String:label.c_str()] forState:UIControlStateNormal];
        button.autoresizingMask = UIViewAutoresizingFlexibleRightMargin | UIViewAutoresizingFlexibleBottomMargin;
        
        // Create target for callback
        ButtonTarget* target = [[ButtonTarget alloc] init];
        target.userData = userData;
        [button addTarget:target action:@selector(buttonClicked:) forControlEvents:UIControlEventTouchUpInside];
        
        // Retain target to keep it alive
        objc_setAssociatedObject(button, "target", target, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        
        [parentView addSubview:button];
        
        return (void*)CFBridgingRetain(button);
    }
}

void destroyButton(void* buttonHandle) {
    @autoreleasepool {
        if (buttonHandle) {
            UIButton* button = (__bridge_transfer UIButton*)buttonHandle;
            [button removeFromSuperview];
        }
    }
}

void setButtonCallback(void* buttonHandle, void (*callback)(void*)) {
    @autoreleasepool {
        if (!buttonHandle) {
            return;
        }
        
        UIButton* button = (__bridge UIButton*)buttonHandle;
        ButtonTarget* target = objc_getAssociatedObject(button, "target");
        if (target) {
            target.callback = callback;
        }
    }
}

} // namespace platform

#endif // PLATFORM_IOS
