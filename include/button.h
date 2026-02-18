#ifndef BUTTON_H
#define BUTTON_H

#include "control.h"
#include <string>
#include <functional>

class Control;

// Platform-agnostic Button interface
// Button inherits from Control, so it supports Owner and Parent
class Button : public Control {
public:
    // Constructor: Button(owner, parent, x, y, width, height, label)
    Button(Component* owner = nullptr, Control* parent = nullptr,
           int x = 0, int y = 0, int width = 100, int height = 30,
           const std::string& label = "");
    ~Button();
    
    // Non-copyable, movable
    Button(const Button&) = delete;
    Button& operator=(const Button&) = delete;
    Button(Button&&) noexcept;
    Button& operator=(Button&&) noexcept;
    
    // Use std::function to allow lambdas with captures
    // Callback receives the parent control (could be Window, Container, etc.)
    void setCallback(std::function<void(Control*)> callback);
    void setLabel(const std::string& label);
    std::string getLabel() const;
    
    // Platform-specific handle (opaque pointer)
    void* getNativeHandle() const override { return nativeHandle_; }
    
protected:
    // Override Control virtual methods
    void OnParentChanged(Control* oldParent, Control* newParent) override;
    void OnBoundsChanged() override;
    
private:
    void* nativeHandle_;
    std::string label_;
    std::function<void(Control*)> callback_;
    
    // Platform-specific implementation
    void createNativeButton();
    void destroyNativeButton();
    void updateNativeButtonBounds();
    
    // Static callback wrapper
    static void callbackWrapper(void* userData);
};

#endif // BUTTON_H
