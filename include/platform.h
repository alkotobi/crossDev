#ifndef PLATFORM_H
#define PLATFORM_H

// Platform detection (use #ifndef so CMake -D flags or prior includes don't cause redefinition)
#if defined(__APPLE__) && defined(__MACH__)
    #include <TargetConditionals.h>
    #if TARGET_OS_IPHONE || TARGET_OS_SIMULATOR
        #ifndef PLATFORM_IOS
        #define PLATFORM_IOS
        #endif
        #define PLATFORM_NAME "iOS"
    #else
        #ifndef PLATFORM_MACOS
        #define PLATFORM_MACOS
        #endif
        #define PLATFORM_NAME "macOS"
    #endif
#elif defined(_WIN32) || defined(_WIN64)
    #ifndef PLATFORM_WINDOWS
    #define PLATFORM_WINDOWS
    #endif
    #define PLATFORM_NAME "Windows"
#elif defined(__linux__)
    #ifndef PLATFORM_LINUX
    #define PLATFORM_LINUX
    #endif
    #define PLATFORM_NAME "Linux"
#else
    #error "Unsupported platform"
#endif

#endif // PLATFORM_H
