#ifndef MESSAGE_HANDLER_H
#define MESSAGE_HANDLER_H

#include <string>
#include <vector>
#include <memory>
#include <nlohmann/json.hpp>

// Base class for all message handlers
class MessageHandler {
public:
    virtual ~MessageHandler() = default;
    
    // Check if this handler can handle the given message type
    virtual bool canHandle(const std::string& messageType) const = 0;
    
    // Handle the message and optionally return a response
    // Returns JSON object response (or empty JSON for void operations)
    virtual nlohmann::json handle(const nlohmann::json& payload, const std::string& requestId) = 0;
    
    // Get all message types this handler supports
    virtual std::vector<std::string> getSupportedTypes() const = 0;
};

#endif // MESSAGE_HANDLER_H
