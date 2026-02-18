#include "../../include/handlers/focus_window_handler.h"
#include "../../include/singleton_webview_window_manager.h"
#include <nlohmann/json.hpp>

class FocusWindowHandler : public MessageHandler {
public:
    bool canHandle(const std::string& messageType) const override {
        return messageType == "focusWindow";
    }

    nlohmann::json handle(const nlohmann::json& payload, const std::string& requestId) override {
        (void)requestId;
        std::string name;
        if (payload.contains("name") && payload["name"].is_string()) {
            name = payload["name"].get<std::string>();
        } else if (payload.contains("windowName") && payload["windowName"].is_string()) {
            name = payload["windowName"].get<std::string>();
        }
        if (name.empty()) {
            return {{"success", false}, {"error", "name or windowName is required"}};
        }
        if (SingletonWebViewWindowManager::getInstance().focusWindow(name)) {
            return {{"success", true}};
        }
        return {{"success", false}, {"error", "window not found: " + name}};
    }

    std::vector<std::string> getSupportedTypes() const override {
        return {"focusWindow"};
    }
};

std::shared_ptr<MessageHandler> createFocusWindowHandler() {
    static std::shared_ptr<MessageHandler> instance = std::make_shared<FocusWindowHandler>();
    return instance;
}
