# CrossDev - Multi-Platform Native Window Application

A cross-platform C++ application framework that creates native windows on macOS, Windows, Linux, and iOS by calling each platform's system libraries directly.

## Component System

CrossDev implements a **Delphi-style Owner/Parent component architecture**:

- **Owner**: Memory management - components are automatically cleaned up when their owner is destroyed
- **Parent**: Visual hierarchy - controls are displayed within their parent control

See [COMPONENT_SYSTEM.md](COMPONENT_SYSTEM.md) for detailed documentation and usage examples.

## Project Structure

```
.
├── include/              # Public headers (platform-agnostic interface)
│   ├── window.h         # Window class interface
│   └── platform.h       # Platform detection macros
├── src/                  # Source files
│   ├── main.cpp         # Application entry point
│   ├── window.cpp       # Window class implementation
│   └── platform/        # Platform-specific implementations
│       ├── platform_impl.h      # Platform implementation interface
│       ├── macos/               # macOS (Cocoa) implementation
│       │   ├── app_macos.mm
│       │   ├── window_macos.mm
│       │   ├── webview_macos.mm
│       │   ├── button_macos.mm
│       │   ├── filedialog_macos.mm
│       │   └── input_macos.mm
│       ├── windows/             # Windows (Win32) implementation
│       │   ├── app_windows.cpp
│       │   ├── window_windows.cpp
│       │   ├── webview_windows.cpp
│       │   ├── button_windows.cpp
│       │   ├── filedialog_windows.cpp
│       │   └── input_windows.cpp
│       ├── linux/               # Linux (X11) implementation
│       │   ├── app_linux.cpp
│       │   ├── window_linux.cpp
│       │   ├── webview_linux.cpp
│       │   ├── button_linux.cpp
│       │   ├── filedialog_linux.cpp
│       │   └── input_linux.cpp
│       └── ios/                 # iOS (UIKit) implementation
│           ├── app_ios.mm
│           ├── window_ios.mm
│           ├── webview_ios.mm
│           ├── button_ios.mm
│           ├── filedialog_ios.mm
│           ├── input_ios.mm
│           └── AppDelegate.mm
├── CMakeLists.txt       # CMake build configuration (recommended)
├── Makefile             # Legacy Makefile build system
└── README.md
```

## Architecture

### Platform Abstraction Layer

The project uses a clean separation between platform-agnostic code and platform-specific implementations:

1. **Public Interface** (`include/window.h`): C++ classes that applications use
2. **Platform Implementation** (`src/platform/platform_impl.h`): C++ namespace with platform-specific functions
3. **Platform-Specific Code**: Each platform has its own directory with native API calls
4. **Component Separation**: Each UI component (window, webview, button, input, filedialog) is in its own file

### Key Design Principles

- **Separation of Concerns**: Platform-specific code is isolated in separate directories
- **Component Isolation**: Each UI component has its own implementation file
- **Single Responsibility**: Each file has a clear, focused purpose
- **Easy to Extend**: Adding new platforms only requires implementing the `platform::` namespace functions
- **Type Safety**: Uses C++ classes and RAII for resource management
- **Delphi-Style Ownership**: Owner/Parent pattern for automatic memory management and visual hierarchy

## Building

### Using CMake (Recommended)

CMake is the recommended build system. It automatically detects your platform and configures the build accordingly.

#### macOS
```bash
mkdir build
cd build
cmake ..
make
```

#### Windows
Requires CMake and a C++ compiler (MinGW, MSVC, or Clang):

**⚠️ Important:** On Windows, especially ARM64, you **MUST** specify the architecture explicitly when using Visual Studio generators.

**With Visual Studio (MSVC):**

For x64 (recommended, works on both x64 and ARM64 Windows via emulation):
```bash
# Clean previous build if it exists
rmdir /s /q build 2>nul
mkdir build
cd build
cmake -A x64 ..
cmake --build .
```

For native ARM64 (if your Visual Studio installation supports it):
```bash
mkdir build
cd build
cmake -A ARM64 ..
cmake --build .
```

For x86:
```bash
mkdir build
cd build
cmake -A Win32 ..
```

**With MinGW (Recommended alternative, often more reliable on ARM64 Windows):**
```bash
# Clean previous build if it exists
rmdir /s /q build 2>nul
mkdir build
cd build
cmake -G "MinGW Makefiles" ..
cmake --build .
```

**Troubleshooting:**
- If you get ARM64 configuration errors with Visual Studio, **always use `-A x64`** flag explicitly
- The architecture cannot be set in CMakeLists.txt - it must be specified via the `-A` flag on the command line
- MinGW is often more reliable on ARM64 Windows systems

