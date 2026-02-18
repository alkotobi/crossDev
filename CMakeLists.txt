cmake_minimum_required(VERSION 3.15)

# On Windows with Visual Studio generator, force x64 to avoid ARM64 VCTargetsPath/OutputPath errors.
# Auto-detected ARM64 often fails with "BaseOutputPath/OutputPath property is not set".
if(WIN32 AND CMAKE_GENERATOR MATCHES "Visual Studio")
    if(NOT CMAKE_GENERATOR_PLATFORM OR CMAKE_GENERATOR_PLATFORM STREQUAL "ARM64")
        set(CMAKE_GENERATOR_PLATFORM "x64" CACHE STRING "Platform (x64 avoids ARM64 build errors)" FORCE)
        message(STATUS "Visual Studio: using x64 platform (pass -A ARM64 to override)")
    endif()
endif()

project(CrossDev CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Component lifecycle + verbose MessageRouter/CreateWindow/Singleton logging
option(COMPONENT_DEBUG_LIFECYCLE "Enable component lifecycle and verbose routing/window debug output (default: ON for Debug)" ON)

# Cars app: invoice export, etc. When OFF, registerAppHandlers is a no-op (weak stub).
option(BUILD_CARS_APP "Build Cars app module (createInvoice handler)" ON)
if(COMPONENT_DEBUG_LIFECYCLE OR CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(COMPONENT_DEBUG_LIFECYCLE=1)
    message(STATUS "Component lifecycle debug: ENABLED")
else()
    message(STATUS "Component lifecycle debug: disabled")
endif()

# Platform detection
if(APPLE)
    if(IOS)
        set(PLATFORM "ios")
    else()
        set(PLATFORM "macos")
    endif()
elseif(WIN32)
    set(PLATFORM "windows")
elseif(UNIX)
    set(PLATFORM "linux")
else()
    message(FATAL_ERROR "Unsupported platform")
endif()

message(STATUS "Building for platform: ${PLATFORM}")

# Use nlohmann/json library (needed for all platforms) - local header only, NO download
if(EXISTS "${CMAKE_SOURCE_DIR}/include/nlohmann/json.hpp")
    message(STATUS "nlohmann/json: using local include/nlohmann/json.hpp (no download)")
    set(JSON_USE_LOCAL_FILE TRUE)
else()
    message(FATAL_ERROR 
        "nlohmann/json.hpp not found at ${CMAKE_SOURCE_DIR}/include/nlohmann/json.hpp\n"
        "Please download it manually:\n"
        "  mkdir -p include/nlohmann\n"
        "  curl -L -o include/nlohmann/json.hpp https://github.com/nlohmann/json/releases/download/v3.11.2/json.hpp\n"
        "\n"
        "Or visit: https://github.com/nlohmann/json/releases/download/v3.11.2/json.hpp"
    )
endif()

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Embed settings.html for local Settings window (chunked for MSVC C2026 string limit)
# Logic inlined to avoid dependency on external .in file
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/generated")
set(SETTINGS_EMBED_CPP "${CMAKE_BINARY_DIR}/generated/settings_embed.cpp")
set(_SE_CHUNK_SIZE 8000)
set(_SE_DELIM "SEmB")
file(READ "${CMAKE_SOURCE_DIR}/settings.html" _SE_BODY)
string(REPLACE "@" "@@" _SE_BODY "${_SE_BODY}")
string(LENGTH "${_SE_BODY}" _SE_TOTAL_LEN)
set(_SE_OUT "")
set(_SE_OFF 0)
while(_SE_OFF LESS _SE_TOTAL_LEN)
  math(EXPR _SE_REMAIN "${_SE_TOTAL_LEN} - ${_SE_OFF}")
  if(_SE_REMAIN LESS _SE_CHUNK_SIZE)
    set(_SE_CHUNK_LEN ${_SE_REMAIN})
  else()
    set(_SE_CHUNK_LEN ${_SE_CHUNK_SIZE})
  endif()
  string(SUBSTRING "${_SE_BODY}" ${_SE_OFF} ${_SE_CHUNK_LEN} _SE_CHUNK)
  set(_SE_OUT "${_SE_OUT}R\"${_SE_DELIM}(${_SE_CHUNK})${_SE_DELIM}\" ")
  math(EXPR _SE_OFF "${_SE_OFF} + ${_SE_CHUNK_LEN}")
endwhile()
set(_SE_CPP "#include \"settings_embed.h\"
#include <string>

// Auto-generated from settings.html - do not edit. Chunked for MSVC string limit.

const std::string& getEmbeddedSettingsHtml() {
    static const std::string html = ${_SE_OUT};
    return html;
}
")
file(WRITE "${SETTINGS_EMBED_CPP}" "${_SE_CPP}")
message(STATUS "Settings HTML embedded from cross_dev/settings.html (chunked for MSVC)")

# Core sources (main.cpp excluded for iOS - AppDelegate.mm provides main)
set(CORE_SOURCES
    src/base64.cpp
    src/component.cpp
    src/control.cpp
    src/native_event_bus.cpp
    src/window.cpp
    src/webview.cpp
    src/webview_window.cpp
    src/singleton_webview_window_manager.cpp
    src/button.cpp
    src/input_field.cpp
    src/container.cpp
    src/layout.cpp
    src/vertical_layout.cpp
    src/horizontal_layout.cpp
    src/application.cpp
    src/event_handler.cpp
    src/message_router.cpp
    src/config_manager.cpp
    src/app_runner.cpp
    src/handlers/create_window_handler.cpp
    src/handlers/app_info_handler.cpp
    src/handlers/calculator_handler.cpp
    src/handlers/file_dialog_handler.cpp
    src/handlers/read_file_handler.cpp
    src/handlers/write_file_handler.cpp
    src/handlers/file_system_handler.cpp
    src/handlers/context_menu_handler.cpp
    src/handlers/focus_window_handler.cpp
    src/handlers/options_handler.cpp
    ${SETTINGS_EMBED_CPP}
)

# Add main.cpp for non-iOS platforms
if(NOT PLATFORM STREQUAL "ios")
    list(APPEND CORE_SOURCES src/main.cpp)
endif()

# App handlers: stub (no-op) when no app; apps provide strong symbol when BUILD_CARS_APP.
# On Windows/MSVC we must exclude stub when app is linked (no weak symbol support).
if(BUILD_CARS_APP AND NOT PLATFORM STREQUAL "ios")
    # app_cars provides registerAppHandlers; stub not needed
else()
    list(APPEND CORE_SOURCES src/app_handlers_stub.cpp)
endif()

# Platform-specific sources
if(PLATFORM STREQUAL "macos")
    # macOS sources (Objective-C++)
    set(PLATFORM_SOURCES
        src/platform/macos/app_macos.mm
        src/platform/macos/window_macos.mm
        src/platform/macos/webview_macos.mm
        src/platform/macos/button_macos.mm
        src/platform/macos/filedialog_macos.mm
        src/platform/macos/input_macos.mm
        src/platform/macos/container_macos.mm
        src/platform/macos/menu_macos.mm
    )
    
    # Enable Objective-C++ for macOS
    set(CMAKE_OBJCXX_STANDARD 17)
    set(CMAKE_OBJCXX_STANDARD_REQUIRED ON)
    
    # macOS frameworks
    find_library(COCOA_FRAMEWORK Cocoa)
    find_library(FOUNDATION_FRAMEWORK Foundation)
    find_library(WEBKIT_FRAMEWORK WebKit)
    find_library(UNIFORMTYPEIDENTIFIERS_FRAMEWORK UniformTypeIdentifiers)
    
    set(PLATFORM_LIBS
        ${COCOA_FRAMEWORK}
        ${FOUNDATION_FRAMEWORK}
        ${WEBKIT_FRAMEWORK}
        ${UNIFORMTYPEIDENTIFIERS_FRAMEWORK}
    )
    
elseif(PLATFORM STREQUAL "windows")
    # Windows sources
    set(PLATFORM_SOURCES
        src/platform/windows/app_windows.cpp
        src/platform/windows/window_windows.cpp
        src/platform/windows/webview_windows.cpp
        src/platform/windows/button_windows.cpp
        src/platform/windows/filedialog_windows.cpp
        src/platform/windows/input_windows.cpp
        src/platform/windows/container_windows.cpp
        src/platform/windows/menu_windows.cpp
    )
    
    # Windows-specific flags (PLATFORM_WINDOWS ensures platform impl #ifdef blocks compile)
    add_definitions(-DUNICODE -D_UNICODE -DPLATFORM_WINDOWS)
    
    # MinGW-specific flags (only apply if using MinGW)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mwindows")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -mwindows")
    endif()
    
    # Windows libraries
    set(PLATFORM_LIBS
        comctl32
        shlwapi
    )
    
    # WebView2 support - use local SDK if present, else FetchContent download.
    set(WEBVIEW2_ENABLED FALSE)
    set(webview2_SOURCE_DIR "")
    
    if(EXISTS "${CMAKE_SOURCE_DIR}/WebView2/build/native/include/WebView2.h")
        set(webview2_SOURCE_DIR "${CMAKE_SOURCE_DIR}/WebView2")
        set(WEBVIEW2_ENABLED TRUE)
        message(STATUS "WebView2: using LOCAL SDK from ${CMAKE_SOURCE_DIR}/WebView2")
    elseif(DEFINED WEBVIEW2_SDK_PATH AND EXISTS "${WEBVIEW2_SDK_PATH}/build/native/include/WebView2.h")
        set(webview2_SOURCE_DIR "${WEBVIEW2_SDK_PATH}")
        set(WEBVIEW2_ENABLED TRUE)
        message(STATUS "WebView2: using LOCAL SDK from WEBVIEW2_SDK_PATH")
    else()
        # Download via FetchContent (NuGet package)
        include(FetchContent)
        FetchContent_Declare(
            webview2
            URL https://www.nuget.org/api/v2/package/Microsoft.Web.WebView2/1.0.2592.51
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        )
        FetchContent_MakeAvailable(webview2)
        if(webview2_SOURCE_DIR AND EXISTS "${webview2_SOURCE_DIR}/build/native/include/WebView2.h")
            set(WEBVIEW2_ENABLED TRUE)
            message(STATUS "WebView2: using SDK from FetchContent (downloaded)")
        else()
            message(WARNING "WebView2: SDK download may have failed - building with fallback")
        endif()
    endif()
    
    # For MSVC: use CONSOLE when Debug or lifecycle logging enabled (so output is visible)
    if(MSVC)
        if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_CONFIGURATION_TYPES OR COMPONENT_DEBUG_LIFECYCLE)
            set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:CONSOLE")
        else()
            set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
        endif()
    endif()
    
elseif(PLATFORM STREQUAL "linux")
    # Linux sources
    set(PLATFORM_SOURCES
        src/platform/linux/app_linux.cpp
        src/platform/linux/window_linux.cpp
        src/platform/linux/webview_linux.cpp
        src/platform/linux/button_linux.cpp
        src/platform/linux/filedialog_linux.cpp
        src/platform/linux/input_linux.cpp
        src/platform/linux/container_linux.cpp
    )
    
    # Find pkg-config packages
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
    pkg_check_modules(WEBKIT2 REQUIRED webkit2gtk-4.0)
    pkg_check_modules(X11 REQUIRED x11)
    
    include_directories(${GTK3_INCLUDE_DIRS})
    include_directories(${WEBKIT2_INCLUDE_DIRS})
    include_directories(${X11_INCLUDE_DIRS})
    
    set(PLATFORM_LIBS
        ${GTK3_LIBRARIES}
        ${WEBKIT2_LIBRARIES}
        ${X11_LIBRARIES}
    )
    
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GTK3_CFLAGS_OTHER} ${WEBKIT2_CFLAGS_OTHER} ${X11_CFLAGS_OTHER}")
    
elseif(PLATFORM STREQUAL "ios")
    # iOS sources (Objective-C++)
    set(PLATFORM_SOURCES
        src/platform/ios/AppDelegate.mm
        src/platform/ios/app_ios.mm
        src/platform/ios/window_ios.mm
        src/platform/ios/webview_ios.mm
        src/platform/ios/button_ios.mm
        src/platform/ios/filedialog_ios.mm
        src/platform/ios/input_ios.mm
        src/platform/ios/container_ios.mm
        src/platform/ios/menu_ios.mm
    )
    
    # Enable Objective-C++ for iOS
    set(CMAKE_OBJCXX_STANDARD 17)
    set(CMAKE_OBJCXX_STANDARD_REQUIRED ON)
    
    # Enable ARC (Automatic Reference Counting) for iOS
    # Set via Xcode attributes so Xcode respects it
    set(CMAKE_XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC "YES")
    set(CMAKE_OBJCXX_FLAGS "${CMAKE_OBJCXX_FLAGS} -fobjc-arc")
    set(CMAKE_OBJC_FLAGS "${CMAKE_OBJC_FLAGS} -fobjc-arc")
    
    # Set proper SDK for simulator vs device
    # Xcode will handle this automatically, but we can set defaults
    set(CMAKE_XCODE_ATTRIBUTE_SDKROOT "iphoneos")
    set(CMAKE_XCODE_ATTRIBUTE_SUPPORTED_PLATFORMS "iphoneos iphonesimulator")
    
    # iOS frameworks - don't use find_library for iOS frameworks
    # They will be linked directly by name in target_link_libraries
    set(PLATFORM_LIBS "")
    
    # iOS-specific settings
    set(CMAKE_SYSTEM_NAME iOS)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0")
    
    # Set iOS SDK path for find_library to work
    execute_process(
        COMMAND xcrun --sdk iphoneos --show-sdk-path
        OUTPUT_VARIABLE IOS_SDK_PATH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    if(IOS_SDK_PATH)
        set(CMAKE_OSX_SYSROOT ${IOS_SDK_PATH})
        message(STATUS "iOS SDK: ${IOS_SDK_PATH}")
    endif()
endif()

# All sources
set(ALL_SOURCES ${CORE_SOURCES} ${PLATFORM_SOURCES})
list(LENGTH ALL_SOURCES _src_count)
message(STATUS "Total sources: ${_src_count} (CORE + PLATFORM)")

# Create executable
if(PLATFORM STREQUAL "ios")
    # For iOS, create an app bundle
    add_executable(${PROJECT_NAME} MACOSX_BUNDLE ${ALL_SOURCES})
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/ios/Info.plist"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.example.${PROJECT_NAME}"
        MACOSX_BUNDLE_BUNDLE_NAME "${PROJECT_NAME}"
        XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC "YES"
        XCODE_ATTRIBUTE_SDKROOT "iphoneos"
        XCODE_ATTRIBUTE_SUPPORTED_PLATFORMS "iphoneos iphonesimulator"
        XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"
        XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH "NO"
        XCODE_ATTRIBUTE_VALID_ARCHS "arm64 x86_64"
        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.example.${PROJECT_NAME}"
        XCODE_ATTRIBUTE_OTHER_LDFLAGS "$(inherited) -framework UIKit -framework Foundation -framework WebKit"
    )
else()
    add_executable(${PROJECT_NAME} ${ALL_SOURCES})
endif()

# Link libraries
if(PLATFORM_LIBS)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${PLATFORM_LIBS})
endif()

