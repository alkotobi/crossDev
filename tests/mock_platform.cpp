// Mock platform implementation for unit tests
// This provides stub implementations of all platform functions
// so tests can run without requiring actual platform libraries

#include "../include/platform.h"
#include "../src/platform/platform_impl.h"
#include <string>
#include <map>
#include <cstdint>

namespace platform {

// Mock window data
struct MockWindowData {
    void* handle;
    bool visible;
    std::string title;
};

static std::map<void*, MockWindowData*> g_mockWindows;
static uintptr_t g_nextHandleValue = 1;

// Window functions
void* createWindow(int x, int y, int width, int height, const std::string& title, void* userData) {
    MockWindowData* data = new MockWindowData;
    data->handle = (void*)g_nextHandleValue++;
    data->visible = false;
    data->title = title;
    g_mockWindows[data->handle] = data;
    return data;
}

void destroyWindow(void* handle) {
    if (handle) {
        auto it = g_mockWindows.find(handle);
        if (it != g_mockWindows.end()) {
            delete it->second;
            g_mockWindows.erase(it);
        }
    }
}

void showWindow(void* handle) {
    if (handle) {
        auto it = g_mockWindows.find(handle);
        if (it != g_mockWindows.end()) {
            it->second->visible = true;
        }
    }
}

void hideWindow(void* handle) {
    if (handle) {
        auto it = g_mockWindows.find(handle);
        if (it != g_mockWindows.end()) {
            it->second->visible = false;
        }
    }
}

void setWindowTitle(void* handle, const std::string& title) {
    if (handle) {
        auto it = g_mockWindows.find(handle);
        if (it != g_mockWindows.end()) {
            it->second->title = title;
        }
    }
}

void maximizeWindow(void* handle) {
    (void)handle;
    // Mock implementation - no-op
}

bool isWindowVisible(void* handle) {
    if (handle) {
        auto it = g_mockWindows.find(handle);
        if (it != g_mockWindows.end()) {
            return it->second->visible;
        }
    }
    return false;
}

void setWindowResizeCallback(void* windowHandle, void (*callback)(int width, int height, void* userData), void* userData) {
    // Mock implementation - do nothing
}

void setWindowMoveCallback(void*, void (*)(int, int, void*), void*) {}
void setWindowFileDropCallback(void*, void (*)(const std::string&, void*), void*) {}

void setWindowCloseCallback(void*, void (*)(void*), void*) {
    // Mock implementation - do nothing
}

void setWindowFocusCallback(void*, void (*)(void*), void*) {}
void setWindowBlurCallback(void*, void (*)(void*), void*) {}
void setWindowStateCallback(void*, void (*)(const char*, void*), void*) {}

void setWindowMainMenu(void*, const std::string&, void (*)(const std::string&, void*), void*) {}

void showContextMenu(void*, int, int, const std::string&, void (*)(const std::string&, void*), void*) {}

void setMenuItemEnabled(void*, const std::string&, bool) {}

// WebView functions
void* createWebView(void* parentHandle, int x, int y, int width, int height) {
    return (void*)g_nextHandleValue++;
}

void destroyWebView(void* webViewHandle) {
    // Mock implementation
}

void resizeWebView(void* webViewHandle, int width, int height) {
    // Mock implementation
}

void loadHTMLFile(void* webViewHandle, const std::string& filePath) {
    // Mock implementation
}

void loadHTMLString(void* webViewHandle, const std::string& html) {
    // Mock implementation
}

void loadURL(void* webViewHandle, const std::string& url) {
    // Mock implementation
}

void setWebViewCreateWindowCallback(void* webViewHandle, void (*callback)(const std::string& title, void* userData), void* userData) {
    // Mock implementation
}

void setWebViewMessageCallback(void* webViewHandle, void (*callback)(const std::string& jsonMessage, void* userData), void* userData) {
    // Mock implementation
}

void setWebViewPreloadScript(void*, const std::string&) {}

void postMessageToJavaScript(void* webViewHandle, const std::string& jsonMessage) {
    // Mock implementation
}

void executeWebViewScript(void* webViewHandle, const std::string& script) {
    // Mock implementation
}

void printWebView(void* webViewHandle) {
    // Mock implementation
}

// Button functions
void* createButton(void* parentHandle, int x, int y, int width, int height, const std::string& label, void* userData) {
    return (void*)g_nextHandleValue++;
}

void destroyButton(void* buttonHandle) {
    // Mock implementation
}

void setButtonCallback(void* buttonHandle, void (*callback)(void*)) {
    // Mock implementation
}

// InputField functions
void* createInputField(void* parentHandle, int x, int y, int width, int height, const std::string& placeholder) {
    return (void*)g_nextHandleValue++;
}

void destroyInputField(void* inputHandle) {
    // Mock implementation
}

void setInputText(void* inputHandle, const std::string& text) {
    // Mock implementation
}

std::string getInputText(void* inputHandle) {
    return "";
}

// Container functions
void* createContainer(void* parentHandle, int x, int y, int width, int height, bool flipped) {
    (void)flipped;
    return (void*)g_nextHandleValue++;
}

void destroyContainer(void* containerHandle) {
    // Mock implementation
}

void resizeContainer(void* containerHandle, int x, int y, int width, int height) {
    // Mock implementation
}

void showContainer(void* containerHandle) {
    // Mock implementation
}

void hideContainer(void* containerHandle) {
    // Mock implementation
}

void bringContainerToFront(void* containerHandle) {
    // Mock implementation
}

void setContainerBackgroundColor(void* containerHandle, int red, int green, int blue) {
    // Mock implementation
}

void setContainerBorderStyle(void* containerHandle, int borderStyle) {
    // Mock implementation
}

// File dialog functions
bool showOpenFileDialog(void* windowHandle, const std::string& title, const std::string& filter, std::string& selectedPath) {
    return false;
}

// Application functions
void initApplication() {
    // Mock implementation
}

void runApplication() {
    // Mock implementation
}

void quitApplication() {
    // Mock implementation
}

void setAppActivateCallback(void (*)(void*), void*) {}
void setAppDeactivateCallback(void (*)(void*), void*) {}
void setThemeChangeCallback(void (*)(const char*, void*), void*) {}
void setKeyShortcutCallback(void (*)(const std::string&, void*), void*) {}
void setAppOpenFileCallback(void (*)(const std::string&, void*), void*) {}
void deliverOpenFilePaths(int, const char**) {}

} // namespace platform
