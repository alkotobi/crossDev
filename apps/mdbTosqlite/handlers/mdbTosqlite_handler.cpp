#include "mdbTosqlite_handler.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <string>

// We'll include the platform-specific file dialog headers
#ifdef __APPLE__
#include "../../../include/file_dialog.h"
#elif _WIN32
#include "../../../include/file_dialog.h"
#else
#include "../../../include/file_dialog.h"
#endif

/**
 * @brief Implementation of mdbTosqlite-specific message handler
 * 
 * This handler processes messages from the mdbTosqlite Vue.js app and coordinates
 * with platform-specific implementations (macOS, Windows, Linux).
 * 
 * Supported message types:
 *   - "selectFolder": Opens a folder selection dialog and returns the selected path
 *   - "convertDatabase": Initiates database conversion (future implementation)
 */
class MdbTosqliteHandlerImpl : public MessageHandler {
public:
    bool canHandle(const std::string& messageType) const override {
        return messageType == "selectFolder" || messageType == "convertDatabase";
    }

    nlohmann::json handle(const nlohmann::json& payload, const std::string& requestId) override {
        (void)requestId;
        
        std::string type;
        if (payload.contains("type") && payload["type"].is_string()) {
            type = payload["type"].get<std::string>();
        }
        
        std::cout << "[MdbTosqliteHandler] Processing message type: " << type << std::endl;
        
        if (type == "selectFolder") {
            return handleSelectFolder(payload);
        } else if (type == "convertDatabase") {
            return handleConvertDatabase(payload);
        }
        
        nlohmann::json error;
        error["success"] = false;
        error["error"] = "Unknown message type";
        return error;
    }

    std::vector<std::string> getSupportedTypes() const override {
        return {"selectFolder", "convertDatabase"};
    }

private:
    nlohmann::json handleSelectFolder(const nlohmann::json& payload) const {
        nlohmann::json result;
        
        try {
            std::cout << "[MdbTosqliteHandler] Opening folder selection dialog..." << std::endl;
            
            // Get the title from payload, or use default
            std::string title = "Select Access Database Folder";
            if (payload.contains("title") && payload["title"].is_string()) {
                title = payload["title"].get<std::string>();
            }
            
            // Call platform-specific folder dialog
            // Note: showOpenFileDialog is currently used for both files and folders
            // In a production app, you'd have a dedicated showOpenFolderDialog function
            std::string selectedPath = showOpenFileDialog();
            
            if (selectedPath.empty()) {
                result["success"] = false;
                result["cancelled"] = true;
                result["message"] = "Folder selection cancelled";
                return result;
            }
            
            std::cout << "[MdbTosqliteHandler] Folder selected: " << selectedPath << std::endl;
            
            result["success"] = true;
            result["path"] = selectedPath;
            result["cancelled"] = false;
            
        } catch (const std::exception& e) {
            std::cout << "[MdbTosqliteHandler] ERROR: " << e.what() << std::endl;
            result["success"] = false;
            result["error"] = e.what();
        }
        
        return result;
    }

    nlohmann::json handleConvertDatabase(const nlohmann::json& payload) const {
        nlohmann::json result;
        
        try {
            if (!payload.contains("mdbPath") || !payload["mdbPath"].is_string()) {
                result["success"] = false;
                result["error"] = "Missing or invalid mdbPath parameter";
                return result;
            }
            
            std::string mdbPath = payload["mdbPath"].get<std::string>();
            std::cout << "[MdbTosqliteHandler] Converting database: " << mdbPath << std::endl;
            
            // TODO: Implement actual database conversion logic here
            result["success"] = true;
            result["message"] = "Database conversion logic not yet implemented";
            
        } catch (const std::exception& e) {
            result["success"] = false;
            result["error"] = e.what();
        }
        
        return result;
    }
};

// Factory function that creates and returns a handler instance
std::shared_ptr<MessageHandler> createMdbTosqliteHandler() {
    return std::make_shared<MdbTosqliteHandlerImpl>();
}
