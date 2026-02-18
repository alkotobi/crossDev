#ifndef APP_HANDLERS_H
#define APP_HANDLERS_H

/**
 * App-specific handler registration hook.
 * CrossDev framework calls this at the end of registerHandlers().
 * Default (weak) implementation is a no-op.
 * Apps (e.g. Cars) provide their own implementation to register createInvoice, etc.
 */
class MessageRouter;
void registerAppHandlers(MessageRouter* router);

#endif // APP_HANDLERS_H
