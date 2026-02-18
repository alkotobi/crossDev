#include "../include/application.h"
#include "platform/platform_impl.h"

// Application implementation
Application& Application::getInstance() {
    static Application instance;
    return instance;
}

void Application::run() {
    platform::initApplication();
    platform::runApplication();
}

void Application::quit() {
    platform::quitApplication();
}
