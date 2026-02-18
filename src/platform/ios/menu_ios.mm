// iOS menu stubs (menus differ on mobile - no traditional menu bar)
#include "../platform_impl.h"
#include <string>

#ifdef PLATFORM_IOS

namespace platform {

void setWindowMainMenu(void*, const std::string&, void (*)(const std::string&, void*), void*) {
    // No-op on iOS - no desktop-style menu bar
}

void showContextMenu(void*, int, int, const std::string&, void (*)(const std::string&, void*), void*) {
    // Stub - could implement with UIMenuController or similar if needed
}

} // namespace platform

#endif // PLATFORM_IOS
