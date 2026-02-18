#include "../../include/message_handler.h"
#include "../../include/window.h"
#include "../platform/platform_impl.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <vector>
#include <functional>

// Handler for opening file dialogs
class FileDialogHandler : public MessageHandler {
public:
    FileDialogHandler(Window* window) : window_(window) {}
    
    bool canHandle(const std::string& messageType) const override {
        return messageType == "openFileDialog";
    }
    
    nlohmann::json handle(const nlohmann::json& payload, const std::string& requestId) override {
        (void)requestId;
        nlohmann::json result;
        
        if (!window_ || !window_->getNativeHandle()) {
            result["success"] = false;
            result["error"] = "Window not available";
            return result;
        }
        
        // Extract dialog options from payload
        std::string title = "Select File";
        std::string filter = "All Files (*.*)|*.*";
        
        if (payload.contains("title") && payload["title"].is_string()) {
            title = payload["title"].get<std::string>();
        }
        
        if (payload.contains("filter") && payload["filter"].is_string()) {
            filter = payload["filter"].get<std::string>();
        }
        
        // Open file dialog. Use nullptr for parent - when invoked from child window (Settings),
        // modal-to-main-window can make the dialog appear disabled or cause focus issues.
        // Application-modal (no parent) works reliably from any window.
        std::string selectedPath;
        bool selected = platform::showOpenFileDialog(
            nullptr,  // No parent - avoids modality/focus issues from child windows
            title,
            filter,
            selectedPath
        );
        
        if (selected && !selectedPath.empty()) {
            result["success"] = true;
            result["filePath"] = selectedPath;
            
            // Extract just the filename from the path
            size_t lastSlash = selectedPath.find_last_of("/\\");
            if (lastSlash != std::string::npos) {
                result["fileName"] = selectedPath.substr(lastSlash + 1);
            } else {
                result["fileName"] = selectedPath;
            }
        } else {
            result["success"] = false;
            result["error"] = "No file selected";
        }
        
        return result;
    }
    
    std::vector<std::string> getSupportedTypes() const override {
        return {"openFileDialog"};
    }
    
private:
    Window* window_;
};

// Factory function
std::shared_ptr<MessageHandler> createFileDialogHandler(Window* window) {
    return std::make_shared<FileDialogHandler>(window);
}
