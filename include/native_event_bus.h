#ifndef NATIVE_EVENT_BUS_H
#define NATIVE_EVENT_BUS_H

#include <string>
#include <functional>
#include <vector>
#include <mutex>

class WebView;

// Global event bus for native → web events (window focus, app activate, etc.)
class NativeEventBus {
public:
    static NativeEventBus& getInstance();
    
    // Subscribe a WebView to receive events (called when WebViewWindow is created)
    void subscribe(WebView* webView);
    void unsubscribe(WebView* webView);
    
    // Emit event to a specific WebView (e.g. window focus for that window)
    void emitTo(WebView* webView, const std::string& eventName, const std::string& payloadJson);
    
    // Emit event to all subscribed WebViews (e.g. app activate)
    void emitToAll(const std::string& eventName, const std::string& payloadJson);
    
private:
    NativeEventBus() = default;
    ~NativeEventBus() = default;
    NativeEventBus(const NativeEventBus&) = delete;
    NativeEventBus& operator=(const NativeEventBus&) = delete;
    
    std::vector<WebView*> subscribers_;
    std::mutex mutex_;
};

#endif // NATIVE_EVENT_BUS_H
