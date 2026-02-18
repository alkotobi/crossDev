#ifndef FILE_DIALOG_HANDLER_H
#define FILE_DIALOG_HANDLER_H

#include "../message_handler.h"
#include <memory>

class Window;

// Factory function to create FileDialogHandler
std::shared_ptr<MessageHandler> createFileDialogHandler(Window* window);

#endif // FILE_DIALOG_HANDLER_H
