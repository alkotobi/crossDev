#include "../../include/message_handler.h"
#include "../../include/base64.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

// Handler for reading files as binary (returns base64)
class ReadFileHandler : public MessageHandler {
public:
    bool canHandle(const std::string& messageType) const override {
        return messageType == "readFile";
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

        // Security: reject absolute paths outside current directory on first run
        // For demo we allow relative paths and simple absolute paths
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            result["success"] = false;
            result["error"] = "Failed to open file: " + path;
            return result;
        }

        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<unsigned char> buffer(static_cast<size_t>(size));
        if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
            result["success"] = false;
            result["error"] = "Failed to read file: " + path;
            return result;
        }

        std::string base64Data = base64::encode(buffer);
        result["success"] = true;
        result["data"] = base64Data;
        result["size"] = static_cast<int64_t>(size);
        return result;
    }

    std::vector<std::string> getSupportedTypes() const override {
        return {"readFile"};
    }
};

std::shared_ptr<MessageHandler> createReadFileHandler() {
    return std::make_shared<ReadFileHandler>();
}
