#ifndef READ_FILE_HANDLER_H
#define READ_FILE_HANDLER_H

#include "../message_handler.h"
#include <memory>

std::shared_ptr<MessageHandler> createReadFileHandler();

#endif // READ_FILE_HANDLER_H
