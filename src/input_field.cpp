#include "../include/input_field.h"
#include "../include/control.h"
#include "platform/platform_impl.h"
#include <stdexcept>

InputField::InputField(Component* owner, Control* parent, int x, int y, int width, int height, const std::string& placeholder)
    : Control(owner, parent), nativeHandle_(nullptr), placeholder_(placeholder) {
    // Set bounds using Control's methods
    SetBounds(x, y, width, height);
    
    if (!GetParent() || !GetParent()->getNativeHandle()) {
        throw std::runtime_error("Parent control must be created before creating input field");
    }
    
    createNativeInputField();
    resetDefaultNameCache();
    debugLogLifecycleCreation(this, GetOwner(), GetParent());
}

InputField::~InputField() {
    if (nativeHandle_) {
        destroyNativeInputField();
    }
}

InputField::InputField(InputField&& other) noexcept
    : Control(std::move(other)),
      nativeHandle_(other.nativeHandle_),
      placeholder_(std::move(other.placeholder_)) {
    other.nativeHandle_ = nullptr;
}

InputField& InputField::operator=(InputField&& other) noexcept {
    if (this != &other) {
        if (nativeHandle_) {
            destroyNativeInputField();
        }
        
        Control::operator=(std::move(other));
        nativeHandle_ = other.nativeHandle_;
        placeholder_ = std::move(other.placeholder_);
        
        other.nativeHandle_ = nullptr;
    }
    return *this;
}

void InputField::createNativeInputField() {
    if (!GetParent() || !GetParent()->getNativeHandle()) {
        return;
    }
    
    nativeHandle_ = platform::createInputField(GetParent()->getNativeHandle(),
                                              GetLeft(), GetTop(),
                                              GetWidth(), GetHeight(),
                                              placeholder_);
    if (!nativeHandle_) {
        throw std::runtime_error("Failed to create input field");
    }
}

void InputField::destroyNativeInputField() {
    if (nativeHandle_) {
        platform::destroyInputField(nativeHandle_);
        nativeHandle_ = nullptr;
    }
}

void InputField::setText(const std::string& text) {
    if (nativeHandle_) {
        platform::setInputText(nativeHandle_, text);
    }
}

std::string InputField::getText() const {
    if (nativeHandle_) {
        return platform::getInputText(nativeHandle_);
    }
    return "";
}

void InputField::setPlaceholder(const std::string& placeholder) {
    if (placeholder_ != placeholder) {
        placeholder_ = placeholder;
        // Platform-specific implementation would update native input placeholder here
    }
}

std::string InputField::getPlaceholder() const {
    return placeholder_;
}

void InputField::OnParentChanged(Control* oldParent, Control* newParent) {
    Control::OnParentChanged(oldParent, newParent);
    // Recreate native input field with new parent
    if (nativeHandle_) {
        destroyNativeInputField();
    }
    if (newParent && newParent->getNativeHandle()) {
        createNativeInputField();
    }
}

void InputField::OnBoundsChanged() {
    Control::OnBoundsChanged();
    updateNativeInputFieldBounds();
}

void InputField::updateNativeInputFieldBounds() {
    if (nativeHandle_) {
        // Platform-specific implementation would update input field position/size here
        // For now, we rely on platform layer to handle this via resize events
    }
}
