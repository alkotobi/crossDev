/**
 * Cars app handler registration.
 * Provides strong symbol for registerAppHandlers, overriding the weak stub.
 */
#include "../../include/app_handlers.h"
#include "../../include/message_router.h"
#include "handlers/create_invoice_handler.h"

void registerAppHandlers(MessageRouter* router) {
    router->registerHandler(createCreateInvoiceHandler());
}
