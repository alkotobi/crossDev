#include "../include/control.h"
#include "../include/layout.h"
#include "platform/platform_impl.h"
#include <algorithm>
#include <stdexcept>

Control::Control(Component* owner, Control* parent)
    : Component(owner),
      parent_(nullptr),
      left_(0), top_(0), width_(100), height_(100),
      visible_(true) {
    SetParent(parent);
    resetDefaultNameCache();
}

Control::~Control() {
    // Remove self from parent's child list
    if (parent_) {
        parent_->removeChildControl(this);
    }
    
    // Remove all child controls from our list (they will handle their own cleanup)
    childControls_.clear();
}

Control::Control(Control&& other) noexcept
    : Component(std::move(other)),
      parent_(other.parent_),
      left_(other.left_),
      top_(other.top_),
      width_(other.width_),
      height_(other.height_),
      visible_(other.visible_),
      childControls_(std::move(other.childControls_)) {
    // Update parent's child list
    if (parent_) {
        parent_->removeChildControl(&other);
        parent_->addChildControl(this);
    }
    
    // Update child controls' parent pointers
    for (auto* ctrl : childControls_) {
        ctrl->parent_ = this;
    }
    
    other.parent_ = nullptr;
    other.childControls_.clear();
}

Control& Control::operator=(Control&& other) noexcept {
    if (this != &other) {
        // Remove from current parent
        if (parent_) {
            parent_->removeChildControl(this);
        }
        
        // Move Component part
        Component::operator=(std::move(other));
        
        // Move Control-specific members
        parent_ = other.parent_;
        left_ = other.left_;
        top_ = other.top_;
        width_ = other.width_;
        height_ = other.height_;
        visible_ = other.visible_;
        childControls_ = std::move(other.childControls_);
        
        // Update parent's child list
        if (parent_) {
            parent_->removeChildControl(&other);
            parent_->addChildControl(this);
        }
        
        // Update child controls' parent pointers
        for (auto* ctrl : childControls_) {
            ctrl->parent_ = this;
        }
        
        other.parent_ = nullptr;
        other.childControls_.clear();
    }
    return *this;
}

void Control::SetParent(Control* parent) {
    if (parent_ == parent) {
        return;
    }
    
    validateParent(parent);
    
    Control* oldParent = parent_;
    
    // Remove from old parent
    if (parent_) {
        parent_->removeChildControl(this);
    }
    
    // Add to new parent
    parent_ = parent;
    if (parent_) {
        parent_->addChildControl(this);
    }
    
    // Notify derived classes
    OnParentChanged(oldParent, parent_);
}

void Control::SetLeft(int left) {
    if (left_ != left) {
        left_ = left;
        OnBoundsChanged();
    }
}

void Control::SetTop(int top) {
    if (top_ != top) {
        top_ = top;
        OnBoundsChanged();
    }
}

void Control::SetWidth(int width) {
    if (width < 0) {
        throw std::runtime_error("Control width cannot be negative");
    }
    if (width_ != width) {
        width_ = width;
        OnBoundsChanged();
    }
}

void Control::SetHeight(int height) {
    if (height < 0) {
        throw std::runtime_error("Control height cannot be negative");
    }
    if (height_ != height) {
        height_ = height;
        OnBoundsChanged();
    }
}

void Control::SetBounds(int left, int top, int width, int height) {
    bool changed = (left_ != left || top_ != top || width_ != width || height_ != height);
    
    if (width < 0 || height < 0) {
        throw std::runtime_error("Control dimensions cannot be negative");
    }
    
    left_ = left;
    top_ = top;
    width_ = width;
    height_ = height;
    
    if (changed) {
        OnBoundsChanged();
    }
}

void Control::SetVisible(bool visible) {
    if (visible_ != visible) {
        visible_ = visible;
        OnVisibleChanged();
    }
}

int Control::GetControlCount() const {
    return static_cast<int>(childControls_.size());
}

Control* Control::GetControl(int index) const {
    if (index < 0 || index >= static_cast<int>(childControls_.size())) {
        return nullptr;
    }
    return childControls_[index];
}

std::vector<Control*> Control::GetControls() const {
    return childControls_;
}

Control* Control::GetRootControl() const {
    const Control* current = this;
    while (current->parent_) {
        current = current->parent_;
    }
    return const_cast<Control*>(current);
}

bool Control::IsChildOf(const Control* parent) const {
    const Control* current = this;
    while (current) {
        if (current->parent_ == parent) {
            return true;
        }
        current = current->parent_;
    }
    return false;
}

void Control::OnParentChanged(Control* oldParent, Control* newParent) {
    // Override in derived classes if needed
    (void)oldParent;
    (void)newParent;
}

void Control::OnBoundsChanged() {
    // Override in derived classes if needed
    updateChildCoordinates();
}

void Control::OnVisibleChanged() {
    // Override in derived classes if needed
}

void Control::addChildControl(Control* control) {
    if (!control) {
        return;
    }
    
    // Check if already added
    auto it = std::find(childControls_.begin(), childControls_.end(), control);
    if (it != childControls_.end()) {
        return;
    }
    
    childControls_.push_back(control);
}

void Control::removeChildControl(Control* control) {
    if (!control) {
        return;
    }
    
    auto it = std::find(childControls_.begin(), childControls_.end(), control);
    if (it != childControls_.end()) {
        childControls_.erase(it);
    }
}

void Control::validateParent(Control* parent) const {
    // Can't parent to self
    if (parent == this) {
        throw std::runtime_error("Control cannot be parent of itself");
    }
    
    // Check for circular parenting
    if (parent) {
        const Control* check = parent;
        while (check) {
            if (check == this) {
                throw std::runtime_error("Circular parent relationship detected");
            }
            check = check->parent_;
        }
    }
}

void Control::updateChildCoordinates() {
    // Update child coordinates if needed
    // This can be overridden in derived classes for layout management
}

SizeHint Control::GetSizeHint() const {
    // Default implementation: use current size as preferred
    SizeHint hint;
    hint.preferredWidth = width_;
    hint.preferredHeight = height_;
    hint.minWidth = 0;
    hint.minHeight = 0;
    hint.maxWidth = 10000;
    hint.maxHeight = 10000;
    return hint;
}

void Control::showContextMenu(int x, int y, const std::string& itemsJson,
                              void (*itemCallback)(const std::string& itemId, void* userData),
                              void* userData) {
    if (!itemCallback || itemsJson.empty()) return;
    Control* root = GetRootControl();
    void* parentHandle = root ? root->getNativeHandle() : nullptr;
    if (parentHandle) {
        platform::showContextMenu(parentHandle, x, y, itemsJson, itemCallback, userData);
    }
}
