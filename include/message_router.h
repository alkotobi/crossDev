#ifndef MESSAGE_ROUTER_H
#define MESSAGE_ROUTER_H

#include "message_handler.h"
#include <string>
#include <memory>
#include <map>
#include <vector>

class WebView;

// Message router that dispatches JavaScript messages to appropriate handlers
class MessageRouter {
public:
    MessageRouter(WebView* webView);
    ~MessageRouter();
    
    // Register a handler for one or more message types
    void registerHandler(const std::string& messageType, std::shared_ptr<MessageHandler> handler);
    void registerHandler(std::shared_ptr<MessageHandler> handler);
    
    // Route a message from JavaScript (called by platform code)
    void routeMessage(const std::string& jsonMessage);
    
    // Send response back to JavaScript
    void sendResponse(const std::string& requestId, const std::string& resultJson, const std::string& error = "");
    
private:
    WebView* webView_;
    std::map<std::string, std::shared_ptr<MessageHandler>> handlers_;
    
    // Helper to parse and validate message
    bool parseMessage(const std::string& jsonMessage, std::string& type, 
                     std::string& payload, std::string& requestId);
};

#endif // MESSAGE_ROUTER_H
