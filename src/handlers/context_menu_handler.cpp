#include "../../include/handlers/context_menu_handler.h"
#include "../../include/webview_window.h"
#include "../../include/message_router.h"
#include "../../include/native_event_bus.h"
#include "../platform/platform_impl.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <memory>

// Default context menu items when none provided
static const char* DEFAULT_CONTEXT_ITEMS = R"([
    {"id":"copy","label":"Copy"},
    {"id":"paste","label":"Paste"},
    {"id":"selectAll","label":"Select All"},
    {"id":"-"},
    {"id":"reload","label":"Reload"},
    {"id":"-"},
    {"id":"inspect","label":"Inspect"}
])";

class ContextMenuHandler : public MessageHandler {
public:
    ContextMenuHandler(std::shared_ptr<WebViewWindow> window, std::shared_ptr<MessageRouter> router)
        : window_(std::move(window)), router_(std::move(router)) {}

    bool canHandle(const std::string& messageType) const override {
        return messageType == "showContextMenu";
    }

    nlohmann::json handle(const nlohmann::json& payload, const std::string& requestId) override {
        if (!window_ || !window_->getWindow() || !router_) {
            return {{"success", false}, {"error", "Window not available"}};
        }

        std::shared_ptr<WebViewWindow> window = window_;
        std::shared_ptr<MessageRouter> router = router_;

        int x = 0, y = 0;
        if (payload.contains("x") && payload["x"].is_number()) {
            x = payload["x"].get<int>();
        }
        if (payload.contains("y") && payload["y"].is_number()) {
            y = payload["y"].get<int>();
        }

        std::string itemsJson = DEFAULT_CONTEXT_ITEMS;
        if (payload.contains("items") && payload["items"].is_array()) {
            itemsJson = payload["items"].dump();
        }

        struct CallbackData {
            std::string requestId;
            std::shared_ptr<MessageRouter> router;
            std::shared_ptr<WebViewWindow> window;
            int x, y;
        };
        auto* cbData = new CallbackData{
            requestId, router, window, x, y
        };

        platform::showContextMenu(
            window_->getWindow()->getNativeHandle(),
            x, y, itemsJson,
            [](const std::string& itemId, void* userData) {
                std::unique_ptr<CallbackData> data(static_cast<CallbackData*>(userData));
                if (!data) return;
                if (data->window && data->window->getWebView()) {
                    nlohmann::json payload;
                    payload["id"] = itemId;
                    payload["x"] = data->x;
                    payload["y"] = data->y;
                    NativeEventBus::getInstance().emitTo(data->window->getWebView(), "menu:context", payload.dump());
                }
                if (data->router) {
                    nlohmann::json result;
                    result["itemId"] = itemId;
                    result["success"] = true;
                    data->router->sendResponse(data->requestId, result.dump(), "");
                }
            },
            cbData
        );

        return {};  // Async - response sent from callback
    }

    std::vector<std::string> getSupportedTypes() const override {
        return {"showContextMenu"};
    }

private:
    std::shared_ptr<WebViewWindow> window_;
    std::shared_ptr<MessageRouter> router_;
};

std::shared_ptr<MessageHandler> createContextMenuHandler(
    std::shared_ptr<WebViewWindow> window,
    std::shared_ptr<MessageRouter> router) {
    return std::make_shared<ContextMenuHandler>(std::move(window), std::move(router));
}
