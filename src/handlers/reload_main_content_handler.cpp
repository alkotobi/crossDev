#include "../../include/message_handler.h"
#include "../../include/webview_window.h"
#include "../../include/config_manager.h"
#include "../../include/singleton_webview_window_manager.h"
#include "../platform/platform_impl.h"
#include <nlohmann/json.hpp>
#include <iostream>

class ReloadMainContentHandler : public MessageHandler {
public:
    explicit ReloadMainContentHandler(WebViewWindow* mainWindow) : mainWindow_(mainWindow) {}

    bool canHandle(const std::string& messageType) const override {
        return messageType == "reloadMainContent";
    }

    nlohmann::json handle(const nlohmann::json& payload, const std::string& requestId) override {
        (void)payload;
        (void)requestId;
        nlohmann::json result;

        std::cout << "[ReloadMainContent] Handler called" << std::endl;

        // Try to get main window from the singleton manager first (most reliable)
        WebViewWindow* mainWindow = SingletonWebViewWindowManager::getInstance().getWindow(
            SingletonWebViewWindowManager::MAIN_WINDOW_NAME);
        
        // Fall back to stored pointer if not found in manager
        if (!mainWindow) {
            mainWindow = mainWindow_;
        }

        if (!mainWindow || !mainWindow->getWebView() || !mainWindow->getWebView()->getNativeHandle()) {
            result["success"] = false;
            result["error"] = "Main window not available";
            std::cerr << "[ReloadMainContent] ERROR: " << result["error"].get<std::string>() << std::endl;
            return result;
        }

        std::cout << "[ReloadMainContent] Main window is valid, loading configuration..." << std::endl;
        ConfigManager& config = ConfigManager::getInstance();
        if (!config.loadOptions()) {
            result["success"] = false;
            result["error"] = "Failed to reload options";
            std::cerr << "[ReloadMainContent] ERROR: " << result["error"].get<std::string>() << std::endl;
            return result;
        }

        std::string preload = ConfigManager::getPreloadScriptContent();
        platform::setWebViewPreloadScript(mainWindow->getWebView()->getNativeHandle(), preload);

        std::string method = config.getHtmlLoadingMethod();
        std::cout << "[ReloadMainContent] HTML loading method: " << method << std::endl;
        
        if (method == "html") {
            std::string content = config.getHtmlContent();
            if (content.empty()) {
                content = ConfigManager::tryLoadFileContent("demo.html");
            }
            if (content.empty()) {
                content = "<html><body><h1>Error: No HTML content</h1><p>Set htmlContent in options.json</p></body></html>";
            }
            std::cout << "[ReloadMainContent] Loading HTML string, size: " << content.length() << " bytes" << std::endl;
            mainWindow->loadHTMLString(content);
        } else if (method == "url") {
            std::string url = config.getHtmlUrl();
            if (url.empty()) {
                mainWindow->loadHTMLString("<html><body><h1>Error: No URL specified</h1><p>Set url in options.json</p></body></html>");
            } else {
                std::cout << "[ReloadMainContent] Loading URL: " << url << std::endl;
                mainWindow->loadURL(url);
            }
        } else if (method == "file") {
            std::string filePath = config.getHtmlFilePath();
            if (filePath.empty()) {
                filePath = "demo.html";
            }
            std::string absolutePath = ConfigManager::resolveFilePathToAbsolute(filePath);
            if (!absolutePath.empty()) {
                std::cout << "[ReloadMainContent] Loading HTML file: " << absolutePath << std::endl;
                mainWindow->loadHTMLFile(absolutePath);
            } else {
                std::cout << "[ReloadMainContent] ERROR: File not found: " << filePath << std::endl;
                mainWindow->loadHTMLString("<html><body><h1>File not found</h1><p>Check filePath in options.json: " + filePath + "</p></body></html>");
            }
        } else {
            std::string content = ConfigManager::tryLoadFileContent("demo.html");
            if (content.empty()) {
                content = "<html><body><h1>Error</h1><p>No content configured</p></body></html>";
            }
            std::cout << "[ReloadMainContent] Loading default content" << std::endl;
            mainWindow->loadHTMLString(content);
        }

        result["success"] = true;
        std::cout << "[ReloadMainContent] Successfully reloaded main content" << std::endl;
        return result;
    }

    std::vector<std::string> getSupportedTypes() const override {
        return {"reloadMainContent"};
    }

private:
    WebViewWindow* mainWindow_;
};

std::shared_ptr<MessageHandler> createReloadMainContentHandler(WebViewWindow* mainWindow) {
    return std::make_shared<ReloadMainContentHandler>(mainWindow);
}
