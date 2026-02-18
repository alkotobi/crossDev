#include "../include/message_router.h"
#include "../include/webview.h"
#include "../include/message_handler.h"
#include "platform/platform_impl.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <sstream>
#ifdef _WIN32
#include <windows.h>
#endif
#ifdef COMPONENT_DEBUG_LIFECYCLE
#ifdef _WIN32
#define MSG_LOG(s) do { std::cout << s; OutputDebugStringA(s); } while(0)
#else
#define MSG_LOG(s) std::cout << s
#endif
#else
#define MSG_LOG(s) do {} while(0)
#endif

MessageRouter::MessageRouter(WebView* webView) : webView_(webView) {
    if (!webView_) {
        throw std::runtime_error("MessageRouter requires a valid WebView");
    }
}

MessageRouter::~MessageRouter() {
    handlers_.clear();
}

void MessageRouter::registerHandler(const std::string& messageType, std::shared_ptr<MessageHandler> handler) {
    if (!handler) {
        return;
    }
    handlers_[messageType] = handler;
}

void MessageRouter::registerHandler(std::shared_ptr<MessageHandler> handler) {
    if (!handler) {
        return;
    }
    // Register for all types this handler supports
    for (const auto& type : handler->getSupportedTypes()) {
        handlers_[type] = handler;
    }
}

void MessageRouter::routeMessage(const std::string& jsonMessage) {
    MSG_LOG("=== MessageRouter::routeMessage called ===\n");
    MSG_LOG(("  jsonMessage: " + jsonMessage.substr(0, 200) + (jsonMessage.length() > 200 ? "..." : "") + "\n").c_str());
    
    if (jsonMessage.empty()) {
        MSG_LOG("  Message is empty, returning\n");
        return;
    }
    
    std::string type, payload, requestId;
    if (!parseMessage(jsonMessage, type, payload, requestId)) {
        std::cerr << "[MessageRouter] Failed to parse message: " << jsonMessage << std::endl;
        if (!requestId.empty()) {
            sendResponse(requestId, "", "Failed to parse message");
        }
        return;
    }
    
    MSG_LOG(("  Parsed - type: " + type + ", requestId: " + requestId + "\n").c_str());
    std::cout << "[MessageRouter] Received message type: " << type << " (requestId: " << requestId << ")" << std::endl;
    
    // Find handler for this message type
    auto it = handlers_.find(type);
    if (it == handlers_.end()) {
        std::cerr << "[MessageRouter] ERROR: No handler registered for message type: " << type << std::endl;
        std::cerr << "[MessageRouter] Registered handlers: ";
        for (const auto& pair : handlers_) {
            std::cerr << pair.first << " ";
        }
        std::cerr << std::endl;
        if (!requestId.empty()) {
            sendResponse(requestId, "", "Unknown message type: " + type);
        }
        return;
    }
    
    // Parse payload JSON
    nlohmann::json payloadJson;
    try {
        if (!payload.empty()) {
            payloadJson = nlohmann::json::parse(payload);
        }
        payloadJson["_type"] = type;  // Inject message type so handlers can use it
    } catch (const nlohmann::json::exception& e) {
        std::cerr << "[MessageRouter] Failed to parse payload JSON: " << e.what() << std::endl;
        if (!requestId.empty()) {
            sendResponse(requestId, "", "Invalid payload JSON: " + std::string(e.what()));
        }
        return;
    }
    
    // Call handler
    MSG_LOG(("Calling handler for type: " + type + "\n").c_str());
    std::cout << "[MessageRouter] Calling handler for type: " << type << std::endl;
    try {
        nlohmann::json result = it->second->handle(payloadJson, requestId);
        MSG_LOG(("Handler returned result: " + result.dump().substr(0, 150) + "\n").c_str());
        std::cout << "[MessageRouter] Handler returned successfully" << std::endl;
        
        // Send response if requestId was provided
        if (!requestId.empty()) {
            std::string resultStr = result.is_null() ? "null" : result.dump();
            MSG_LOG(("Sending response for requestId: " + requestId + "\n").c_str());
            sendResponse(requestId, resultStr, "");
        } else {
#ifdef COMPONENT_DEBUG_LIFECYCLE
            std::cout << "No requestId, skipping response" << std::endl;
#endif
        }
    } catch (const std::exception& e) {
        std::cerr << "Handler error: " << e.what() << std::endl;
        if (!requestId.empty()) {
            sendResponse(requestId, "", "Handler error: " + std::string(e.what()));
        }
    }
}

void MessageRouter::sendResponse(const std::string& requestId, const std::string& resultJson, const std::string& error) {
    MSG_LOG("=== MessageRouter::sendResponse called ===\n");
    MSG_LOG(("  requestId: " + requestId + "\n").c_str());
    MSG_LOG(("  resultJson: " + resultJson.substr(0, 100) + "\n").c_str());
    MSG_LOG(("  error: " + error + "\n").c_str());
    
    if (!webView_) {
        std::cerr << "ERROR: webView_ is null!" << std::endl;
#ifdef _WIN32
        OutputDebugStringA("ERROR: MessageRouter webView_ is null!\n");
#endif
        return;
    }
    
    // Create response JSON
    nlohmann::json response;
    response["requestId"] = requestId;
    if (!error.empty()) {
        response["error"] = error;
        response["result"] = nullptr;
    } else {
        response["error"] = nullptr;
        if (!resultJson.empty() && resultJson != "null") {
            try {
                response["result"] = nlohmann::json::parse(resultJson);
            } catch (...) {
                response["result"] = resultJson; // Fallback to string
            }
        } else {
            response["result"] = nullptr;
        }
    }
    
    // Send to JavaScript via platform API (PostWebMessageAsJson - page receives event.data as object)
    std::string responseStr = response.dump();
    MSG_LOG(("  postMessageToJavaScript requestId=" + requestId + " len=" + std::to_string(responseStr.length()) + "\n").c_str());
    platform::postMessageToJavaScript(webView_->getNativeHandle(), responseStr);
    MSG_LOG("  Response sent! (page receives as parsed object via addEventListener)\n");
}

bool MessageRouter::parseMessage(const std::string& jsonMessage, std::string& type, 
                                 std::string& payload, std::string& requestId) {
    try {
        nlohmann::json msg = nlohmann::json::parse(jsonMessage);
        
        // Extract type (required)
        if (!msg.contains("type") || !msg["type"].is_string()) {
            return false;
        }
        type = msg["type"].get<std::string>();
        
        // Extract payload (optional)
        if (msg.contains("payload")) {
            payload = msg["payload"].dump();
        }
        
        // Extract requestId (optional)
        if (msg.contains("requestId") && msg["requestId"].is_string()) {
            requestId = msg["requestId"].get<std::string>();
        }
        
        return true;
    } catch (const nlohmann::json::exception& e) {
        std::cerr << "JSON parse error: " << e.what() << std::endl;
        return false;
    }
}
