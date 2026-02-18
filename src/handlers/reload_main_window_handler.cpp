#include "../../include/message_handler.h"
#include "../../include/webview_window.h"
#include "../../include/config_manager.h"
#include "../platform/platform_impl.h"
#include <nlohmann/json.hpp>
#include <iostream>

// Dedicated handler for reloading ONLY the main window
// Passed specifically to settings/child windows so they can reload the main window
class ReloadMainWindowHandler : public MessageHandler {
public:
    explicit ReloadMainWindowHandler(WebViewWindow* mainWindow) : mainWindow_(mainWindow) {}

    bool canHandle(const std::string& messageType) const override {
        return messageType == "reloadMainWindow";
    }

    nlohmann::json handle(const nlohmann::json& payload, const std::string& requestId) override {
        (void)payload;
        (void)requestId;
        nlohmann::json result;

        std::cout << "[ReloadMainWindow] ===== HANDLER CALLED =====" << std::endl;

        if (!mainWindow_) {
            result["success"] = false;
            result["error"] = "Main window pointer is NULL";
            std::cerr << "[ReloadMainWindow] ERROR: " << result["error"].get<std::string>() << std::endl;
            return result;
        }

        if (!mainWindow_->getWebView()) {
            result["success"] = false;
            result["error"] = "Main window WebView is NULL";
            std::cerr << "[ReloadMainWindow] ERROR: " << result["error"].get<std::string>() << std::endl;
            return result;
        }

        if (!mainWindow_->getWebView()->getNativeHandle()) {
            result["success"] = false;
            result["error"] = "Main window WebView native handle is NULL";
            std::cerr << "[ReloadMainWindow] ERROR: " << result["error"].get<std::string>() << std::endl;
            return result;
        }

        std::cout << "[ReloadMainWindow] Main window pointer is valid ✓" << std::endl;
        
        ConfigManager& config = ConfigManager::getInstance();
        
        std::cout << "[ReloadMainWindow] Loading configuration..." << std::endl;
        if (!config.loadOptions()) {
            result["success"] = false;
            result["error"] = "Failed to reload options";
            std::cerr << "[ReloadMainWindow] ERROR: " << result["error"].get<std::string>() << std::endl;
            return result;
        }
        std::cout << "[ReloadMainWindow] Configuration loaded ✓" << std::endl;
        
        // DEBUG: Log what we actually loaded from ConfigManager
        std::cout << "[ReloadMainWindow] DEBUG - ConfigManager data after load:" << std::endl;
        std::string htmlMethod = config.getHtmlLoadingMethod();
        std::string htmlFilePath = config.getHtmlFilePath();
        std::string htmlUrl = config.getHtmlUrl();
        std::string htmlContent = config.getHtmlContent();
        std::cout << "  Method: " << htmlMethod << std::endl;
        std::cout << "  FilePath: " << htmlFilePath << std::endl;
        std::cout << "  URL: " << htmlUrl << std::endl;
        std::cout << "  HtmlContent size: " << htmlContent.length() << " bytes" << std::endl;

        std::string preload = ConfigManager::getPreloadScriptContent();
        std::cout << "[ReloadMainWindow] Setting preload script (size: " << preload.length() << ")" << std::endl;
        platform::setWebViewPreloadScript(mainWindow_->getWebView()->getNativeHandle(), preload);

        std::cout << "[ReloadMainWindow] HTML loading method: '" << htmlMethod << "'" << std::endl;
        
        if (htmlMethod == "html") {
            std::string content = config.getHtmlContent();
            if (content.empty()) {
                std::cout << "[ReloadMainWindow] HTML content empty, trying to load demo.html..." << std::endl;
                content = ConfigManager::tryLoadFileContent("demo.html");
            }
            if (content.empty()) {
                content = "<html><body><h1>Error: No HTML content</h1></body></html>";
            }
            std::cout << "[ReloadMainWindow] Loading HTML string (" << content.length() << " bytes)" << std::endl;
            mainWindow_->loadHTMLString(content);
        } else if (htmlMethod == "url") {
            std::string url = config.getHtmlUrl();
            std::cout << "[ReloadMainWindow] URL loading, URL: '" << url << "'" << std::endl;
            if (url.empty()) {
                mainWindow_->loadHTMLString("<html><body><h1>Error: No URL specified</h1></body></html>");
            } else {
                std::cout << "[ReloadMainWindow] Calling loadURL('" << url << "')" << std::endl;
                mainWindow_->loadURL(url);
            }
        } else if (htmlMethod == "file") {
            std::string filePath = config.getHtmlFilePath();
            std::cout << "[ReloadMainWindow] FILE loading, raw filePath: '" << filePath << "'" << std::endl;
            
            if (filePath.empty()) {
                std::cout << "[ReloadMainWindow] File path empty, using default 'demo.html'" << std::endl;
                filePath = "demo.html";
            }
            
            std::string absolutePath = ConfigManager::resolveFilePathToAbsolute(filePath);
            std::cout << "[ReloadMainWindow] Resolved path: '" << absolutePath << "'" << std::endl;
            
            if (!absolutePath.empty()) {
                std::cout << "[ReloadMainWindow] Calling loadHTMLFile('" << absolutePath << "')" << std::endl;
                mainWindow_->loadHTMLFile(absolutePath);
                std::cout << "[ReloadMainWindow] loadHTMLFile() returned" << std::endl;
            } else {
                std::cout << "[ReloadMainWindow] ERROR: File resolution failed for '" << filePath << "'" << std::endl;
                mainWindow_->loadHTMLString("<html><body><h1>File not found</h1><p>" + filePath + "</p></body></html>");
            }
        } else {
            std::cout << "[ReloadMainWindow] UNKNOWN method '" << htmlMethod << "', loading default content" << std::endl;
            std::string content = ConfigManager::tryLoadFileContent("demo.html");
            if (content.empty()) {
                content = "<html><body><h1>Error</h1><p>No content configured</p></body></html>";
            }
            mainWindow_->loadHTMLString(content);
        }

        result["success"] = true;
        std::cout << "[ReloadMainWindow] ===== HANDLER COMPLETE =====" << std::endl;
        return result;
    }

    std::vector<std::string> getSupportedTypes() const override {
        return {"reloadMainWindow"};
    }

private:
    WebViewWindow* mainWindow_;
};

std::shared_ptr<MessageHandler> createReloadMainWindowHandler(WebViewWindow* mainWindow) {
    return std::make_shared<ReloadMainWindowHandler>(mainWindow);
}
