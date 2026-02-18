#ifndef RELOAD_MAIN_WINDOW_HANDLER_H
#define RELOAD_MAIN_WINDOW_HANDLER_H

#include "../message_handler.h"
#include <memory>

class WebViewWindow;

// Dedicated handler that reloads the MAIN window (not the calling window)
// Used by settings/child windows to explicitly reload main window content
std::shared_ptr<MessageHandler> createReloadMainWindowHandler(WebViewWindow* mainWindow);

#endif // RELOAD_MAIN_WINDOW_HANDLER_H
