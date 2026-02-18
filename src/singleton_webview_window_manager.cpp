#include "../include/singleton_webview_window_manager.h"
#include <algorithm>
#include <cctype>
#include <iostream>
#include <memory>
#ifdef _WIN32
#include <windows.h>
#endif

SingletonWebViewWindowManager& SingletonWebViewWindowManager::getInstance() {
    static SingletonWebViewWindowManager instance;
    return instance;
}

std::string SingletonWebViewWindowManager::toLower(const std::string& s) {
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return out;
}

WebViewWindow* SingletonWebViewWindowManager::getOrCreate(
    const std::string& name,
    const std::string& title,
    WebViewContentType contentType,
    const std::string& content,
    Component* parent,
    std::function<void(WebView*)> attachFn,
    int x, int y, int width, int height) {
    if (name.empty()) {
        return nullptr;
    }
#ifdef COMPONENT_DEBUG_LIFECYCLE
    std::cout << "[SingletonWebViewWindowManager] getOrCreate name=" << name << " title=" << title << std::endl;
#ifdef _WIN32
    OutputDebugStringA(("[SingletonWebViewWindowManager] getOrCreate name=" + name + " title=" + title + "\n").c_str());
#endif
#endif
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = windows_.find(toLower(name));
    if (it != windows_.end()) {
        WebViewWindow* existing = it->second;
        if (existing && existing->getWindow()) {
            existing->show();
#ifdef COMPONENT_DEBUG_LIFECYCLE
            std::cout << "Singleton window '" << name << "' shown (existing)" << std::endl;
#endif
            return existing;
        }
        windows_.erase(it);
    }
    auto childPtr = std::make_unique<WebViewWindow>(parent, x, y, width, height, title, contentType, content);
    WebViewWindow* child = childPtr.get();
    if (attachFn && child->getWebView()) {
        attachFn(child->getWebView());
    }
    std::string nameCopy = name;
    child->setOnDestroyCallback([this, nameCopy]() {
        unregister(nameCopy);
    });
    windows_[toLower(name)] = child;
    child->show();
    childPtr.release();  // Ownership transferred to Component parent
#ifdef COMPONENT_DEBUG_LIFECYCLE
    std::cout << "Singleton window '" << name << "' created" << std::endl;
#endif
    return child;
}

void SingletonWebViewWindowManager::unregister(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = windows_.find(toLower(name));
    if (it != windows_.end()) {
        windows_.erase(it);
#ifdef COMPONENT_DEBUG_LIFECYCLE
        std::cout << "Singleton window '" << name << "' unregistered" << std::endl;
#endif
    }
}

void SingletonWebViewWindowManager::registerWindow(const std::string& name, WebViewWindow* window) {
    if (name.empty() || !window) return;
    std::lock_guard<std::mutex> lock(mutex_);
    windows_[toLower(name)] = window;
#ifdef COMPONENT_DEBUG_LIFECYCLE
    std::cout << "Window '" << name << "' registered" << std::endl;
#endif
}

void SingletonWebViewWindowManager::registerFocusCallback(const std::string& name, std::function<void()> onFocus) {
    if (name.empty() || !onFocus) return;
    std::lock_guard<std::mutex> lock(mutex_);
    focusCallbacks_[toLower(name)] = std::move(onFocus);
#ifdef COMPONENT_DEBUG_LIFECYCLE
    std::cout << "Focus callback '" << name << "' registered" << std::endl;
#endif
}

bool SingletonWebViewWindowManager::focusWindow(const std::string& name) const {
    if (name.empty()) return false;
    WebViewWindow* windowToShow = nullptr;
    std::function<void()> callbackToInvoke;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        std::string key = toLower(name);
        auto it = windows_.find(key);
        if (it != windows_.end() && it->second) {
            windowToShow = it->second;
        } else {
            auto cbIt = focusCallbacks_.find(key);
            if (cbIt != focusCallbacks_.end() && cbIt->second) {
                callbackToInvoke = cbIt->second;
            }
        }
    }
    if (windowToShow) {
        windowToShow->show();
        return true;
    }
    if (callbackToInvoke) {
        callbackToInvoke();
        return true;
    }
    return false;
}

WebViewWindow* SingletonWebViewWindowManager::getWindow(const std::string& name) const {
    if (name.empty()) return nullptr;
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = windows_.find(toLower(name));
    return (it != windows_.end()) ? it->second : nullptr;
}
