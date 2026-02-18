// macOS file dialog implementation
#import <Cocoa/Cocoa.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#include "../../../include/platform.h"
#include "../platform_impl.h"
#include <string>
#include <dispatch/dispatch.h>

#ifdef PLATFORM_MACOS

namespace platform {

static bool showOpenFileDialogImpl(void* windowHandle, const std::string& title, const std::string& filter, std::string& selectedPath) {
    @autoreleasepool {
        NSOpenPanel* panel = [NSOpenPanel openPanel];
        
        // Set dialog title
        if (!title.empty()) {
            [panel setTitle:[NSString stringWithUTF8String:title.c_str()]];
        }
        
        // Parse filter: "HTML Files (*.html)|*.html|All Files (*.*)|*.*" or "JavaScript (*.js)|*.js|..."
        // Extract extensions from patterns (Description (*.ext)|*.ext)
        NSMutableArray* extensions = [NSMutableArray array];
        std::string f = filter;
        for (size_t i = 0; i < f.size(); ) {
            size_t pipe = f.find('|', i);
            if (pipe == std::string::npos) break;
            if (pipe + 1 < f.size()) {
                size_t start = pipe + 1;
                size_t nextPipe = f.find('|', start);
                std::string pattern = (nextPipe != std::string::npos) ? f.substr(start, nextPipe - start) : f.substr(start);
                if (pattern.size() >= 3 && pattern[0] == '*' && pattern[1] == '.') {
                    std::string ext = pattern.substr(2);
                    if (ext != "*") {
                        NSString* nsExt = [NSString stringWithUTF8String:ext.c_str()];
                        if (![extensions containsObject:nsExt]) [extensions addObject:nsExt];
                    }
                }
                i = (nextPipe != std::string::npos) ? nextPipe : f.size();
            } else break;
        }
        
        // Set allowed file types (empty = allow all)
        if (extensions.count > 0) {
            if (@available(macOS 11.0, *)) {
                NSMutableArray* contentTypes = [[NSMutableArray alloc] init];
                for (NSString* ext in extensions) {
                    UTType* ut = [UTType typeWithFilenameExtension:ext];
                    if (ut) [contentTypes addObject:ut];
                }
                if (contentTypes.count > 0) {
                    [panel setAllowedContentTypes:contentTypes];
                }
            } else {
                #pragma clang diagnostic push
                #pragma clang diagnostic ignored "-Wdeprecated-declarations"
                [panel setAllowedFileTypes:extensions];
                #pragma clang diagnostic pop
            }
        }
        
        // Configure panel
        [panel setAllowsMultipleSelection:NO];
        [panel setCanChooseFiles:YES];
        [panel setCanChooseDirectories:NO];
        [panel setAllowsOtherFileTypes:YES];  // Allow user to select "All Files" / override filter
        
        // Set default directory
        [panel setDirectoryURL:[NSURL fileURLWithPath:[NSHomeDirectory() stringByAppendingPathComponent:@"Documents"]]];
        
        // Activate the app to ensure dialog is visible
        [[NSApplication sharedApplication] activateIgnoringOtherApps:YES];
        
        // Get parent window - use keyWindow when nil so sheet attaches to frontmost window.
        // Standalone runModal when invoked from child (e.g. Settings) can appear disabled;
        // presenting as sheet to the key window fixes focus and interactivity.
        NSWindow* parentWindow = nil;
        if (windowHandle) {
            parentWindow = (__bridge NSWindow*)windowHandle;
        }
        if (!parentWindow) {
            parentWindow = [NSApp keyWindow];
        }
        if (!parentWindow) {
            parentWindow = [NSApp mainWindow];
        }
        if (parentWindow) {
            [parentWindow makeKeyWindow];
        }
        
        NSInteger result;
        if (parentWindow) {
            // Present as sheet attached to the window - avoids "disabled" state
            __block NSModalResponse modalResult = NSModalResponseCancel;
            __block BOOL done = NO;
            [panel beginSheetModalForWindow:parentWindow completionHandler:^(NSModalResponse resp) {
                modalResult = resp;
                done = YES;
            }];
            // Pump run loop until sheet is dismissed (avoids sync->async API mismatch)
            while (!done) {
                [[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode beforeDate:[NSDate distantFuture]];
            }
            result = modalResult;
        } else {
            result = [panel runModal];
        }
        
        if (result == NSModalResponseOK) {
            NSURL* url = [panel URL];
            if (url) {
                selectedPath = [[url path] UTF8String];
                return true;
            }
        }
        
        return false;
    }
}

bool showOpenFileDialog(void* windowHandle, const std::string& title, const std::string& filter, std::string& selectedPath) {
    // Ensure we're on the main thread
    if (![NSThread isMainThread]) {
        __block bool result = false;
        __block std::string path;
        dispatch_sync(dispatch_get_main_queue(), ^{
            result = showOpenFileDialogImpl(windowHandle, title, filter, path);
        });
        selectedPath = path;
        return result;
    }
    
    return showOpenFileDialogImpl(windowHandle, title, filter, selectedPath);
}

} // namespace platform

#endif // PLATFORM_MACOS
