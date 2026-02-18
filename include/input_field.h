#ifndef INPUT_FIELD_H
#define INPUT_FIELD_H

#include "control.h"
#include <string>

// Platform-agnostic InputField interface
// InputField inherits from Control, so it supports Owner and Parent
class InputField : public Control {
public:
    // Constructor: InputField(owner, parent, x, y, width, height, placeholder)
    InputField(Component* owner = nullptr, Control* parent = nullptr,
               int x = 0, int y = 0, int width = 200, int height = 30,
               const std::string& placeholder = "");
    ~InputField();
    
    // Non-copyable, movable
    InputField(const InputField&) = delete;
    InputField& operator=(const InputField&) = delete;
    InputField(InputField&&) noexcept;
    InputField& operator=(InputField&&) noexcept;
    
    void setText(const std::string& text);
    std::string getText() const;
    void setPlaceholder(const std::string& placeholder);
    std::string getPlaceholder() const;
    
    // Platform-specific handle (opaque pointer)
    void* getNativeHandle() const override { return nativeHandle_; }
    
protected:
    // Override Control virtual methods
    void OnParentChanged(Control* oldParent, Control* newParent) override;
    void OnBoundsChanged() override;
    
private:
    void* nativeHandle_;
    std::string placeholder_;
    
    // Platform-specific implementation
    void createNativeInputField();
    void destroyNativeInputField();
    void updateNativeInputFieldBounds();
};

#endif // INPUT_FIELD_H
