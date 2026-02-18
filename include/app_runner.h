#ifndef APP_RUNNER_H
#define APP_RUNNER_H

#include "webview_window.h"
#include <string>
#include <memory>

class EventHandler;

// Encapsulates application setup: config, main window, event handling, and handler registration.
// Keeps main.cpp minimal and separates bootstrap logic for maintainability.
class AppRunner {
public:
    explicit AppRunner(int argc, const char* argv[]);
    ~AppRunner();

    // Run the application: show window, deliver open files, start event loop.
    int run();

private:
    void loadConfig();
    void createMainWindow();
    void setupEventHandler();
    void registerHandlers();

    int argc_;
    const char** argv_;
    std::string loadingMethod_;
    WebViewContentType contentType_;
    std::string content_;

    std::unique_ptr<EventHandler> eventHandler_;
    std::shared_ptr<WebViewWindow> mainWindow_;
};

#endif // APP_RUNNER_H
