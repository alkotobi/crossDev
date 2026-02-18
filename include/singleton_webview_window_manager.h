#ifndef SINGLETON_WEBVIEW_WINDOW_MANAGER_H
#define SINGLETON_WEBVIEW_WINDOW_MANAGER_H

#include "webview_window.h"
#include <functional>
#include <map>
#include <mutex>
#include <string>

// Manages singleton WebViewWindow instances by name.
// Use the window "name" (distinct identifier) not "title" (user-facing string).
// If a window with the given name already exists, show it instead of creating a new one.
class SingletonWebViewWindowManager {
public:
    static constexpr const char* MAIN_WINDOW_NAME = "MainWindows";

    static SingletonWebViewWindowManager& getInstance();

    // Non-copyable
    SingletonWebViewWindowManager(const SingletonWebViewWindowManager&) = delete;
    SingletonWebViewWindowManager& operator=(const SingletonWebViewWindowManager&) = delete;

    // Get existing window or create new one. Returns nullptr if name is empty (caller creates normally).
    // name: unique identifier for singleton lookup (not the displayed title)
    // attachFn: called with the new window's WebView after creation (for message routing)
    WebViewWindow* getOrCreate(
        const std::string& name,
        const std::string& title,
        WebViewContentType contentType,
        const std::string& content,
        Component* parent,
        std::function<void(WebView*)> attachFn,
        int x = 150, int y = 150, int width = 900, int height = 700);

    // Remove a name from the map (called when window is destroyed)
    void unregister(const std::string& name);

    // Register an existing window by name (e.g. main window as MAIN_WINDOW_NAME).
    // Name lookup is case-insensitive.
    void registerWindow(const std::string& name, WebViewWindow* window);

    // Register a focus callback for windows that are not registered via registerWindow.
    // When focusWindow(name) is called, the callback is invoked to show/bring to front.
    void registerFocusCallback(const std::string& name, std::function<void()> onFocus);

    // Get window by name (case-insensitive). Returns nullptr if not found.
    WebViewWindow* getWindow(const std::string& name) const;

    // Try to focus a window by name. Returns true if found and focused (WebViewWindow or registered callback).
    bool focusWindow(const std::string& name) const;

private:
    SingletonWebViewWindowManager() = default;
    ~SingletonWebViewWindowManager() = default;

    static std::string toLower(const std::string& s);

    mutable std::mutex mutex_;
    std::map<std::string, WebViewWindow*> windows_;  // key = lowercase name
    std::map<std::string, std::function<void()>> focusCallbacks_;
};

#endif // SINGLETON_WEBVIEW_WINDOW_MANAGER_H
