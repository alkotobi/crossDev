#ifndef CALCULATOR_HANDLER_H
#define CALCULATOR_HANDLER_H

#include "../message_handler.h"
#include <memory>

// Factory function to create CalculatorHandler
std::shared_ptr<MessageHandler> createCalculatorHandler();

#endif // CALCULATOR_HANDLER_H
