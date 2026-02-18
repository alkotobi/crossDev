#ifndef RELOAD_MAIN_CONTENT_HANDLER_H
#define RELOAD_MAIN_CONTENT_HANDLER_H

#include "../message_handler.h"
#include <memory>

class WebViewWindow;

std::shared_ptr<MessageHandler> createReloadMainContentHandler(WebViewWindow* mainWindow);

#endif
