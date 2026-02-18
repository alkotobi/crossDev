#include "../include/layout.h"
#include "../include/container.h"
#include "../include/control.h"
#include <algorithm>

Layout::Layout(Component* owner)
    : Component(owner),
      container_(nullptr),
      spacing_(5),
      marginLeft_(0), marginTop_(0), marginRight_(0), marginBottom_(0),
      needsUpdate_(true) {
    resetDefaultNameCache();
}

Layout::~Layout() {
    // Remove layout from container if set
    if (container_) {
        container_->setLayout(nullptr);
    }
}

Layout::Layout(Layout&& other) noexcept
    : Component(std::move(other)),
      container_(other.container_),
      controls_(std::move(other.controls_)),
      spacing_(other.spacing_),
      marginLeft_(other.marginLeft_),
      marginTop_(other.marginTop_),
      marginRight_(other.marginRight_),
      marginBottom_(other.marginBottom_),
      needsUpdate_(other.needsUpdate_) {
    other.container_ = nullptr;
    other.controls_.clear();
    
    // Update container's layout pointer
    if (container_) {
        container_->setLayout(this);
    }
}

Layout& Layout::operator=(Layout&& other) noexcept {
    if (this != &other) {
        // Remove from old container
        if (container_) {
            container_->setLayout(nullptr);
        }
        
        Component::operator=(std::move(other));
        container_ = other.container_;
        controls_ = std::move(other.controls_);
        spacing_ = other.spacing_;
        marginLeft_ = other.marginLeft_;
        marginTop_ = other.marginTop_;
        marginRight_ = other.marginRight_;
        marginBottom_ = other.marginBottom_;
        needsUpdate_ = other.needsUpdate_;
        
        other.container_ = nullptr;
        other.controls_.clear();
        
        // Update container's layout pointer
        if (container_) {
            container_->setLayout(this);
        }
    }
    return *this;
}

void Layout::setContainer(Container* container) {
    if (container_ == container) {
        return;
    }
    
    // Remove from old container
    if (container_ && container_->getLayout() == this) {
        container_->setLayout(nullptr);
    }
    
    container_ = container;
    
    // Set this layout on the container
    if (container_) {
        container_->setLayout(this);
        invalidateLayout();
    }
}

void Layout::addControl(Control* control) {
    if (!control) {
        return;
    }
    
    // Check if already added
    auto it = std::find(controls_.begin(), controls_.end(), control);
    if (it != controls_.end()) {
        return;
    }
    
    controls_.push_back(control);
    invalidateLayout();
}

void Layout::removeControl(Control* control) {
    if (!control) {
        return;
    }
    
    auto it = std::find(controls_.begin(), controls_.end(), control);
    if (it != controls_.end()) {
        controls_.erase(it);
        invalidateLayout();
    }
}

void Layout::setSpacing(int spacing) {
    if (spacing < 0) {
        spacing = 0;
    }
    if (spacing_ != spacing) {
        spacing_ = spacing;
        invalidateLayout();
    }
}

void Layout::setMargins(int left, int top, int right, int bottom) {
    if (left < 0) left = 0;
    if (top < 0) top = 0;
    if (right < 0) right = 0;
    if (bottom < 0) bottom = 0;
    
    if (marginLeft_ != left || marginTop_ != top || 
        marginRight_ != right || marginBottom_ != bottom) {
        marginLeft_ = left;
        marginTop_ = top;
        marginRight_ = right;
        marginBottom_ = bottom;
        invalidateLayout();
    }
}

void Layout::getMargins(int& left, int& top, int& right, int& bottom) const {
    left = marginLeft_;
    top = marginTop_;
    right = marginRight_;
    bottom = marginBottom_;
}

void Layout::updateLayout() {
    if (!container_ || !needsUpdate_) {
        return;
    }
    
    // Get container's client area (accounting for margins)
    // For layout purposes, we use (0, 0) as origin relative to container
    int containerWidth = container_->GetWidth();
    int containerHeight = container_->GetHeight();
    
    // Calculate available space (minus margins)
    // Coordinates are relative to container's top-left (0, 0)
    int availableX = marginLeft_;
    int availableY = marginTop_;
    int availableWidth = containerWidth - marginLeft_ - marginRight_;
    int availableHeight = containerHeight - marginTop_ - marginBottom_;
    
    // Perform the layout (coordinates relative to container)
    doLayout(availableX, availableY, availableWidth, availableHeight);
    
    needsUpdate_ = false;
}

void Layout::invalidateLayout() {
    needsUpdate_ = true;
    if (container_) {
        updateLayout();
    }
}

SizeHint Layout::getControlSizeHint(Control* control) const {
    if (!control) {
        return SizeHint();
    }
    return control->GetSizeHint();
}

void Layout::onContainerBoundsChanged() {
    invalidateLayout();
}
