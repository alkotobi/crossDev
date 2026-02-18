#include "../include/container.h"
#include "../include/control.h"
#include "../include/layout.h"
#include "platform/platform_impl.h"
#include <stdexcept>

Container::Container(Component* owner, Control* parent, int x, int y, int width, int height, bool flipped)
    : Control(owner, parent),
      nativeHandle_(nullptr),
      bgRed_(255), bgGreen_(255), bgBlue_(255),  // Default white background
      borderStyle_(BorderNone),
      layout_(nullptr),
      flipped_(flipped) {
    // Set bounds using Control's methods
    SetBounds(x, y, width, height);
    
    if (!GetParent() || !GetParent()->getNativeHandle()) {
        throw std::runtime_error("Parent control must be created before creating container");
    }
    
    createNativeContainer();
    resetDefaultNameCache();
    debugLogLifecycleCreation(this, GetOwner(), GetParent());
}

Container::~Container() {
    if (nativeHandle_) {
        destroyNativeContainer();
    }
}

Container::Container(Container&& other) noexcept
    : Control(std::move(other)),
      nativeHandle_(other.nativeHandle_),
      bgRed_(other.bgRed_),
      bgGreen_(other.bgGreen_),
      bgBlue_(other.bgBlue_),
      borderStyle_(other.borderStyle_),
      flipped_(other.flipped_) {
    other.nativeHandle_ = nullptr;
}

Container& Container::operator=(Container&& other) noexcept {
    if (this != &other) {
        if (nativeHandle_) {
            destroyNativeContainer();
        }
        
        Control::operator=(std::move(other));
        nativeHandle_ = other.nativeHandle_;
        bgRed_ = other.bgRed_;
        bgGreen_ = other.bgGreen_;
        bgBlue_ = other.bgBlue_;
        borderStyle_ = other.borderStyle_;
        flipped_ = other.flipped_;
        
        other.nativeHandle_ = nullptr;
    }
    return *this;
}

void Container::createNativeContainer() {
    if (!GetParent() || !GetParent()->getNativeHandle()) {
        return;
    }
    
    // For now, we'll use a simple approach: create a native container view
    // Platform implementations will need to be added
    // For now, this is a placeholder that will be implemented per platform
    nativeHandle_ = platform::createContainer(GetParent()->getNativeHandle(),
                                             GetLeft(), GetTop(),
                                             GetWidth(), GetHeight(), flipped_);
    if (!nativeHandle_) {
        throw std::runtime_error("Failed to create container");
    }
    
    updateNativeContainerAppearance();
}

void Container::destroyNativeContainer() {
    if (nativeHandle_) {
        platform::destroyContainer(nativeHandle_);
        nativeHandle_ = nullptr;
    }
}

void Container::setBackgroundColor(int red, int green, int blue) {
    // Clamp values to 0-255
    bgRed_ = (red < 0) ? 0 : (red > 255) ? 255 : red;
    bgGreen_ = (green < 0) ? 0 : (green > 255) ? 255 : green;
    bgBlue_ = (blue < 0) ? 0 : (blue > 255) ? 255 : blue;
    
    updateNativeContainerAppearance();
}

void Container::getBackgroundColor(int& red, int& green, int& blue) const {
    red = bgRed_;
    green = bgGreen_;
    blue = bgBlue_;
}

void Container::setBorderStyle(BorderStyle style) {
    if (borderStyle_ != style) {
        borderStyle_ = style;
        updateNativeContainerAppearance();
    }
}

Container::BorderStyle Container::getBorderStyle() const {
    return borderStyle_;
}

void Container::OnParentChanged(Control* oldParent, Control* newParent) {
    Control::OnParentChanged(oldParent, newParent);
    // Recreate native container with new parent
    if (nativeHandle_) {
        destroyNativeContainer();
    }
    if (newParent && newParent->getNativeHandle()) {
        createNativeContainer();
    }
}

void Container::OnBoundsChanged() {
    Control::OnBoundsChanged();
    updateNativeContainerBounds();
    
    // Update layout if one is set
    if (layout_) {
        layout_->updateLayout();
    }
}

void Container::setLayout(Layout* layout) {
    if (layout_ == layout) {
        return;
    }
    
    // Remove old layout's container reference
    if (layout_) {
        Layout* oldLayout = layout_;
        layout_ = nullptr;
        // Don't call setContainer(nullptr) on oldLayout to avoid recursion
        // The layout will handle cleanup in its destructor
    }
    
    layout_ = layout;
    
    // Set this container on the layout
    if (layout_) {
        layout_->setContainer(this);
    }
}

void Container::OnVisibleChanged() {
    Control::OnVisibleChanged();
    if (nativeHandle_) {
        if (GetVisible()) {
            platform::showContainer(nativeHandle_);
        } else {
            platform::hideContainer(nativeHandle_);
        }
    }
}

void Container::updateNativeContainerBounds() {
    if (nativeHandle_) {
        platform::resizeContainer(nativeHandle_, GetLeft(), GetTop(), GetWidth(), GetHeight());
    }
}

void Container::updateNativeContainerAppearance() {
    if (nativeHandle_) {
        platform::setContainerBackgroundColor(nativeHandle_, bgRed_, bgGreen_, bgBlue_);
        platform::setContainerBorderStyle(nativeHandle_, static_cast<int>(borderStyle_));
    }
}
