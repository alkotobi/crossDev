#!/bin/bash
# Generate Xcode project using CMake
# This is more reliable than manually generating project.pbxproj

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
IOS_DIR="$SCRIPT_DIR"
BUILD_DIR="$IOS_DIR/build_cmake"

echo "📦 Generating Xcode project using CMake..."
echo "   Project root: $PROJECT_ROOT"
echo "   iOS directory: $IOS_DIR"

# Clean previous build
if [ -d "$BUILD_DIR" ]; then
    echo "🧹 Cleaning previous build directory..."
    rm -rf "$BUILD_DIR"
fi

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Find iOS SDK
IOS_SDK=$(xcrun --sdk iphoneos --show-sdk-path 2>/dev/null || echo "")
if [ -z "$IOS_SDK" ]; then
    echo "❌ Error: iOS SDK not found. Make sure Xcode is installed."
    exit 1
fi

echo "✅ Found iOS SDK: $IOS_SDK"

# Configure CMake for iOS with Xcode generator
# Disable try_compile to avoid DerivedData access issues
echo "🔧 Configuring CMake for iOS..."
cmake "$PROJECT_ROOT" \
    -G Xcode \
    -DCMAKE_TOOLCHAIN_FILE="$IOS_DIR/ios.toolchain.cmake" \
    -DCMAKE_XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET=12.0 \
    -DCMAKE_XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH=NO \
    -DCMAKE_XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY="iPhone Developer" \
    -DIOS=ON \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_COMPILER_WORKS=ON \
    -DCMAKE_CXX_COMPILER_WORKS=ON \
    -DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY

if [ $? -eq 0 ]; then
    echo ""
    echo "✅✅✅ Xcode project generated successfully!"
    echo ""
    echo "📝 Next steps:"
    echo "   1. Open the project in Xcode:"
    echo "      open $BUILD_DIR/WindowTest.xcodeproj"
    echo "   2. Select a development team in Signing & Capabilities"
    echo "   3. Select your target device (Simulator or Device)"
    echo "   4. Build and run (⌘R)"
    echo ""
else
    echo "❌ Failed to generate Xcode project"
    exit 1
fi
