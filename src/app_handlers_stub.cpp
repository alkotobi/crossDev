/**
 * Stub implementation of registerAppHandlers.
 * No-op when building vanilla CrossDev without an app module.
 * App modules (e.g. apps/cars) override with a strong symbol.
 */
#include "../include/app_handlers.h"
#include "../include/message_router.h"

extern "C" {
#if defined(__GNUC__) || defined(__clang__)
__attribute__((weak))
#endif
void registerAppHandlers(MessageRouter* /*router*/) {
}
}
