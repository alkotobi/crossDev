// iOS container implementation
#import <UIKit/UIKit.h>
#include "../../../include/platform.h"
#include "../platform_impl.h"
#include <string>

#ifdef PLATFORM_IOS

namespace platform {

void* createContainer(void* parentHandle, int x, int y, int width, int height, bool flipped) {
    (void)flipped;
    @autoreleasepool {
        if (!parentHandle) {
            return nullptr;
        }
        
        UIView *parentView = nullptr;
        
        // Get parent view - could be UIWindow's root view or another UIView
        if ([(__bridge id)parentHandle isKindOfClass:[UIWindow class]]) {
            UIWindow *window = (__bridge UIWindow*)parentHandle;
            parentView = window.rootViewController.view;
        } else if ([(__bridge id)parentHandle isKindOfClass:[UIView class]]) {
            parentView = (__bridge UIView*)parentHandle;
        } else {
            return nullptr;
        }
        
        CGRect containerRect = CGRectMake(x, y, width, height);
        UIView *containerView = [[UIView alloc] initWithFrame:containerRect];
        containerView.backgroundColor = [UIColor whiteColor];
        
        [parentView addSubview:containerView];
        
        return (void*)CFBridgingRetain(containerView);
    }
}

void destroyContainer(void* containerHandle) {
    @autoreleasepool {
        if (containerHandle) {
            UIView *containerView = (__bridge UIView*)containerHandle;
            [containerView removeFromSuperview];
            CFBridgingRelease(containerHandle);
        }
    }
}

void resizeContainer(void* containerHandle, int x, int y, int width, int height) {
    @autoreleasepool {
        if (containerHandle) {
            UIView *containerView = (__bridge UIView*)containerHandle;
            CGRect newRect = CGRectMake(x, y, width, height);
            containerView.frame = newRect;
        }
    }
}

void showContainer(void* containerHandle) {
    @autoreleasepool {
        if (containerHandle) {
            UIView *containerView = (__bridge UIView*)containerHandle;
            containerView.hidden = NO;
        }
    }
}

void hideContainer(void* containerHandle) {
    @autoreleasepool {
        if (containerHandle) {
            UIView *containerView = (__bridge UIView*)containerHandle;
            containerView.hidden = YES;
        }
    }
}

void bringContainerToFront(void* containerHandle) {
    @autoreleasepool {
        if (containerHandle) {
            UIView *containerView = (__bridge UIView*)containerHandle;
            [[containerView superview] bringSubviewToFront:containerView];
        }
    }
}

void setContainerBackgroundColor(void* containerHandle, int red, int green, int blue) {
    @autoreleasepool {
        if (containerHandle) {
            UIView *containerView = (__bridge UIView*)containerHandle;
            UIColor *color = [UIColor colorWithRed:red/255.0
                                             green:green/255.0
                                              blue:blue/255.0
                                             alpha:1.0];
            containerView.backgroundColor = color;
        }
    }
}

void setContainerBorderStyle(void* containerHandle, int borderStyle) {
    @autoreleasepool {
        if (containerHandle) {
            UIView *containerView = (__bridge UIView*)containerHandle;
            if (borderStyle == 1) { // BorderSingle
                containerView.layer.borderWidth = 1.0;
                containerView.layer.borderColor = [[UIColor grayColor] CGColor];
            } else if (borderStyle == 2) { // BorderDouble
                containerView.layer.borderWidth = 2.0;
                containerView.layer.borderColor = [[UIColor grayColor] CGColor];
            } else { // BorderNone
                containerView.layer.borderWidth = 0.0;
            }
        }
    }
}

} // namespace platform

#endif // PLATFORM_IOS
