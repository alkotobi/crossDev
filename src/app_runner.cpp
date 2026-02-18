#include "../include/app_runner.h"
#include "../include/application.h"
#include "../include/event_handler.h"
#include "../include/message_router.h"
#include "../include/config_manager.h"
#include "../include/platform.h"
#include "../include/webview_window.h"
#include "../include/singleton_webview_window_manager.h"
#include "../include/handlers/create_window_handler.h"
#include "../include/handlers/app_info_handler.h"
#include "../include/handlers/calculator_handler.h"
#include "../include/handlers/file_dialog_handler.h"
#include "../include/handlers/read_file_handler.h"
#include "../include/handlers/write_file_handler.h"
#include "../include/handlers/file_system_handler.h"
#include "../include/handlers/context_menu_handler.h"
#include "../include/handlers/focus_window_handler.h"
#include "../include/handlers/options_handler.h"
#include "../include/handlers/reload_main_content_handler.h"
#include "../include/handlers/reload_main_window_handler.h"
#include "../include/app_handlers.h"
#include "platform/platform_impl.h"
#include <iostream>

AppRunner::AppRunner(int argc, const char* argv[])
    : argc_(argc), argv_(argv) {
}

AppRunner::~AppRunner() {
    eventHandler_.reset();
    mainWindow_.reset();
}

void AppRunner::loadConfig() {
    ConfigManager& config = ConfigManager::getInstance();
    if (!config.loadOptions()) {
        std::cerr << "Warning: Failed to load options, using defaults" << std::endl;
    }

    loadingMethod_ = config.getHtmlLoadingMethod();
    contentType_ = WebViewContentType::Default;
    content_.clear();

    if (loadingMethod_ == "html") {
        contentType_ = WebViewContentType::Html;
        content_ = config.getHtmlContent();
        if (content_.empty()) {
            content_ = ConfigManager::tryLoadFileContent("demo.html");
        }
        if (content_.empty()) {
            content_ = "<html><body><h1>Error: No HTML content</h1><p>Set htmlContent in options.json</p></body></html>";
        }
    } else if (loadingMethod_ == "url") {
        content_ = config.getHtmlUrl();
        if (content_.empty()) {
            contentType_ = WebViewContentType::Html;
            content_ = "<html><body><h1>Error: No URL specified</h1><p>Set url in options.json</p></body></html>";
        } else {
            contentType_ = WebViewContentType::Url;
        }
    } else if (loadingMethod_ == "file") {
        content_ = config.getHtmlFilePath();
        if (content_.empty()) {
            content_ = ConfigManager::tryLoadFileContent("demo.html");
        }
        if (content_.empty()) {
            contentType_ = WebViewContentType::Html;
            content_ = "<html><body><h1>Error: Could not load file</h1><p>Check filePath in options.json</p></body></html>";
        } else {
            contentType_ = WebViewContentType::File;
        }
    } else {
        content_ = ConfigManager::tryLoadFileContent("demo.html");
        if (content_.empty()) {
            content_ = "<html><body><h1>Error</h1><p>No content configured</p></body></html>";
        }
        contentType_ = WebViewContentType::Html;
    }
}

void AppRunner::createMainWindow() {
    mainWindow_ = std::make_shared<WebViewWindow>(
        nullptr, 100, 100, 800, 600,
        "Cars", contentType_, content_);
}

