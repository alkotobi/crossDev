#include "../include/native_event_bus.h"
#include "../include/webview.h"
#include "platform/platform_impl.h"
#include <nlohmann/json.hpp>
#include <algorithm>

NativeEventBus& NativeEventBus::getInstance() {
    static NativeEventBus instance;
    return instance;
}

void NativeEventBus::subscribe(WebView* webView) {
    if (!webView) return;
    std::lock_guard<std::mutex> lock(mutex_);
    if (std::find(subscribers_.begin(), subscribers_.end(), webView) == subscribers_.end()) {
        subscribers_.push_back(webView);
    }
}

void NativeEventBus::unsubscribe(WebView* webView) {
    if (!webView) return;
    std::lock_guard<std::mutex> lock(mutex_);
    subscribers_.erase(std::remove(subscribers_.begin(), subscribers_.end(), webView), subscribers_.end());
}

void NativeEventBus::emitTo(WebView* webView, const std::string& eventName, const std::string& payloadJson) {
    if (!webView) return;
    nlohmann::json msg;
    msg["type"] = "crossdev:event";
    msg["name"] = eventName;
    try {
        msg["payload"] = payloadJson.empty() ? nlohmann::json::object() : nlohmann::json::parse(payloadJson);
    } catch (...) {
        msg["payload"] = nlohmann::json::object();
    }
    webView->postMessageToJavaScript(msg.dump());
}

void NativeEventBus::emitToAll(const std::string& eventName, const std::string& payloadJson) {
    std::vector<WebView*> copy;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        copy = subscribers_;
    }
    for (WebView* wv : copy) {
        emitTo(wv, eventName, payloadJson);
    }
}
