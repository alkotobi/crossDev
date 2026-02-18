#include "../../include/message_handler.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <ctime>

#ifdef _WIN32
    #include <windows.h>
#elif __APPLE__
    #include <TargetConditionals.h>
    #if TARGET_OS_MAC
        #include <sys/utsname.h>
    #endif
#elif __linux__
    #include <sys/utsname.h>
#endif

// Handler for getting application/system information
class AppInfoHandler : public MessageHandler {
public:
    bool canHandle(const std::string& messageType) const override {
        return messageType == "getAppInfo";
    }
    
    nlohmann::json handle(const nlohmann::json& payload, const std::string& requestId) override {
        (void)requestId;
        nlohmann::json result;
        
        // Get platform name
        std::string platform;
#ifdef _WIN32
        platform = "Windows";
#elif __APPLE__
    #if TARGET_OS_IPHONE
        platform = "iOS";
    #else
        platform = "macOS";
    #endif
#elif __linux__
        platform = "Linux";
#else
        platform = "Unknown";
#endif
        
        // Get system info
        std::string osVersion = "Unknown";
#ifdef _WIN32
        OSVERSIONINFOEX osvi;
        ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4996)  // GetVersionEx deprecated - acceptable for basic OS info
#endif
        if (GetVersionEx((OSVERSIONINFO*)&osvi)) {
            osVersion = std::to_string(osvi.dwMajorVersion) + "." + std::to_string(osvi.dwMinorVersion);
        }
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
#elif __APPLE__ || __linux__
        struct utsname unameData;
        if (uname(&unameData) == 0) {
            osVersion = std::string(unameData.release);
        }
#endif
        
        // Build response
        result["platform"] = platform;
        result["osVersion"] = osVersion;
        result["appName"] = "CrossDev";
        result["version"] = "1.0.0";
        result["timestamp"] = std::time(nullptr);
        
        // If payload requests specific info, filter response
        if (payload.contains("fields") && payload["fields"].is_array()) {
            nlohmann::json filtered;
            for (const auto& field : payload["fields"]) {
                std::string fieldName = field.get<std::string>();
                if (result.contains(fieldName)) {
                    filtered[fieldName] = result[fieldName];
                }
            }
            return filtered;
        }
        
        return result;
    }
    
    std::vector<std::string> getSupportedTypes() const override {
        return {"getAppInfo"};
    }
};

// Factory function
std::shared_ptr<MessageHandler> createAppInfoHandler() {
    return std::make_shared<AppInfoHandler>();
}
