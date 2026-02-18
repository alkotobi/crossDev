# iOS Xcode Project

This directory contains the Xcode project for building the iOS version of NativeWindow.

## Project Structure

- `NativeWindow.xcodeproj/` - Xcode project file
- `Info.plist` - iOS app configuration
- `generate_xcode_project.py` - Script to regenerate the Xcode project

## Building the iOS App

### Option 1: Using Xcode (Recommended)

1. Open the project in Xcode:
   ```bash
   open ios/NativeWindow.xcodeproj
   ```

2. Select a target:
   - Choose "Any iOS Device" for physical device
   - Choose a simulator (e.g., "iPhone 15 Pro") for simulator

3. Configure Signing:
   - Select the project in the navigator
   - Go to "Signing & Capabilities"
   - Select your development team
   - Xcode will automatically manage provisioning

4. Build and Run:
   - Press `Cmd+R` or click the Run button
   - The app will build and launch on the selected device/simulator

### Option 2: Using Command Line

```bash
# Build for iOS Simulator
xcodebuild -project NativeWindow.xcodeproj \
           -scheme NativeWindow \
           -sdk iphonesimulator \
           -configuration Debug \
           build

# Build for iOS Device
xcodebuild -project NativeWindow.xcodeproj \
           -scheme NativeWindow \
           -sdk iphoneos \
           -configuration Debug \
           build
```

## Regenerating the Project

If you need to regenerate the Xcode project (e.g., after adding new source files):

```bash
cd ios
python3 generate_xcode_project.py
```

## Project Configuration

- **Deployment Target**: iOS 12.0
- **C++ Standard**: C++17
- **Frameworks**: UIKit, Foundation, WebKit
- **Language**: Objective-C++ (.mm files)

## Source Files Included

The project includes:
- All core C++ source files from `src/`
- iOS-specific implementations from `src/platform/ios/`
- All header files from `include/`
- Message handlers from `src/handlers/`

## Troubleshooting

### "No signing certificate found"
- Go to Xcode > Preferences > Accounts
- Add your Apple ID
- Select your team in Signing & Capabilities

### "Module not found" errors
- Ensure all header files are in the `include/` directory
- Check that Header Search Paths includes `$(SRCROOT)/../include`

### Build errors
- Clean build folder: `Cmd+Shift+K`
- Delete derived data: `rm -rf ~/Library/Developer/Xcode/DerivedData`
- Rebuild: `Cmd+B`

## Notes

- The project uses Objective-C++ (.mm files) to bridge C++ and Objective-C
- The main entry point is `AppDelegate.mm` which creates the C++ window
- The app uses WKWebView for displaying HTML content
- File dialogs use UIDocumentPickerViewController on iOS
