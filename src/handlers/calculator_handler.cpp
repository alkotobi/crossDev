#include "../../include/message_handler.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <vector>
#include <stdexcept>

// Handler for performing calculations
class CalculatorHandler : public MessageHandler {
public:
    bool canHandle(const std::string& messageType) const override {
        return messageType == "calculate";
    }
    
    nlohmann::json handle(const nlohmann::json& payload, const std::string& requestId) override {
        (void)requestId;
        nlohmann::json result;
        
        // Validate payload
        if (!payload.contains("operation") || !payload.contains("a") || !payload.contains("b")) {
            result["success"] = false;
            result["error"] = "Missing required fields: operation, a, b";
            return result;
        }
        
        std::string operation = payload["operation"].get<std::string>();
        double a = payload["a"].get<double>();
        double b = payload["b"].get<double>();
        
        double calculationResult = 0.0;
        bool success = true;
        std::string error;
        
        try {
            if (operation == "add") {
                calculationResult = a + b;
            } else if (operation == "subtract") {
                calculationResult = a - b;
            } else if (operation == "multiply") {
                calculationResult = a * b;
            } else if (operation == "divide") {
                if (b == 0.0) {
                    success = false;
                    error = "Division by zero";
                } else {
                    calculationResult = a / b;
                }
            } else {
                success = false;
                error = "Unknown operation: " + operation;
            }
        } catch (const std::exception& e) {
            success = false;
            error = std::string("Calculation error: ") + e.what();
        }
        
        result["success"] = success;
        if (success) {
            result["result"] = calculationResult;
            result["operation"] = operation;
            result["a"] = a;
            result["b"] = b;
        } else {
            result["error"] = error;
        }
        
        return result;
    }
    
    std::vector<std::string> getSupportedTypes() const override {
        return {"calculate"};
    }
};

// Factory function
std::shared_ptr<MessageHandler> createCalculatorHandler() {
    return std::make_shared<CalculatorHandler>();
}
