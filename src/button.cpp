#include "../include/button.h"
#include "../include/control.h"
#include "platform/platform_impl.h"
#include <stdexcept>
#include <functional>

Button::Button(Component* owner, Control* parent, int x, int y, int width, int height, const std::string& label)
    : Control(owner, parent), nativeHandle_(nullptr), label_(label) {
    // Set bounds using Control's methods
    SetBounds(x, y, width, height);
    
    if (!GetParent() || !GetParent()->getNativeHandle()) {
        throw std::runtime_error("Parent control must be created before creating button");
    }
    
    createNativeButton();
    resetDefaultNameCache();
    debugLogLifecycleCreation(this, GetOwner(), GetParent());
}

Button::~Button() {
    if (nativeHandle_) {
        destroyNativeButton();
    }
}

Button::Button(Button&& other) noexcept
    : Control(std::move(other)),
      nativeHandle_(other.nativeHandle_),
      label_(std::move(other.label_)),
      callback_(std::move(other.callback_)) {
    other.nativeHandle_ = nullptr;
}

Button& Button::operator=(Button&& other) noexcept {
    if (this != &other) {
        if (nativeHandle_) {
            destroyNativeButton();
        }
        
        Control::operator=(std::move(other));
        nativeHandle_ = other.nativeHandle_;
        label_ = std::move(other.label_);
        callback_ = std::move(other.callback_);
        
        other.nativeHandle_ = nullptr;
    }
    return *this;
}

void Button::createNativeButton() {
    if (!GetParent() || !GetParent()->getNativeHandle()) {
        return;
    }
    
    nativeHandle_ = platform::createButton(GetParent()->getNativeHandle(), 
                                          GetLeft(), GetTop(), 
                                          GetWidth(), GetHeight(), 
                                          label_, this);
    if (!nativeHandle_) {
        throw std::runtime_error("Failed to create button");
    }
    // Set callback wrapper
    platform::setButtonCallback(nativeHandle_, callbackWrapper);
}

void Button::destroyNativeButton() {
    if (nativeHandle_) {
        platform::destroyButton(nativeHandle_);
        nativeHandle_ = nullptr;
    }
}

void Button::setCallback(std::function<void(Control*)> callback) {
    callback_ = callback;
}

void Button::setLabel(const std::string& label) {
    if (label_ != label) {
        label_ = label;
        // Platform-specific implementation would update native button label here
    }
}

std::string Button::getLabel() const {
    return label_;
}

void Button::OnParentChanged(Control* oldParent, Control* newParent) {
    Control::OnParentChanged(oldParent, newParent);
    // Recreate native button with new parent
    if (nativeHandle_) {
        destroyNativeButton();
    }
    if (newParent && newParent->getNativeHandle()) {
        createNativeButton();
    }
}

void Button::OnBoundsChanged() {
    Control::OnBoundsChanged();
    updateNativeButtonBounds();
}

void Button::updateNativeButtonBounds() {
    if (nativeHandle_) {
        // Platform-specific implementation would update button position/size here
        // For now, we rely on platform layer to handle this via resize events
    }
}

void Button::callbackWrapper(void* userData) {
    Button* button = static_cast<Button*>(userData);
    if (button && button->callback_ && button->GetParent()) {
        button->callback_(button->GetParent());
    }
}
