#include "../../include/handlers/options_handler.h"
#include "../../include/config_manager.h"
#include "../../include/message_handler.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>
#include <iostream>

// Handler for options.json: getOptionsPath, readOptions, writeOptions
class OptionsHandler : public MessageHandler {
public:
    bool canHandle(const std::string& messageType) const override {
        return messageType == "getOptionsPath" || messageType == "readOptions" || messageType == "writeOptions";
    }

    nlohmann::json handle(const nlohmann::json& payload, const std::string& requestId) override {
        (void)requestId;
        std::string type;
        if (payload.contains("_type") && payload["_type"].is_string()) {
            type = payload["_type"].get<std::string>();
        }
        if (type == "writeOptions") {
            return handleWriteOptions(payload);
        }
        if (type == "readOptions") {
            return handleReadOptions();
        }
        return handleGetOptionsPath();
    }

    nlohmann::json handleGetOptionsPath() const {
        nlohmann::json result;
        try {
            std::string path = ConfigManager::getOptionsFilePath();
            result["success"] = true;
            result["path"] = path;
        } catch (const std::exception& e) {
            result["success"] = false;
            result["error"] = e.what();
        }
        return result;
    }

    nlohmann::json handleReadOptions() const {
        nlohmann::json result;
        try {
            std::string path = ConfigManager::getOptionsFilePath();
            std::ifstream f(path);
            if (!f.is_open()) {
                result["success"] = false;
                result["error"] = "Failed to open options.json";
                return result;
            }
            std::stringstream ss;
            ss << f.rdbuf();
            std::string content = ss.str();
            f.close();
            if (content.empty()) {
                result["success"] = true;
                result["options"] = ConfigManager::getInstance().getOptions();
                return result;
            }
            result["success"] = true;
            result["options"] = nlohmann::json::parse(content);
        } catch (const std::exception& e) {
            result["success"] = false;
            result["error"] = e.what();
        }
        return result;
    }

    nlohmann::json handleWriteOptions(const nlohmann::json& payload) const {
        nlohmann::json result;
        std::cout << "[OptionsHandler] handleWriteOptions called" << std::endl;
        try {
            if (!payload.contains("options") || !payload["options"].is_object()) {
                std::cout << "[OptionsHandler] ERROR: Missing or invalid 'options' object" << std::endl;
                result["success"] = false;
                result["error"] = "Missing or invalid 'options' object";
                return result;
            }
            
            // CRITICAL: Log what we're receiving from the client
            auto optionsToSave = payload["options"];
            std::cout << "\n========== OPTIONS DEBUG ==========" << std::endl;
            std::cout << "FULL PAYLOAD:\n" << optionsToSave.dump(2) << std::endl;
            std::cout << "================================\n" << std::endl;
            
            std::string path = ConfigManager::getOptionsFilePath();
            std::cout << "[OptionsHandler] Writing to: " << path << std::endl;
            std::ofstream f(path);
            if (!f.is_open()) {
                std::cout << "[OptionsHandler] ERROR: Failed to open file for writing" << std::endl;
                result["success"] = false;
                result["error"] = "Failed to open options.json for writing";
                return result;
            }
            f << payload["options"].dump(2);
            f.flush();
            f.close();
            std::cout << "[OptionsHandler] Successfully wrote options to file" << std::endl;
            
            // Verify what was actually written
            std::ifstream verifyFile(path);
            if (verifyFile.is_open()) {
                std::stringstream ss;
                ss << verifyFile.rdbuf();
                verifyFile.close();
                std::cout << "\n========== FILE VERIFICATION ==========" << std::endl;
                std::cout << "File on disk now contains:\n" << ss.str().substr(0, 500) << "\n..." << std::endl;
                std::cout << "=======================================\n" << std::endl;
            }
            
            std::cout << "[OptionsHandler] Reloading ConfigManager..." << std::endl;
            ConfigManager::getInstance().loadOptions();
            std::cout << "[OptionsHandler] ConfigManager reloaded successfully" << std::endl;
            result["success"] = true;
        } catch (const std::exception& e) {
            std::cout << "[OptionsHandler] EXCEPTION: " << e.what() << std::endl;
            result["success"] = false;
            result["error"] = e.what();
        }
        return result;
    }

    std::vector<std::string> getSupportedTypes() const override {
        return {"getOptionsPath", "readOptions", "writeOptions"};
    }
};

std::shared_ptr<MessageHandler> createOptionsHandler() {
    return std::make_shared<OptionsHandler>();
}
