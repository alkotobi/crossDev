#ifndef WEBVIEW_WINDOW_H
#define WEBVIEW_WINDOW_H

#include "component.h"
#include "window.h"
#include "webview.h"
#include <string>
#include <functional>
#include <memory>

// Content type for WebViewWindow initial load
enum class WebViewContentType {
    Default,  // Use simple default HTML (when none supplied)
    Html,     // content is HTML code
    Url,      // content is URL to load
    File      // content is path to HTML file
};

// A window that contains a WebView that fills the entire window.
// Inherits from Component for owner-based automatic cleanup.
// The first WebViewWindow created with owner=nullptr becomes the "main" window.
// All other WebViewWindows should use the main (or another WebViewWindow) as owner
// so they are automatically freed when the owner is destroyed.
//
// Circular ownership prevention: Component::SetOwner (and the constructor) validate
// that setting an owner cannot create a cycle (A owns B owns A). A cycle will throw
// std::runtime_error("Circular ownership detected"). Same for self-ownership.
class WebViewWindow : public Component {
public:
    // Get the main WebViewWindow (first created with owner=nullptr). Returns nullptr if none.
    static WebViewWindow* GetMainWebViewWindow();

    // Constructor: owner=nullptr for the main window; use GetMainWebViewWindow() or another
    // WebViewWindow as owner for child windows (they will be auto-freed when owner is destroyed).
    // Owner can also be assigned manually via SetOwner().
    WebViewWindow(Component* owner, int x, int y, int width, int height, const std::string& title,
                  WebViewContentType type = WebViewContentType::Default,
                  const std::string& content = "");
    ~WebViewWindow();

    // Non-copyable
    WebViewWindow(const WebViewWindow&) = delete;
    WebViewWindow& operator=(const WebViewWindow&) = delete;
    
    // Window operations
    void show();
    void hide();
    void setTitle(const std::string& title);
    bool isVisible() const;
    
    // WebView operations
    void loadHTMLFile(const std::string& filePath);
    void loadHTMLString(const std::string& html);
    void loadURL(const std::string& url);
    
    // Message handling
    void setCreateWindowCallback(std::function<void(const std::string& title)> callback);
    void setMessageCallback(std::function<void(const std::string& jsonMessage)> callback);
    void postMessageToJavaScript(const std::string& jsonMessage);
    
    // Close all owned child WebViewWindows (used before quit to tear down while run loop is active)
    void closeAllOwnedWebViewWindows();

    // Set main menu bar (macOS/Windows/Linux). Call after construction.
    void setMainMenu(const std::string& menuJson,
                    void (*itemCallback)(const std::string& itemId, void* userData),
                    void* userData = nullptr);

    // Called in destructor before destruction. Use for cleanup (e.g. unregister from singleton manager).
    void setOnDestroyCallback(std::function<void()> callback);

    // Get underlying window and webview (for advanced usage)
    Window* getWindow() { return window_.get(); }
    WebView* getWebView() { return webView_.get(); }
    const Window* getWindow() const { return window_.get(); }
    const WebView* getWebView() const { return webView_.get(); }
    
private:
    std::unique_ptr<Window> window_;
    std::unique_ptr<WebView> webView_;
    std::function<void()> onDestroyCallback_;
    
    // Handle window resize to update WebView size
    void onWindowResize(int newWidth, int newHeight);
    void onWindowMove(int x, int y);
    
    void registerResizeCallback();
    void registerMoveCallback();
    void registerFileDropCallback();
    void registerStateCallback();
    void registerCloseCallback();
    void registerMainWindowCloseCallback();
    void registerMainMenu();
    void registerEventCallbacks();
};

#endif // WEBVIEW_WINDOW_H
