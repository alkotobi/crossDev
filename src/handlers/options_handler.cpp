#include "../../include/handlers/options_handler.h"
#include "../../include/config_manager.h"
#include "../../include/message_handler.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>

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
        try {
            if (!payload.contains("options") || !payload["options"].is_object()) {
                result["success"] = false;
                result["error"] = "Missing or invalid 'options' object";
                return result;
            }
            std::string path = ConfigManager::getOptionsFilePath();
            std::ofstream f(path);
            if (!f.is_open()) {
                result["success"] = false;
                result["error"] = "Failed to open options.json for writing";
                return result;
            }
            f << payload["options"].dump(2);
            f.close();
            ConfigManager::getInstance().loadOptions();
            result["success"] = true;
        } catch (const std::exception& e) {
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
