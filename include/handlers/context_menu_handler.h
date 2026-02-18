#ifndef CONTEXT_MENU_HANDLER_H
#define CONTEXT_MENU_HANDLER_H

#include "../message_handler.h"
#include <memory>

class WebViewWindow;
class MessageRouter;

std::shared_ptr<MessageHandler> createContextMenuHandler(
    std::shared_ptr<WebViewWindow> window,
    std::shared_ptr<MessageRouter> router);

#endif // CONTEXT_MENU_HANDLER_H
