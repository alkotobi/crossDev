#pragma once

#include <string>
#include <memory>
#include "../../../include/message_handler.h"

/**
 * @file mdbTosqlite_handler.h
 * @brief App-specific handler for mdbTosqlite conversion utility
 * 
 * ARCHITECTURE PATTERN - App-Specific Code Separation:
 * ===================================================
 * 
 * This handler demonstrates the recommended approach for separating app-specific C++ code
 * from the general CrossDev framework.
 * 
 * Directory Structure:
 * --------------------
 * cross_dev/
 *   ├── src/
 *   │   ├── handlers/               <- General handlers (shared by all apps)
 *   │   │   ├── options_handler.cpp
 *   │   │   ├── file_dialog_handler.cpp
 *   │   │   └── ...
 *   │   └── app_runner.cpp          <- Conditionally registers handlers
 *   │
 *   └── apps/
 *       └── mdbTosqlite/            <- App-specific code
 *           ├── handlers/           <- App-specific handlers
 *           │   ├── mdbTosqlite_handler.h
 *           │   ├── mdbTosqlite_handler.cpp
 *           │   └── conversion_logic.cpp
 *           └── ...
 * 
 * Communication Flow:
 * -------------------
 * Vue.js App
 *      ↓
 *      window.parent.postMessage({
 *        type: "selectFolder",
 *        channel: "mdbTosqlite"
 *      })
 *      ↓
 * C++ Message Router
 *      ↓
 * MdbTosqliteHandler (app-specific)
 *      ↓
 * Platform-specific dialog (macOS/Windows/Linux)
 *      ↓
 * Response back to Vue via window.postMessage()
 * 
 * Key Principles:
 * ---------------
 * 1. SEPARATION OF CONCERNS
 *    - General framework code stays in src/handlers/
 *    - App-specific logic goes in apps/{appName}/handlers/
 * 
 * 2. CONDITIONAL REGISTRATION
 *    - app_runner.cpp detects which app is running
 *    - Registers only relevant handlers for that app
 * 
 * 3. MESSAGE-BASED COMMUNICATION
 *    - Vue communicates with C++ via JSON messages
 *    - Type field identifies which handler processes the message
 *    - Channel field specifies the app (optional, for debugging)
 * 
 * 4. REUSABLE INFRASTRUCTURE
 *    - App-specific handlers extend MessageHandler base class
 *    - Use existing platform APIs (file dialogs, system calls, etc.)
 *    - Can be extended with app-specific platform code if needed
 * 
 * 5. BUILD INTEGRATION
 *    - CMakeLists.txt includes app-specific source files
 *    - Only relevant handlers compiled for each app
 * 
 * Example: Adding a new app-specific handler
 * -------------------------------------------
 * 1. Create directory: apps/myApp/handlers/
 * 2. Create myApp_handler.h and myApp_handler.cpp
 * 3. Extend MessageHandler and implement:
 *    - canHandle() - return true for your message types
 *    - handle() - process the message
 *    - getSupportedTypes() - list your message types
 * 4. Create factory function: createMyAppHandler()
 * 5. In app_runner.cpp, conditionally register:
 *    if (windowName == "MyApp") {
 *        messageRouter->registerHandler(createMyAppHandler());
 *    }
 * 6. Update CMakeLists.txt to include your .cpp files
 * 7. From Vue: window.parent.postMessage({type: "myMessageType"})
 */

// Factory function - creates and returns a handler instance
// The caller owns the shared_ptr and ensures proper cleanup
std::shared_ptr<MessageHandler> createMdbTosqliteHandler();
