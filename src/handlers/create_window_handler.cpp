#include "../../include/handlers/create_window_handler.h"
#include "../../include/message_handler.h"
#include "../../include/window.h"
#include "settings_embed.h"
#include <nlohmann/json.hpp>
#include <cctype>
#include <iostream>
#include <vector>
#include <functional>
#include <map>
#ifdef _WIN32
#include <windows.h>
#endif


// Handler for creating new windows from JavaScript.
// Name is derived from className + incremental int (e.g. car-stock-1, car-stock-2).
// For singleton, name = className (one per class).
namespace {
    std::map<std::string, int> g_classNameCounters;
}

class CreateWindowHandler : public MessageHandler {
public:
    CreateWindowHandler(std::function<void(const std::string& name, const std::string& title, WebViewContentType contentType, const std::string& content, bool isSingleton, int x, int y, int width, int height)> onCreateWindow)
        : onCreateWindow_(onCreateWindow) {}
    
    bool canHandle(const std::string& messageType) const override {
        return messageType == "createWindow";
    }
    
    nlohmann::json handle(const nlohmann::json& payload, const std::string& requestId) override {
#ifdef COMPONENT_DEBUG_LIFECYCLE
        std::cout << "[CreateWindowHandler] handle() requestId=" << requestId << " payload=" << payload.dump() << std::endl;
#ifdef _WIN32
        OutputDebugStringA(("[CreateWindowHandler] handle() requestId=" + requestId + "\n").c_str());
#endif
#endif
        std::string className;
        if (payload.contains("className") && payload["className"].is_string()) {
            className = payload["className"].get<std::string>();
        }
        if (className.empty()) {
            // Derive from title for backward compatibility (e.g. demo.html sends only title)
            if (payload.contains("title") && payload["title"].is_string()) {
                std::string t = payload["title"].get<std::string>();
                for (char& c : t) {
                    if (c == ' ' || c == '\t') c = '-';
                    else if (!std::isalnum(static_cast<unsigned char>(c)) && c != '-' && c != '_') c = '-';
                }
                if (!t.empty()) className = "window-" + t;
            }
        }
        if (className.empty()) {
            return {{"success", false}, {"error", "className or title is required"}};
        }
        bool isSingleton = false;
        if (payload.contains("isSingleton") && payload["isSingleton"].is_boolean()) {
            isSingleton = payload["isSingleton"].get<bool>();
        }
        std::string name;
        if (isSingleton) {
            name = className;
        } else {
            int& nextId = g_classNameCounters[className];
            nextId++;
            name = className + "-" + std::to_string(nextId);
        }
        std::string title = "New Window";
        if (payload.contains("title") && payload["title"].is_string()) {
            title = payload["title"].get<std::string>();
        }
        
        // Determine content type and content. Priority: url > html > file > default
        WebViewContentType contentType = WebViewContentType::Default;
        std::string content;
        
        if (payload.contains("url") && payload["url"].is_string()) {
            content = payload["url"].get<std::string>();
            if (!content.empty()) {
                contentType = WebViewContentType::Url;
            }
        }
        if (contentType == WebViewContentType::Default && payload.contains("html") && payload["html"].is_string()) {
            content = payload["html"].get<std::string>();
            if (!content.empty()) {
                contentType = WebViewContentType::Html;
            }
        }
        if (contentType == WebViewContentType::Default && payload.contains("file") && payload["file"].is_string()) {
            content = payload["file"].get<std::string>();
            if (!content.empty()) {
                contentType = WebViewContentType::File;
            }
        }
        if (contentType == WebViewContentType::Default && payload.contains("filePath") && payload["filePath"].is_string()) {
            content = payload["filePath"].get<std::string>();
            if (!content.empty()) {
                contentType = WebViewContentType::File;
            }
        }
        // Settings window: use embedded HTML (local, not deployed to remote server)
        if (contentType == WebViewContentType::Default && className == "settings") {
            content = getEmbeddedSettingsHtml();
            if (!content.empty()) {
                contentType = WebViewContentType::Html;
            }
        }
        
        int x = 150, y = 150, width = 900, height = 700;
        if (payload.contains("x") && payload["x"].is_number()) {
            int v = payload["x"].get<int>();
            if (v >= 0) x = v;
        }
        if (payload.contains("y") && payload["y"].is_number()) {
            int v = payload["y"].get<int>();
            if (v >= 0) y = v;
        }
        if (payload.contains("width") && payload["width"].is_number()) {
            int v = payload["width"].get<int>();
            if (v > 0) width = v;
        }
        if (payload.contains("height") && payload["height"].is_number()) {
            int v = payload["height"].get<int>();
            if (v > 0) height = v;
        }
        
        if (onCreateWindow_) {
            try {
#ifdef COMPONENT_DEBUG_LIFECYCLE
                std::cout << "[CreateWindowHandler] Calling callback name=" << name << " title=" << title << " isSingleton=" << isSingleton << std::endl;
#ifdef _WIN32
                OutputDebugStringA(("[CreateWindowHandler] Calling callback name=" + name + " title=" + title + "\n").c_str());
#endif
#endif
                onCreateWindow_(name, title, contentType, content, isSingleton, x, y, width, height);
#ifdef COMPONENT_DEBUG_LIFECYCLE
                std::cout << "[CreateWindowHandler] Callback completed, returning success" << std::endl;
#ifdef _WIN32
                OutputDebugStringA("[CreateWindowHandler] Callback completed, returning success\n");
#endif
#endif
                nlohmann::json result;
                result["success"] = true;
                result["className"] = className;
                result["name"] = name;
                result["title"] = title;
                result["isSingleton"] = isSingleton;
                return result;
            } catch (const std::exception& e) {
                std::cerr << "[CreateWindowHandler] Exception: " << e.what() << std::endl;
#ifdef _WIN32
                OutputDebugStringA(("[CreateWindowHandler] Exception: " + std::string(e.what()) + "\n").c_str());
#endif
                nlohmann::json result;
                result["success"] = false;
                result["error"] = e.what();
                return result;
            }
        }
        
        nlohmann::json result;
        result["success"] = false;
        result["error"] = "No create window callback registered";
        return result;
    }
    
    std::vector<std::string> getSupportedTypes() const override {
        return {"createWindow"};
    }
    
private:
    std::function<void(const std::string& name, const std::string& title, WebViewContentType contentType, const std::string& content, bool isSingleton, int x, int y, int width, int height)> onCreateWindow_;
};

std::shared_ptr<MessageHandler> createCreateWindowHandler(
    std::function<void(const std::string& name, const std::string& title, WebViewContentType contentType, const std::string& content, bool isSingleton, int x, int y, int width, int height)> onCreateWindow) {
    return std::make_shared<CreateWindowHandler>(onCreateWindow);
}
