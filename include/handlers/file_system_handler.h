#ifndef FILE_SYSTEM_HANDLER_H
#define FILE_SYSTEM_HANDLER_H

#include "../message_handler.h"
#include <memory>

std::shared_ptr<MessageHandler> createFileSystemHandler();

#endif // FILE_SYSTEM_HANDLER_H