# OpenXLSX for Excel export (template-based .xlsx: copy template, fill cells)
# Use external/OpenXLSX if present (e.g. git submodule), else FetchContent
if(EXISTS "${CMAKE_SOURCE_DIR}/external/OpenXLSX/CMakeLists.txt")
    set(OPENXLSX_CREATE_DOCS OFF CACHE BOOL "" FORCE)
    set(OPENXLSX_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
    set(OPENXLSX_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    add_subdirectory(external/OpenXLSX)
    target_link_libraries(${PROJECT_NAME} PRIVATE OpenXLSX::OpenXLSX)
    message(STATUS "OpenXLSX: using local external/OpenXLSX")
else()
    include(FetchContent)
    FetchContent_Declare(
        OpenXLSX
        GIT_REPOSITORY https://github.com/troldal/OpenXLSX.git
        GIT_TAG master
        GIT_SHALLOW TRUE
    )
    set(OPENXLSX_CREATE_DOCS OFF CACHE BOOL "" FORCE)
    set(OPENXLSX_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
    set(OPENXLSX_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(OpenXLSX)
    target_link_libraries(${PROJECT_NAME} PRIVATE OpenXLSX::OpenXLSX)
    message(STATUS "OpenXLSX: fetched via FetchContent")
endif()

# Ensure PLATFORM_WINDOWS is defined for Windows builds (needed for platform impl #ifdef)
if(PLATFORM STREQUAL "windows")
    target_compile_definitions(${PROJECT_NAME} PRIVATE PLATFORM_WINDOWS)
endif()

# iOS frameworks - multiple approaches to ensure they're linked
if(PLATFORM STREQUAL "ios")
    # Approach 1: Use target_link_options (CMake 3.13+)
    target_link_options(${PROJECT_NAME} PRIVATE
        "-framework" "UIKit"
        "-framework" "Foundation"
        "-framework" "WebKit"
    )
    
    # Approach 2: Also set via LINK_FLAGS as backup
    set_target_properties(${PROJECT_NAME} PROPERTIES
        LINK_FLAGS "-framework UIKit -framework Foundation -framework WebKit"
    )
    
    message(STATUS "Linking iOS frameworks: UIKit, Foundation, WebKit")
endif()

# Include nlohmann/json (for all platforms)
if(JSON_USE_LOCAL_FILE)
    # Using manually downloaded file - include directory is already in include_directories
    message(STATUS "nlohmann/json: Using local file from include/nlohmann/json.hpp")
elseif(TARGET nlohmann_json::nlohmann_json)
    target_link_libraries(${PROJECT_NAME} PRIVATE nlohmann_json::nlohmann_json)
elseif(json_SOURCE_DIR)
    target_include_directories(${PROJECT_NAME} PRIVATE ${json_SOURCE_DIR}/include)
endif()

# WebView2 support (Windows only)
if(PLATFORM STREQUAL "windows" AND WEBVIEW2_ENABLED)
    target_include_directories(${PROJECT_NAME} PRIVATE ${webview2_SOURCE_DIR}/build/native/include)
    target_compile_definitions(${PROJECT_NAME} PRIVATE HAVE_WEBVIEW2)
    
    # Link WebView2 loader library
    # Determine architecture
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(WEBVIEW2_ARCH "x64")
    else()
        set(WEBVIEW2_ARCH "x86")
    endif()
    
    # WebView2 NuGet package structure: build/native/[arch]/
    # The library might be in different locations depending on package version
    set(WEBVIEW2_LIB_DIR "${webview2_SOURCE_DIR}/build/native/${WEBVIEW2_ARCH}")
    
    # Debug output
    message(STATUS "WebView2 library directory: ${WEBVIEW2_LIB_DIR}")
    message(STATUS "WebView2 source dir: ${webview2_SOURCE_DIR}")
    
    # WebView2 NuGet package typically has the loader in build/native/[arch]/
    # But some versions might use different structures
    # Check what actually exists
    if(EXISTS "${webview2_SOURCE_DIR}/build/native")
        file(GLOB_RECURSE WEBVIEW2_LIB_FILES "${webview2_SOURCE_DIR}/build/native/*.lib")
        if(WEBVIEW2_LIB_FILES)
            message(STATUS "Found WebView2 .lib files:")
            foreach(LIB ${WEBVIEW2_LIB_FILES})
                message(STATUS "  ${LIB}")
            endforeach()
        endif()
    endif()
    
    # List possible library locations and names
    set(WEBVIEW2_POSSIBLE_LIBS
        "${WEBVIEW2_LIB_DIR}/WebView2LoaderStatic.lib"
        "${WEBVIEW2_LIB_DIR}/WebView2LoaderStaticd.lib"
        "${WEBVIEW2_LIB_DIR}/WebView2Loader.lib"
        "${WEBVIEW2_LIB_DIR}/WebView2Loaderd.lib"
    )
    
    # Also check in lib/ subdirectory (some package versions use this)
    list(APPEND WEBVIEW2_POSSIBLE_LIBS
        "${webview2_SOURCE_DIR}/build/native/lib/${WEBVIEW2_ARCH}/WebView2LoaderStatic.lib"
        "${webview2_SOURCE_DIR}/build/native/lib/${WEBVIEW2_ARCH}/WebView2LoaderStaticd.lib"
        "${webview2_SOURCE_DIR}/lib/${WEBVIEW2_ARCH}/WebView2LoaderStatic.lib"
        "${webview2_SOURCE_DIR}/lib/${WEBVIEW2_ARCH}/WebView2LoaderStaticd.lib"
    )
    
    # Find the actual library file
    set(WEBVIEW2_FOUND_LIB "")
    foreach(LIB_PATH ${WEBVIEW2_POSSIBLE_LIBS})
        if(EXISTS "${LIB_PATH}")
            set(WEBVIEW2_FOUND_LIB "${LIB_PATH}")
            message(STATUS "Found WebView2 library: ${LIB_PATH}")
            break()
        endif()
    endforeach()
    
    if(WEBVIEW2_FOUND_LIB)
        # Link the library using full path
        target_link_libraries(${PROJECT_NAME} PRIVATE "${WEBVIEW2_FOUND_LIB}")
        message(STATUS "WebView2 library linked successfully")
    else()
        # The NuGet package might not include static libraries - WebView2 uses runtime loading
        # In that case, we need to use the DLL loader or link differently
        message(WARNING "WebView2 static loader library not found")
        message(WARNING "  Searched in: ${WEBVIEW2_POSSIBLE_LIBS}")
        message(WARNING "  WebView2 may use runtime DLL loading - linking may fail")
        message(WARNING "  Check if WebView2Loader.dll is available at runtime")
        
        # Try to add the directory anyway in case CMake can find it
        target_link_directories(${PROJECT_NAME} PRIVATE ${WEBVIEW2_LIB_DIR})
        # Try common library names
        target_link_libraries(${PROJECT_NAME} PRIVATE WebView2LoaderStatic WebView2LoaderStaticd WebView2Loader)
    endif()
endif()

# Compiler warnings
target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)

# iOS-specific compiler options
if(PLATFORM STREQUAL "ios")
    # Enable ARC for all Objective-C++ files
    target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<COMPILE_LANGUAGE:OBJCXX>:-fobjc-arc>
        $<$<COMPILE_LANGUAGE:OBJC>:-fobjc-arc>
    )
    
    # Suppress availability warnings from system headers (they're harmless)
    target_compile_options(${PROJECT_NAME} PRIVATE
        -Wno-unguarded-availability
        -Wno-unsupported-availability-guard
    )
endif()

# On Windows, avoid double .exe (use base name only; CMake adds .exe)
if(PLATFORM STREQUAL "windows")
    set_target_properties(${PROJECT_NAME} PROPERTIES
        RUNTIME_OUTPUT_NAME ${PROJECT_NAME}
        RUNTIME_OUTPUT_NAME_DEBUG ${PROJECT_NAME}
        RUNTIME_OUTPUT_NAME_RELEASE ${PROJECT_NAME}
    )
endif()

# Installation (optional)
if(PLATFORM STREQUAL "ios")
    # iOS app bundle installation
    install(TARGETS ${PROJECT_NAME}
        BUNDLE DESTINATION .
        RUNTIME DESTINATION bin
    )
else()
    install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION bin
    )
endif()

# Testing
enable_testing()

# Test executables
add_executable(test_component tests/test_component.cpp src/component.cpp)
target_include_directories(test_component PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME ComponentTests COMMAND test_component)

add_executable(test_control tests/test_control.cpp src/component.cpp src/control.cpp tests/mock_platform.cpp)
target_include_directories(test_control PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME ControlTests COMMAND test_control)

# Tests that require platform implementations (using mock platform)
add_executable(test_button tests/test_button.cpp src/component.cpp src/control.cpp src/button.cpp src/window.cpp tests/mock_platform.cpp)
target_include_directories(test_button PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME ButtonTests COMMAND test_button)

add_executable(test_container tests/test_container.cpp src/component.cpp src/control.cpp src/container.cpp src/button.cpp src/window.cpp src/layout.cpp src/vertical_layout.cpp src/horizontal_layout.cpp tests/mock_platform.cpp)
target_include_directories(test_container PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME ContainerTests COMMAND test_container)

add_executable(test_ownership tests/test_ownership.cpp src/component.cpp src/control.cpp src/window.cpp src/button.cpp src/container.cpp src/layout.cpp src/vertical_layout.cpp src/horizontal_layout.cpp tests/mock_platform.cpp)
target_include_directories(test_ownership PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME OwnershipTests COMMAND test_ownership)

add_executable(test_component_collection tests/test_component_collection.cpp src/component.cpp src/component_collection.cpp)
target_include_directories(test_component_collection PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME ComponentCollectionTests COMMAND test_component_collection)

add_executable(test_layout tests/test_layout.cpp src/component.cpp src/control.cpp src/layout.cpp src/vertical_layout.cpp src/horizontal_layout.cpp src/container.cpp src/button.cpp src/window.cpp tests/mock_platform.cpp)
target_include_directories(test_layout PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME LayoutTests COMMAND test_layout)

# Example: Layout and Component System Demo
if(NOT PLATFORM STREQUAL "ios")
    # Create a list of sources without main.cpp for the demo
    set(LAYOUT_DEMO_SOURCES ${CORE_SOURCES})
    list(REMOVE_ITEM LAYOUT_DEMO_SOURCES src/main.cpp)
    
    add_executable(layout_demo examples/layout_demo.cpp ${LAYOUT_DEMO_SOURCES} ${PLATFORM_SOURCES})
    target_include_directories(layout_demo PRIVATE ${CMAKE_SOURCE_DIR}/include)
    target_link_libraries(layout_demo PRIVATE ${PLATFORM_LIBS})
    if(PLATFORM STREQUAL "macos" AND PDFKIT_FRAMEWORK)
        target_link_libraries(layout_demo PRIVATE ${PDFKIT_FRAMEWORK})
    endif()
    if(PLATFORM STREQUAL "windows")
        target_compile_definitions(layout_demo PRIVATE PLATFORM_WINDOWS)
    endif()
    set_target_properties(layout_demo PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples
    )

    # Excel module: reusable template-based export (see docs/EXCEL_EXPORT_DESIGN.md)
    add_library(excel_excel STATIC
        src/excel/excel_sheet_ops.cpp
        src/excel/excel_paths.cpp
        src/excel/excel_exporter.cpp
        src/excel/excel_image.cpp
        src/excel/excel_invoice.cpp
    )
    target_include_directories(excel_excel PUBLIC ${CMAKE_SOURCE_DIR}/include)
    target_link_libraries(excel_excel PUBLIC OpenXLSX::OpenXLSX)
    target_compile_options(excel_excel PRIVATE -Wno-defaulted-function-deleted)

    # Excel template test: uses excel module, opens output in default app
    add_executable(excel_template_test examples/excel_template_test.cpp)
    target_link_libraries(excel_template_test PRIVATE excel_excel)
    target_compile_options(excel_template_test PRIVATE -Wno-defaulted-function-deleted)
    if(PLATFORM STREQUAL "windows")
        target_link_libraries(excel_template_test PRIVATE shell32)
    endif()
    set_target_properties(excel_template_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples
    )

    # Beautiful invoice test: creates styled invoice with logo
    add_executable(excel_invoice_test examples/excel_invoice_test.cpp)
    target_link_libraries(excel_invoice_test PRIVATE excel_excel)
    target_compile_options(excel_invoice_test PRIVATE -Wno-defaulted-function-deleted)
    if(PLATFORM STREQUAL "windows")
        target_link_libraries(excel_invoice_test PRIVATE shell32)
    endif()
    set_target_properties(excel_invoice_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples
    )

    # Cars app: createInvoice handler, etc. Strong registerAppHandlers overrides weak stub.
    if(BUILD_CARS_APP AND EXISTS "${CMAKE_SOURCE_DIR}/apps/cars")
        add_subdirectory(apps/cars)
        target_link_libraries(${PROJECT_NAME} PRIVATE app_cars)
        target_link_libraries(layout_demo PRIVATE app_cars)
        message(STATUS "Cars app: ENABLED (createInvoice handler)")
    elseif(BUILD_CARS_APP)
        message(STATUS "Cars app: skipped (apps/cars not found; build from cars repo or disable with -DBUILD_CARS_APP=OFF)")
    else()
        message(STATUS "Cars app: disabled (using weak stub)")
    endif()
endif()

# Optional: MS Access to SQLite conversion tool (Windows only; uses in-repo SQLite)
# Default: tools/sqlite_direct_minimal/sqlite (or sibling ../ when cross_dev is inside cars repo).
# Override: -DSQLITE_AMALGAMATION_DIR=C:/path/to/sqlite_direct_minimal/sqlite
if(NOT DEFINED SQLITE_AMALGAMATION_DIR)
  if(EXISTS "${CMAKE_SOURCE_DIR}/tools/sqlite_direct_minimal/sqlite/sqlite3.c")
    set(SQLITE_AMALGAMATION_DIR "${CMAKE_SOURCE_DIR}/tools/sqlite_direct_minimal/sqlite")
  else()
    set(SQLITE_AMALGAMATION_DIR "${CMAKE_SOURCE_DIR}/../sqlite_direct_minimal/sqlite")
  endif()
endif()
if(WIN32)
  if(EXISTS "${SQLITE_AMALGAMATION_DIR}/sqlite3.c" AND EXISTS "${SQLITE_AMALGAMATION_DIR}/CMakeLists.txt")
    add_subdirectory(${SQLITE_AMALGAMATION_DIR} ${CMAKE_BINARY_DIR}/sqlite_build)
    set(MSACCESS_TO_SQLITE_SQLite3_AVAILABLE 1)
  endif()
  if(MSACCESS_TO_SQLITE_SQLite3_AVAILABLE)
    add_subdirectory(tools/msaccess_to_sqlite)
    message(STATUS "msaccess_to_sqlite: ENABLED (using sqlite_direct_minimal)")
  else()
    message(STATUS "msaccess_to_sqlite: skipped (sqlite_direct_minimal/sqlite not found)")
  endif()
endif()

# Note: test_window requires full platform libraries, so we'll skip it in test builds
# It will be tested when building the main application
