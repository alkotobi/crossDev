// macOS container implementation
#import <Cocoa/Cocoa.h>
#include "../../../include/platform.h"
#include "../platform_impl.h"
#include <string>

#ifdef PLATFORM_MACOS

// Flipped container for tab bar (y=0 at top, correct hit-testing for overlay)
@interface FlippedContainerView : NSView
@end
@implementation FlippedContainerView
- (BOOL)isFlipped { return YES; }
@end

namespace platform {

void* createContainer(void* parentHandle, int x, int y, int width, int height, bool flipped) {
    @autoreleasepool {
        if (!parentHandle) {
            return nullptr;
        }
        
        NSView *parentView = nullptr;
        
        // Get parent view - could be NSWindow's contentView or another NSView
        if ([(__bridge id)parentHandle isKindOfClass:[NSWindow class]]) {
            NSWindow *window = (__bridge NSWindow*)parentHandle;
            parentView = [window contentView];
        } else if ([(__bridge id)parentHandle isKindOfClass:[NSView class]]) {
            parentView = (__bridge NSView*)parentHandle;
        } else {
            return nullptr;
        }
        
        // Convert coordinates: if parent is flipped (y=0 at top), use directly; else convert from top-left to bottom-left
        NSRect parentBounds = [parentView bounds];
        CGFloat yPos = [parentView isFlipped] ? (CGFloat)y : (parentBounds.size.height - (CGFloat)y - (CGFloat)height);
        NSRect containerRect = NSMakeRect((CGFloat)x, yPos, (CGFloat)width, (CGFloat)height);
        NSView *containerView = flipped
            ? [[FlippedContainerView alloc] initWithFrame:containerRect]
            : [[NSView alloc] initWithFrame:containerRect];
        [containerView setWantsLayer:YES];
        containerView.layer.backgroundColor = [NSColor whiteColor].CGColor;
        
        [parentView addSubview:containerView];
        
        return (void*)CFBridgingRetain(containerView);
    }
}

void destroyContainer(void* containerHandle) {
    @autoreleasepool {
        if (containerHandle) {
            NSView *containerView = (__bridge NSView*)containerHandle;
            [containerView removeFromSuperview];
            CFBridgingRelease(containerHandle);
        }
    }
}

void resizeContainer(void* containerHandle, int x, int y, int width, int height) {
    @autoreleasepool {
        if (containerHandle) {
            NSView *containerView = (__bridge NSView*)containerHandle;
            NSView *superview = [containerView superview];
            if (superview) {
                CGFloat yPos = [superview isFlipped] ? (CGFloat)y : ([superview bounds].size.height - (CGFloat)y - (CGFloat)height);
                NSRect newRect = NSMakeRect((CGFloat)x, yPos, (CGFloat)width, (CGFloat)height);
                [containerView setFrame:newRect];
            }
        }
    }
}

void showContainer(void* containerHandle) {
    @autoreleasepool {
        if (containerHandle) {
            NSView *containerView = (__bridge NSView*)containerHandle;
            [containerView setHidden:NO];
        }
    }
}

void hideContainer(void* containerHandle) {
    @autoreleasepool {
        if (containerHandle) {
            NSView *containerView = (__bridge NSView*)containerHandle;
            [containerView setHidden:YES];
        }
    }
}

void bringContainerToFront(void* containerHandle) {
    @autoreleasepool {
        if (containerHandle) {
            NSView *containerView = (__bridge NSView*)containerHandle;
            NSView *superview = [containerView superview];
            if (superview) {
                [superview addSubview:containerView positioned:NSWindowAbove relativeTo:nil];
            }
            // Force tab bar above WebView in layer hierarchy (WKWebView can steal hits otherwise)
            if (containerView.layer) {
                containerView.layer.zPosition = 1000;
            }
        }
    }
}

void setContainerBackgroundColor(void* containerHandle, int red, int green, int blue) {
    @autoreleasepool {
        if (containerHandle) {
            NSView *containerView = (__bridge NSView*)containerHandle;
            NSColor *color = [NSColor colorWithCalibratedRed:red/255.0
                                                       green:green/255.0
                                                        blue:blue/255.0
                                                       alpha:1.0];
            CALayer *layer = containerView.layer;
            if (layer) layer.backgroundColor = color.CGColor;
        }
    }
}

void setContainerBorderStyle(void* containerHandle, int borderStyle) {
    @autoreleasepool {
        if (containerHandle) {
            NSView *containerView = (__bridge NSView*)containerHandle;
            CALayer *layer = [containerView layer];
            if (layer) {
                if (borderStyle == 1) { // BorderSingle
                    layer.borderWidth = 1.0;
                    layer.borderColor = [[NSColor grayColor] CGColor];
                } else if (borderStyle == 2) { // BorderDouble
                    layer.borderWidth = 2.0;
                    layer.borderColor = [[NSColor grayColor] CGColor];
                } else { // BorderNone
                    layer.borderWidth = 0.0;
                }
            }
        }
    }
}

} // namespace platform

#endif // PLATFORM_MACOS
