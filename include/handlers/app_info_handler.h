#ifndef APP_INFO_HANDLER_H
#define APP_INFO_HANDLER_H

#include "../message_handler.h"
#include <memory>

// Factory function to create AppInfoHandler
std::shared_ptr<MessageHandler> createAppInfoHandler();

#endif // APP_INFO_HANDLER_H
