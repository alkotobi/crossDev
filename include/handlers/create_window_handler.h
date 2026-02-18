#ifndef CREATE_WINDOW_HANDLER_H
#define CREATE_WINDOW_HANDLER_H

#include "../message_handler.h"
#include "../webview_window.h"
#include <memory>
#include <functional>
#include <string>

// Factory function to create CreateWindowHandler
// Callback receives (name, title, contentType, content, isSingleton, x, y, width, height).
// All geometry from JS: x, y (position), width, height (size). Use 0 for defaults.
// Payload: className, title, url/html/file, isSingleton, x, y, width, height.
std::shared_ptr<MessageHandler> createCreateWindowHandler(
    std::function<void(const std::string& name, const std::string& title, WebViewContentType contentType, const std::string& content, bool isSingleton, int x, int y, int width, int height)> onCreateWindow);

#endif // CREATE_WINDOW_HANDLER_H
