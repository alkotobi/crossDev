#ifndef OPTIONS_HANDLER_H
#define OPTIONS_HANDLER_H

#include "../message_handler.h"
#include <memory>

// Handler for getOptionsPath, readOptions, writeOptions (options.json)
std::shared_ptr<MessageHandler> createOptionsHandler();

#endif // OPTIONS_HANDLER_H
