#ifndef WRITE_FILE_HANDLER_H
#define WRITE_FILE_HANDLER_H

#include "../message_handler.h"
#include <memory>

std::shared_ptr<MessageHandler> createWriteFileHandler();

#endif // WRITE_FILE_HANDLER_H