#### Linux
Requires X11 and GTK development libraries:
```bash
sudo apt-get install libx11-dev libgtk-3-dev libwebkit2gtk-4.0-dev cmake  # Debian/Ubuntu
# or
sudo yum install libX11-devel gtk3-devel webkit2gtk3-devel cmake    # RHEL/CentOS
```

Then build:
```bash
mkdir build
cd build
cmake ..
make
```

#### iOS
iOS builds with CMake require additional configuration:
```bash
mkdir build-ios
cd build-ios
cmake -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0 ..
cmake --build .
```

**Note:** iOS builds create a binary, not a complete app bundle. For a full iOS app, you'll need to:
1. Create an Xcode project
2. Add the built binary or source files
3. Configure code signing and provisioning
4. Build through Xcode

### Using Makefile (Legacy)

The project also includes a Makefile for direct builds:

#### macOS
```bash
make
```

#### Windows
Requires MinGW (g++) or MSVC. With MinGW:
```bash
make
```

If you have a different compiler, you can override it:
```bash
make CXX=g++
# or
make CXX=cl
```

#### Linux
Requires X11 and GTK development libraries (see CMake section above), then:
```bash
make
```

#### iOS
iOS builds require Xcode and must be built manually using specific targets:

**For iOS Simulator:**
```bash
make ios-sim
# or
make ios
```

**For iOS Device:**
```bash
make ios-device
```

**Clean iOS builds:**
```bash
make ios-clean
```

## Running

```bash
./window_test        # macOS/Linux
window_test.exe      # Windows
```

## Features

- **Native Windows**: Create platform-native windows
- **Web View**: Embedded web view for displaying HTML content
- **File Dialog**: Native file picker for selecting HTML files
- **Input Field**: Text input for entering URLs or file paths
- **Button**: Native buttons with callbacks
- **Container**: Panel-like container for grouping controls
- **URL Loading**: Load web URLs directly in the web view
- **File Loading**: Load HTML files from disk
- **Component System**: Delphi-style Owner/Parent architecture with automatic cleanup
- **Component Naming**: Name components for easy lookup
- **Component Enumeration**: Iterate through owned components and visual children

## Usage

1. Enter a URL (e.g., `https://example.com`) or file path in the input field
2. Click "Load File" button to load the content
3. If input is empty, clicking the button opens a file dialog
4. The web view displays the loaded content

## Adding a New Platform

1. Create a new directory under `src/platform/` (e.g., `src/platform/bsd/`)
2. Implement all functions in the `platform::` namespace from `platform_impl.h`
3. Create separate files for each component:
   - `app_<platform>.cpp` - Application lifecycle
   - `window_<platform>.cpp` - Window management
   - `webview_<platform>.cpp` - Web view
   - `button_<platform>.cpp` - Buttons
   - `input_<platform>.cpp` - Input fields
   - `filedialog_<platform>.cpp` - File dialogs
4. Add platform detection in `include/platform.h`
5. Update `CMakeLists.txt` (and optionally `Makefile`) to include your platform's sources and build flags

## Platform-Specific Notes

### macOS
- Uses Cocoa framework via Objective-C++
- Files use `.mm` extension for Objective-C++ compilation
- Automatic memory management with ARC
- WebKit for web view rendering

### Windows
- Uses Win32 API
- Requires Windows.h
- Manual window class registration
- **WebView2 Support**: For full HTML rendering and URL loading, **WebView2 Runtime must be installed** on the system
  - **Download and install WebView2 Runtime**: https://go.microsoft.com/fwlink/p/?LinkId=2124703
  - The WebView2 SDK is automatically downloaded by CMake using FetchContent
  - Without WebView2 Runtime installed, the app will show an error message with installation instructions
  - The error message includes a direct download link to install WebView2 Runtime

### Linux
- Uses X11 library and GTK3
- Requires Xlib and GTK development headers
- WebKitGTK for web view rendering
- Basic event loop implementation

### iOS
- Uses UIKit framework via Objective-C++
- Files use `.mm` extension for Objective-C++ compilation
- Requires Xcode and iOS SDK
- Manual build targets (not auto-detected)
- Creates UIWindow with view controller instead of traditional windows
- WebKit for web view rendering
- Note: iOS apps require proper app bundle structure and code signing for device deployment

## Dependencies

- **macOS**: Cocoa, Foundation, WebKit, UniformTypeIdentifiers frameworks (system)
- **Windows**: Win32 API (system), MinGW g++ or MSVC compiler
- **Linux**: X11, GTK3, WebKitGTK development libraries
- **iOS**: UIKit, Foundation, WebKit frameworks, Xcode with iOS SDK

## License

This is a demonstration project for educational purposes.
