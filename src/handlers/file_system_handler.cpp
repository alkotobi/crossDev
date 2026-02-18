#include "../../include/message_handler.h"
#include "../../include/handlers/file_system_handler.h"
#include <nlohmann/json.hpp>
#include <filesystem>

namespace fs = std::filesystem;

// Resolve path and ensure it doesn't escape the base (cwd).
// Returns empty string on security violation.
static std::string resolvePath(const std::string& path, const fs::path& base) {
    try {
        fs::path baseAbs = fs::absolute(base);
        fs::path p = path.empty() ? baseAbs : (baseAbs / path);
        fs::path normalized = p.lexically_normal();
        auto [baseIt, pathIt] = std::mismatch(baseAbs.begin(), baseAbs.end(),
                                              normalized.begin(), normalized.end());
        if (baseIt != baseAbs.end()) {
            return "";  // Path escaped base
        }
        return normalized.string();
    } catch (...) {
        return "";
    }
}

// Handler for filesystem operations: exists, listDir, mkdir, deleteFile, rename, stat
class FileSystemHandler : public MessageHandler {
public:
    bool canHandle(const std::string& messageType) const override {
        return messageType == "exists" || messageType == "listDir" ||
               messageType == "mkdir" || messageType == "deleteFile" ||
               messageType == "rename" || messageType == "stat";
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

        // Use injected _type from router, or fallback to operation in payload
        std::string op;
        if (payload.contains("_type") && payload["_type"].is_string()) {
            op = payload["_type"].get<std::string>();
        } else if (payload.contains("operation") && payload["operation"].is_string()) {
            op = payload["operation"].get<std::string>();
        }
        if (op.empty() || (op != "exists" && op != "listDir" && op != "mkdir" &&
            op != "deleteFile" && op != "rename" && op != "stat")) {
            result["success"] = false;
            result["error"] = "Invalid operation (expected: exists|listDir|mkdir|deleteFile|rename|stat)";
            return result;
        }

        fs::path base = fs::current_path();
        std::string resolved = resolvePath(path, base);
        if (resolved.empty()) {
            result["success"] = false;
            result["error"] = "Invalid or disallowed path";
            return result;
        }
        fs::path p(resolved);

        if (op == "exists") {
            try {
                result["success"] = true;
                result["exists"] = fs::exists(p);
                return result;
            } catch (const fs::filesystem_error& e) {
                result["success"] = false;
                result["error"] = std::string(e.what());
                return result;
            }
        }

        if (op == "stat") {
            try {
                if (!fs::exists(p)) {
                    result["success"] = false;
                    result["error"] = "Path does not exist";
                    return result;
                }
                result["success"] = true;
                result["exists"] = true;
                result["isDirectory"] = fs::is_directory(p);
                result["isFile"] = fs::is_regular_file(p);
                if (fs::is_regular_file(p)) {
                    result["size"] = static_cast<int64_t>(fs::file_size(p));
                }
                return result;
            } catch (const fs::filesystem_error& e) {
                result["success"] = false;
                result["error"] = std::string(e.what());
                return result;
            }
        }

        if (op == "listDir") {
            try {
                if (!fs::exists(p)) {
                    result["success"] = false;
                    result["error"] = "Path does not exist";
                    return result;
                }
                if (!fs::is_directory(p)) {
                    result["success"] = false;
                    result["error"] = "Path is not a directory";
                    return result;
                }
                nlohmann::json entries = nlohmann::json::array();
                for (const auto& entry : fs::directory_iterator(p)) {
                    nlohmann::json e;
                    e["name"] = entry.path().filename().string();
                    e["isDirectory"] = entry.is_directory();
                    e["isFile"] = entry.is_regular_file();
                    if (entry.is_regular_file()) {
                        try {
                            e["size"] = static_cast<int64_t>(fs::file_size(entry.path()));
                        } catch (...) {
                            e["size"] = 0;
                        }
                    }
                    entries.push_back(e);
                }
                result["success"] = true;
                result["entries"] = entries;
                return result;
            } catch (const fs::filesystem_error& e) {
                result["success"] = false;
                result["error"] = std::string(e.what());
                return result;
            }
        }

        if (op == "mkdir") {
            try {
                bool created = fs::create_directories(p);
                result["success"] = true;
                result["created"] = created;
                return result;
            } catch (const fs::filesystem_error& e) {
                result["success"] = false;
                result["error"] = std::string(e.what());
                return result;
            }
        }

        if (op == "deleteFile") {
            try {
                if (!fs::exists(p)) {
                    result["success"] = false;
                    result["error"] = "Path does not exist";
                    return result;
                }
                std::uintmax_t n = fs::remove_all(p);
                result["success"] = true;
                result["removedCount"] = static_cast<int64_t>(n);
                return result;
            } catch (const fs::filesystem_error& e) {
                result["success"] = false;
                result["error"] = std::string(e.what());
                return result;
            }
        }

        if (op == "rename") {
            if (!payload.contains("to") || !payload["to"].is_string()) {
                result["success"] = false;
                result["error"] = "Missing or invalid 'to' in payload for rename";
                return result;
            }
            std::string toPath = payload["to"].get<std::string>();
            if (toPath.empty()) {
                result["success"] = false;
                result["error"] = "'to' path cannot be empty";
                return result;
            }
            std::string toResolved = resolvePath(toPath, base);
            if (toResolved.empty()) {
                result["success"] = false;
                result["error"] = "Invalid or disallowed 'to' path";
                return result;
            }
            try {
                fs::rename(p, fs::path(toResolved));
                result["success"] = true;
                return result;
            } catch (const fs::filesystem_error& e) {
                result["success"] = false;
                result["error"] = std::string(e.what());
                return result;
            }
        }

        result["success"] = false;
        result["error"] = "Unknown operation: " + op;
        return result;
    }

    std::vector<std::string> getSupportedTypes() const override {
        return {"exists", "listDir", "mkdir", "deleteFile", "rename", "stat"};
    }
};

std::shared_ptr<MessageHandler> createFileSystemHandler() {
    return std::make_shared<FileSystemHandler>();
}