void AppRunner::setupEventHandler() {
    eventHandler_ = std::make_unique<EventHandler>(
        mainWindow_->getWindow(), mainWindow_->getWebView());

    eventHandler_->onWebViewCreateWindow(
        [this](const std::string& name, const std::string& title, WebViewContentType type, const std::string& cnt, bool isSingleton, int x, int y, int width, int height) {
            try {
                std::string content = cnt;
                WebViewContentType contentType = type;
                if (content.empty()) {
                    content = "<html><body><h1>" + title + "</h1><p>No content specified.</p></body></html>";
                    contentType = WebViewContentType::Html;
                }
                if (x < 0) x = 150;
                if (y < 0) y = 150;
                if (width <= 0) width = 900;
                if (height <= 0) height = 700;
                auto attachFn = [this, name](WebView* wv) {
                    if (eventHandler_ && wv && mainWindow_) {
                        std::vector<std::shared_ptr<MessageHandler>> extras;
                        extras.push_back(createFocusWindowHandler());
                        extras.push_back(createOptionsHandler());
                        extras.push_back(createFileDialogHandler(mainWindow_->getWindow()));
                        // For settings window, add reloadMainWindow to explicitly reload main window
                        if (name == "settings") {
                            std::cout << "[AppRunner] Attaching reloadMainWindowHandler to settings window ✓" << std::endl;
                            extras.push_back(createReloadMainWindowHandler(mainWindow_.get()));
                        } else {
                            std::cout << "[AppRunner] Attaching reloadMainContentHandler to window: " << name << std::endl;
                            extras.push_back(createReloadMainContentHandler(mainWindow_.get()));
                        }
                        eventHandler_->attachWebView(wv, extras);
                    }
                };
                WebViewWindow* child = nullptr;
                if (isSingleton) {
                    child = SingletonWebViewWindowManager::getInstance().getOrCreate(
                        name, title, contentType, content, mainWindow_.get(), attachFn, x, y, width, height);
                }
                if (!child) {
                    auto childPtr = std::make_unique<WebViewWindow>(mainWindow_.get(), x, y, width, height, title, contentType, content);
                    child = childPtr.get();
                    if (eventHandler_ && child && mainWindow_) {
                        std::vector<std::shared_ptr<MessageHandler>> extras;
                        extras.push_back(createFocusWindowHandler());
                        extras.push_back(createOptionsHandler());
                        extras.push_back(createFileDialogHandler(mainWindow_->getWindow()));
                        // For settings window, add reloadMainWindow to explicitly reload main window
                        if (name == "settings") {
                            std::cout << "[AppRunner] Attaching reloadMainWindowHandler to settings window (non-singleton) ✓" << std::endl;
                            extras.push_back(createReloadMainWindowHandler(mainWindow_.get()));
                        } else {
                            std::cout << "[AppRunner] Attaching reloadMainContentHandler to window (non-singleton): " << name << std::endl;
                            extras.push_back(createReloadMainContentHandler(mainWindow_.get()));
                        }
                        eventHandler_->attachWebView(child->getWebView(), extras);
                    }
                    child->show();
                    childPtr.release();  // Ownership transferred to Component parent (mainWindow_)
                }
                if (child) {
                    std::cout << "Opened window: " << name << " (" << title << ")" << std::endl;
                }
            } catch (const std::exception& e) {
                std::cerr << "Error creating window: " << e.what() << std::endl;
            }
        });
}

void AppRunner::registerHandlers() {
    MessageRouter* router = eventHandler_->getMessageRouter();

    SingletonWebViewWindowManager::getInstance().registerWindow(SingletonWebViewWindowManager::MAIN_WINDOW_NAME, mainWindow_.get());

    router->registerHandler(createAppInfoHandler());
    router->registerHandler(createCalculatorHandler());
    router->registerHandler(createFileDialogHandler(mainWindow_->getWindow()));
    router->registerHandler(createReadFileHandler());
    router->registerHandler(createWriteFileHandler());
    router->registerHandler(createFileSystemHandler());
    router->registerHandler(createContextMenuHandler(mainWindow_, eventHandler_->getMessageRouterShared()));
    router->registerHandler(createFocusWindowHandler());
    router->registerHandler(createOptionsHandler());
    router->registerHandler(createReloadMainContentHandler(mainWindow_.get()));

    registerAppHandlers(router);
}

int AppRunner::run() {
    std::cout << "Running on " << PLATFORM_NAME << std::endl;
    std::cout << "Config directory: " << ConfigManager::getConfigDirectory() << std::endl;
    std::cout << "Options file: " << ConfigManager::getOptionsFilePath() << std::endl;

    loadConfig();
    createMainWindow();
    setupEventHandler();
    registerHandlers();

    std::cout << "HTML loading method: " << loadingMethod_ << std::endl;

    mainWindow_->show();
    mainWindow_->getWindow()->maximize();

    platform::deliverOpenFilePaths(argc_, argv_);

    std::cout << "Window created with web view." << std::endl;

    Application::getInstance().run();

    return 0;
}
