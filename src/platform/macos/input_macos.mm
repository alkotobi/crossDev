// macOS input field implementation
#import <Cocoa/Cocoa.h>
#include "../../../include/platform.h"
#include "../platform_impl.h"
#include <string>

#ifdef PLATFORM_MACOS

namespace platform {

void* createInputField(void* parentHandle, int x, int y, int width, int height, const std::string& placeholder) {
    @autoreleasepool {
        if (!parentHandle) {
            return nullptr;
        }
        
        NSView *parentView = nullptr;
        
        // Get parent view - could be NSWindow's contentView or an NSView (from Container)
        if ([(__bridge id)parentHandle isKindOfClass:[NSWindow class]]) {
            NSWindow *window = (__bridge NSWindow*)parentHandle;
            parentView = [window contentView];
        } else if ([(__bridge id)parentHandle isKindOfClass:[NSView class]]) {
            parentView = (__bridge NSView*)parentHandle;
        } else {
            return nullptr;
        }
        
        // Convert coordinates: parent view uses bottom-left origin, but we receive top-left origin
        NSRect parentBounds = [parentView bounds];
        NSRect inputRect = NSMakeRect(x, parentBounds.size.height - y - height, width, height);
        NSTextField *textField = [[NSTextField alloc] initWithFrame:inputRect];
        [textField setPlaceholderString:[NSString stringWithUTF8String:placeholder.c_str()]];
        [textField setAutoresizingMask:NSViewMaxXMargin | NSViewMinYMargin];
        [textField setBezelStyle:NSTextFieldSquareBezel];
        
        [parentView addSubview:textField];
        
        return (void*)CFBridgingRetain(textField);
    }
}

void destroyInputField(void* inputHandle) {
    @autoreleasepool {
        if (inputHandle) {
            NSTextField *textField = (__bridge NSTextField*)inputHandle;
            [textField removeFromSuperview];
            CFBridgingRelease(inputHandle);
        }
    }
}

void setInputText(void* inputHandle, const std::string& text) {
    @autoreleasepool {
        if (inputHandle) {
            NSTextField *textField = (__bridge NSTextField*)inputHandle;
            [textField setStringValue:[NSString stringWithUTF8String:text.c_str()]];
        }
    }
}

std::string getInputText(void* inputHandle) {
    @autoreleasepool {
        if (inputHandle) {
            NSTextField *textField = (__bridge NSTextField*)inputHandle;
            NSString *value = [textField stringValue];
            if (value) {
                return [value UTF8String];
            }
        }
        return "";
    }
}

} // namespace platform

#endif // PLATFORM_MACOS
