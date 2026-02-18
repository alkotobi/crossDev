#ifndef PLATFORM_IMPL_H
#define PLATFORM_IMPL_H

#include <string>
#include <vector>

// Platform-specific implementation interface
// Each platform implements these functions

namespace platform {
    // Window management
    void* createWindow(int x, int y, int width, int height, const std::string& title, void* userData = nullptr);
    void destroyWindow(void* handle);
    void showWindow(void* handle);
    void hideWindow(void* handle);
    void setWindowTitle(void* handle, const std::string& title);
    void maximizeWindow(void* handle);
    bool isWindowVisible(void* handle);
    
    // Web view management
    // parentHandle can be a Window or Container native handle
    void* createWebView(void* parentHandle, int x, int y, int width, int height);
    void destroyWebView(void* webViewHandle);
    void resizeWebView(void* webViewHandle, int width, int height);
    void loadHTMLFile(void* webViewHandle, const std::string& filePath);
    void loadHTMLString(void* webViewHandle, const std::string& html);
    void loadURL(void* webViewHandle, const std::string& url);
    void setWebViewCreateWindowCallback(void* webViewHandle, void (*callback)(const std::string& title, void* userData), void* userData);
    void setWebViewMessageCallback(void* webViewHandle, void (*callback)(const std::string& jsonMessage, void* userData), void* userData);
    // Optional: set custom preload script before message callback. Empty = use built-in bridge.
    void setWebViewPreloadScript(void* webViewHandle, const std::string& scriptContent);
    void postMessageToJavaScript(void* webViewHandle, const std::string& jsonMessage);
    // Execute JavaScript in the WebView (fire-and-forget; used for Edit menu: cut, copy, paste, etc.)
    void executeWebViewScript(void* webViewHandle, const std::string& script);
    // Open native print dialog for the webview content (File menu: Print)
    void printWebView(void* webViewHandle);

    // Window resize callback management
    void setWindowResizeCallback(void* windowHandle, void (*callback)(int width, int height, void* userData), void* userData);
    
    // Window move callback - called when window position changes (x, y in screen coords)
    void setWindowMoveCallback(void* windowHandle, void (*callback)(int x, int y, void* userData), void* userData);
    
    // File drop callback - called when user drops files onto the window.
    // pathsJson: JSON array of file path strings, e.g. ["/path/to/a","/path/to/b"]
    void setWindowFileDropCallback(void* windowHandle, void (*callback)(const std::string& pathsJson, void* userData), void* userData);
    
    // Window close callback - called when user closes the window (X button).
    // Pass callback that deletes the WebViewWindow. userData is the WebViewWindow* to delete.
    void setWindowCloseCallback(void* windowHandle, void (*callback)(void* userData), void* userData);
    
    // Window focus callbacks - called when window gains/loses key status.
    void setWindowFocusCallback(void* windowHandle, void (*callback)(void* userData), void* userData);
    void setWindowBlurCallback(void* windowHandle, void (*callback)(void* userData), void* userData);
    
    // Window state callback - called when window is minimized, maximized, or restored.
    // state is "minimize", "maximize", or "restore".
    void setWindowStateCallback(void* windowHandle, void (*callback)(const char* state, void* userData), void* userData);
    
    // Main menu bar (macOS menu bar, Windows/Linux window menu).
    // menuJson: array of {id, label, items:[{id, label, shortcut?}, ...]}, "-" id = separator.
    void setWindowMainMenu(void* windowHandle, const std::string& menuJson,
                          void (*itemCallback)(const std::string& itemId, void* userData), void* userData);
    
    // Context menu (right-click menu). parentHandle = window or control.
    // itemsJson: array of {id, label} or {id:"-"} for separator.
    void showContextMenu(void* parentHandle, int x, int y, const std::string& itemsJson,
                        void (*itemCallback)(const std::string& itemId, void* userData), void* userData);
    
    // Button management
    // parentHandle can be a Window or Container native handle
    void* createButton(void* parentHandle, int x, int y, int width, int height, const std::string& label, void* userData);
    void destroyButton(void* buttonHandle);
    void setButtonCallback(void* buttonHandle, void (*callback)(void*));
    
    // File dialog management
    bool showOpenFileDialog(void* windowHandle, const std::string& title, const std::string& filter, std::string& selectedPath);
    
    // Input field management
    // parentHandle can be a Window or Container native handle
    void* createInputField(void* parentHandle, int x, int y, int width, int height, const std::string& placeholder);
    void destroyInputField(void* inputHandle);
    void setInputText(void* inputHandle, const std::string& text);
    std::string getInputText(void* inputHandle);
    
    // Container functions
    void* createContainer(void* parentHandle, int x, int y, int width, int height, bool flipped = false);
    void destroyContainer(void* containerHandle);
    void resizeContainer(void* containerHandle, int x, int y, int width, int height);
    void showContainer(void* containerHandle);
    void hideContainer(void* containerHandle);
    void bringContainerToFront(void* containerHandle);
    void setContainerBackgroundColor(void* containerHandle, int red, int green, int blue);
    void setContainerBorderStyle(void* containerHandle, int borderStyle);
    
    // Application lifecycle
    void initApplication();
    void runApplication();
    void quitApplication();
    // App activate/deactivate (macOS: become/resign key; Windows/Linux: optional)
    void setAppActivateCallback(void (*callback)(void*), void* userData);
    void setAppDeactivateCallback(void (*callback)(void*), void* userData);
    
    // Theme change (dark/light). Callback receives "dark" or "light".
    // macOS: AppleInterfaceThemeChangedNotification; Windows: WM_SETTINGCHANGE; Linux: stub.
    void setThemeChangeCallback(void (*callback)(const char* theme, void* userData), void* userData);
    
    // Key shortcut - callback receives JSON: {"key":"s","modifiers":["meta"],"keyCode":83}
    // macOS: NSEvent localMonitor; Windows/Linux: stub.
    void setKeyShortcutCallback(void (*callback)(const std::string& payloadJson, void* userData), void* userData);
    
    // Called when OS requests app to open a file (e.g. Open Document, argv). path may be empty.
    void setAppOpenFileCallback(void (*callback)(const std::string& path, void* userData), void* userData);
    // Deliver file paths from argv to the callback (call after window is ready). Skips argv[0].
    void deliverOpenFilePaths(int argc, const char* argv[]);
}

#endif // PLATFORM_IMPL_H
