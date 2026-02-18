#ifndef WEBVIEW_H
#define WEBVIEW_H

#include "control.h"
#include <string>
#include <functional>

// Platform-agnostic WebView interface
// WebView inherits from Control, so it supports Owner and Parent
class WebView : public Control {
public:
    // Constructor: WebView(owner, parent, x, y, width, height)
    WebView(Component* owner = nullptr, Control* parent = nullptr,
            int x = 0, int y = 0, int width = 800, int height = 600);
    ~WebView();
    
    // Non-copyable, movable
    WebView(const WebView&) = delete;
    WebView& operator=(const WebView&) = delete;
    WebView(WebView&&) noexcept;
    WebView& operator=(WebView&&) noexcept;
    
    void loadHTMLFile(const std::string& filePath);
    void loadHTMLString(const std::string& html);
    void loadURL(const std::string& url);
    void setCreateWindowCallback(std::function<void(const std::string& title)> callback);
    void setMessageCallback(std::function<void(const std::string& jsonMessage)> callback);
    void postMessageToJavaScript(const std::string& jsonMessage);
    
    // Platform-specific handle (opaque pointer)
    void* getNativeHandle() const override { return nativeHandle_; }
    
protected:
    // Override Control virtual methods
    void OnParentChanged(Control* oldParent, Control* newParent) override;
    void OnBoundsChanged() override;
    
private:
    void* nativeHandle_;
    std::function<void(const std::string& title)> createWindowCallback_;
    std::function<void(const std::string& jsonMessage)> messageCallback_;
    
    // Platform-specific implementation
    void createNativeWebView();
    void destroyNativeWebView();
    void updateNativeWebViewBounds();
    
    // Static callback wrappers
    static void createWindowCallbackWrapper(const std::string& title, void* userData);
    static void messageCallbackWrapper(const std::string& jsonMessage, void* userData);
};

#endif // WEBVIEW_H
