#include "../include/webview.h"
#include "../include/control.h"
#include "platform/platform_impl.h"
#include <stdexcept>
#include <functional>

WebView::WebView(Component* owner, Control* parent, int x, int y, int width, int height)
    : Control(owner, parent), nativeHandle_(nullptr) {
    // Set bounds using Control's methods
    SetBounds(x, y, width, height);
    
    if (!GetParent() || !GetParent()->getNativeHandle()) {
        throw std::runtime_error("Parent control must be created before creating web view");
    }
    
    createNativeWebView();
    resetDefaultNameCache();
    debugLogLifecycleCreation(this, GetOwner(), GetParent());
}

WebView::~WebView() {
    if (nativeHandle_) {
        destroyNativeWebView();
    }
}

WebView::WebView(WebView&& other) noexcept
    : Control(std::move(other)),
      nativeHandle_(other.nativeHandle_),
      createWindowCallback_(std::move(other.createWindowCallback_)),
      messageCallback_(std::move(other.messageCallback_)) {
    other.nativeHandle_ = nullptr;
}

WebView& WebView::operator=(WebView&& other) noexcept {
    if (this != &other) {
        if (nativeHandle_) {
            destroyNativeWebView();
        }
        
        Control::operator=(std::move(other));
        nativeHandle_ = other.nativeHandle_;
        createWindowCallback_ = std::move(other.createWindowCallback_);
        messageCallback_ = std::move(other.messageCallback_);
        
        other.nativeHandle_ = nullptr;
    }
    return *this;
}

void WebView::createNativeWebView() {
    if (!GetParent() || !GetParent()->getNativeHandle()) {
        return;
    }
    
    nativeHandle_ = platform::createWebView(GetParent()->getNativeHandle(),
                                           GetLeft(), GetTop(),
                                           GetWidth(), GetHeight());
    if (!nativeHandle_) {
        throw std::runtime_error("Failed to create web view");
    }
}

void WebView::destroyNativeWebView() {
    if (nativeHandle_) {
        platform::destroyWebView(nativeHandle_);
        nativeHandle_ = nullptr;
    }
}

void WebView::loadHTMLFile(const std::string& filePath) {
    if (!nativeHandle_) {
        throw std::runtime_error("Web view is not initialized");
    }
    platform::loadHTMLFile(nativeHandle_, filePath);
}

void WebView::loadHTMLString(const std::string& html) {
    if (!nativeHandle_) {
        throw std::runtime_error("Web view is not initialized");
    }
    platform::loadHTMLString(nativeHandle_, html);
}

void WebView::loadURL(const std::string& url) {
    if (!nativeHandle_) {
        throw std::runtime_error("Web view is not initialized");
    }
    // URLs like "localhost:5173" lack a scheme; WebViews misparse them. Prepend http:// when absent.
    std::string urlToLoad = url;
    if (!url.empty() && url.find("://") == std::string::npos) {
        urlToLoad = "http://" + url;
    }
    platform::loadURL(nativeHandle_, urlToLoad);
}

void WebView::OnParentChanged(Control* oldParent, Control* newParent) {
    Control::OnParentChanged(oldParent, newParent);
    // Recreate native web view with new parent
    if (nativeHandle_) {
        destroyNativeWebView();
    }
    if (newParent && newParent->getNativeHandle()) {
        createNativeWebView();
        // Re-register callbacks if they were set
        if (createWindowCallback_) {
            setCreateWindowCallback(createWindowCallback_);
        }
        if (messageCallback_) {
            setMessageCallback(messageCallback_);
        }
    }
}

void WebView::OnBoundsChanged() {
    Control::OnBoundsChanged();
    updateNativeWebViewBounds();
}

void WebView::updateNativeWebViewBounds() {
    if (nativeHandle_) {
        // Use platform resize function
        platform::resizeWebView(nativeHandle_, GetWidth(), GetHeight());
    }
}

void WebView::setCreateWindowCallback(std::function<void(const std::string& title)> callback) {
    if (!nativeHandle_) {
        return;
    }
    createWindowCallback_ = callback;
    // Pass this WebView instance as userData so we can access the callback
    platform::setWebViewCreateWindowCallback(nativeHandle_, 
        createWindowCallbackWrapper, this);
}

void WebView::setMessageCallback(std::function<void(const std::string& jsonMessage)> callback) {
    if (!nativeHandle_) {
        return;
    }
    messageCallback_ = callback;
    // Pass this WebView instance as userData so we can access the callback
    platform::setWebViewMessageCallback(nativeHandle_, 
        messageCallbackWrapper, this);
}

void WebView::postMessageToJavaScript(const std::string& jsonMessage) {
    if (!nativeHandle_) {
        return;
    }
    platform::postMessageToJavaScript(nativeHandle_, jsonMessage);
}

void WebView::createWindowCallbackWrapper(const std::string& title, void* userData) {
    WebView* webview = static_cast<WebView*>(userData);
    if (webview && webview->createWindowCallback_) {
        webview->createWindowCallback_(title);
    }
}

void WebView::messageCallbackWrapper(const std::string& jsonMessage, void* userData) {
    WebView* webview = static_cast<WebView*>(userData);
    if (webview && webview->messageCallback_) {
        webview->messageCallback_(jsonMessage);
    }
}
