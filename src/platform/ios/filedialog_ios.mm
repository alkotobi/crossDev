// iOS file dialog implementation
#import <UIKit/UIKit.h>
#include "../../../include/platform.h"
#include "../platform_impl.h"
#include <string>

#ifdef PLATFORM_IOS

namespace platform {

bool showOpenFileDialog(void* windowHandle, const std::string& title, const std::string& filter, std::string& selectedPath) {
    @autoreleasepool {
        // On iOS, file access is limited. We'll use a simple alert with text input
        // In a real app, you'd use UIDocumentPickerViewController
        
        UIWindow* window = (__bridge UIWindow*)windowHandle;
        if (!window || !window.rootViewController) {
            return false;
        }
        
        UIAlertController* alert = [UIAlertController 
            alertControllerWithTitle:@"Open HTML File"
            message:@"Enter file path or use document picker"
            preferredStyle:UIAlertControllerStyleAlert];
        
        __block bool fileSelected = false;
        __block std::string selectedFile;
        
        [alert addTextFieldWithConfigurationHandler:^(UITextField* textField) {
            textField.placeholder = @"File path";
            textField.text = @"/Documents/";
        }];
        
        UIAlertAction* openAction = [UIAlertAction 
            actionWithTitle:@"Open" 
            style:UIAlertActionStyleDefault 
            handler:^(UIAlertAction* action) {
                UITextField* textField = alert.textFields.firstObject;
                if (textField && textField.text.length > 0) {
                    selectedFile = [textField.text UTF8String];
                    fileSelected = true;
                }
            }];
        
        UIAlertAction* cancelAction = [UIAlertAction 
            actionWithTitle:@"Cancel" 
            style:UIAlertActionStyleCancel 
            handler:nil];
        
        [alert addAction:openAction];
        [alert addAction:cancelAction];
        
        dispatch_async(dispatch_get_main_queue(), ^{
            [window.rootViewController presentViewController:alert animated:YES completion:nil];
        });
        
        // Wait for user response (simplified - in real app would use completion handler)
        // For now, return false as iOS file access requires more complex implementation
        return false;
    }
}

} // namespace platform

#endif // PLATFORM_IOS
