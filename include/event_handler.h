#ifndef EVENT_HANDLER_H
#define EVENT_HANDLER_H

#include "webview_window.h"
#include "message_handler.h"
#include <string>
#include <functional>
#include <map>
#include <memory>
#include <vector>

class Window;
class WebView;
class MessageRouter;

// Event handler for managing HTML/webview events
class EventHandler {
public:
    EventHandler(Window* window, WebView* webView);
    ~EventHandler();

    // Register callback for webview create window event (9-param: name, title, contentType, content, isSingleton, x, y, width, height)
    void onWebViewCreateWindow(std::function<void(const std::string& name, const std::string& title, WebViewContentType contentType, const std::string& content, bool isSingleton, int x, int y, int width, int height)> callback);

    // Overload for 3-param callback (title, contentType, content) - uses default 900x700
    void onWebViewCreateWindow(std::function<void(const std::string& title, WebViewContentType contentType, const std::string& content)> callback);

    // Attach a child window's WebView so CrossDev.invoke (e.g. createWindow) works from it.
    void attachWebView(WebView* webView);
    void attachWebView(WebView* webView, std::vector<std::shared_ptr<MessageHandler>> extraHandlers);
    
    // Get the message router (for registering additional handlers)
    MessageRouter* getMessageRouter() { return messageRouter_.get(); }
    std::shared_ptr<MessageRouter> getMessageRouterShared() { return messageRouter_; }

private:
    Window* window_;
    WebView* webView_;
    std::shared_ptr<MessageRouter> messageRouter_;
    std::function<void(const std::string&, const std::string&, WebViewContentType, const std::string&, bool, int, int, int, int)> createWindowCallback_;
    std::vector<std::unique_ptr<MessageRouter>> attachedRouters_;
};

#endif // EVENT_HANDLER_H
