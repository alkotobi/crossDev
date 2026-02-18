#include "../include/event_handler.h"
#include "../include/window.h"
#include "../include/webview.h"
#include "../include/message_router.h"
#include "../include/config_manager.h"
#include "../include/handlers/create_window_handler.h"
#include "platform/platform_impl.h"
#include <iostream>

EventHandler::EventHandler(Window* window, WebView* webView)
    : window_(window), webView_(webView) {
    if (!window_ || !webView_) {
        throw std::runtime_error("EventHandler requires valid window and webView");
    }
    
    // Set custom preload script if configured (must be before message callback)
    std::string preload = ConfigManager::getPreloadScriptContent();
    if (!preload.empty() && webView_->getNativeHandle()) {
        platform::setWebViewPreloadScript(webView_->getNativeHandle(), preload);
    }
    
    // Create message router (shared_ptr for handlers that cross async boundaries, e.g. context menu)
    messageRouter_ = std::make_shared<MessageRouter>(webView_);
    
    // Set up message callback to route all messages through MessageRouter
    webView_->setMessageCallback([this](const std::string& jsonMessage) {
        if (messageRouter_) {
            messageRouter_->routeMessage(jsonMessage);
        }
    });
}

EventHandler::~EventHandler() {
    // MessageRouter will be automatically destroyed
}

void EventHandler::onWebViewCreateWindow(std::function<void(const std::string& name, const std::string& title, WebViewContentType contentType, const std::string& content, bool isSingleton, int x, int y, int width, int height)> callback) {
    createWindowCallback_ = std::move(callback);
    if (messageRouter_ && createWindowCallback_) {
        messageRouter_->registerHandler(createCreateWindowHandler(createWindowCallback_));
    }
}

void EventHandler::onWebViewCreateWindow(std::function<void(const std::string& title, WebViewContentType contentType, const std::string& content)> callback) {
    createWindowCallback_ = [cb = std::move(callback)](const std::string&, const std::string& title, WebViewContentType type, const std::string& content, bool, int, int, int, int) {
        cb(title, type, content);
    };
    if (messageRouter_ && createWindowCallback_) {
        messageRouter_->registerHandler(createCreateWindowHandler(createWindowCallback_));
    }
}

void EventHandler::attachWebView(WebView* webView) {
    attachWebView(webView, {});
}

void EventHandler::attachWebView(WebView* webView, std::vector<std::shared_ptr<MessageHandler>> extraHandlers) {
    if (!webView) return;
    auto router = std::make_unique<MessageRouter>(webView);
    if (createWindowCallback_) {
        router->registerHandler(createCreateWindowHandler(createWindowCallback_));
    }
    for (auto& h : extraHandlers) {
        if (h) router->registerHandler(h);
    }
    MessageRouter* routerPtr = router.get();
    attachedRouters_.push_back(std::move(router));
    webView->setMessageCallback([routerPtr](const std::string& jsonMessage) {
        if (routerPtr) {
            routerPtr->routeMessage(jsonMessage);
        }
    });
}
