#include "../../include/message_handler.h"
#include "../../include/base64.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <fstream>
#include <vector>

// Handler for writing binary data to files
class WriteFileHandler : public MessageHandler {
public:
    bool canHandle(const std::string& messageType) const override {
        return messageType == "writeFile";
    }

    nlohmann::json handle(const nlohmann::json& payload, const std::string& requestId) override {
        (void)requestId;
        nlohmann::json result;

        if (!payload.contains("path") || !payload["path"].is_string()) {
            result["success"] = false;
            result["error"] = "Missing or invalid 'path' in payload";
            return result;
        }

        std::string path = payload["path"].get<std::string>();
        if (path.empty()) {
            result["success"] = false;
            result["error"] = "Path cannot be empty";
            return result;
        }

        std::string base64Data;
        if (payload.contains("data")) {
            if (payload["data"].is_string()) {
                base64Data = payload["data"].get<std::string>();
            } else if (payload["data"].contains("__base64") && payload["data"]["__base64"].is_string()) {
                base64Data = payload["data"]["__base64"].get<std::string>();
            }
        }
        if (base64Data.empty()) {
            result["success"] = false;
            result["error"] = "Missing or invalid 'data' in payload (expect base64 string or { __base64: '...' })";
            return result;
        }

        std::vector<unsigned char> buffer = base64::decode(base64Data);
        if (buffer.empty() && !base64Data.empty()) {
            result["success"] = false;
            result["error"] = "Invalid base64 data";
            return result;
        }

        std::ofstream file(path, std::ios::binary);
        if (!file.is_open()) {
            result["success"] = false;
            result["error"] = "Failed to open file for writing: " + path;
            return result;
        }

        if (!file.write(reinterpret_cast<const char*>(buffer.data()), static_cast<std::streamsize>(buffer.size()))) {
            result["success"] = false;
            result["error"] = "Failed to write file: " + path;
            return result;
        }

        result["success"] = true;
        result["bytesWritten"] = static_cast<int64_t>(buffer.size());
        return result;
    }

    std::vector<std::string> getSupportedTypes() const override {
        return {"writeFile"};
    }
};

std::shared_ptr<MessageHandler> createWriteFileHandler() {
    return std::make_shared<WriteFileHandler>();
}
