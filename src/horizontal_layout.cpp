#include "../include/horizontal_layout.h"
#include "../include/container.h"
#include "../include/control.h"
#include <algorithm>

HorizontalLayout::HorizontalLayout(Component* owner)
    : Layout(owner) {
    resetDefaultNameCache();
    debugLogLifecycleCreation(this, GetOwner(), nullptr);
}

SizeHint HorizontalLayout::calculatePreferredSize() const {
    SizeHint hint;
    const std::vector<Control*>& controls_ = getControls();
    
    if (controls_.empty()) {
        hint.preferredWidth = marginLeft_ + marginRight_;
        hint.preferredHeight = marginTop_ + marginBottom_;
        return hint;
    }
    
    // Calculate preferred height (max of all control heights)
    int maxHeight = 0;
    int totalWidth = 0;
    
    for (Control* control : controls_) {
        if (!control || !control->GetVisible()) {
            continue;
        }
        
        SizeHint controlHint = getControlSizeHint(control);
        if (controlHint.preferredHeight > maxHeight) {
            maxHeight = controlHint.preferredHeight;
        }
        totalWidth += controlHint.preferredWidth;
    }
    
    // Add spacing between controls
    int visibleCount = 0;
    for (Control* control : controls_) {
        if (control && control->GetVisible()) {
            visibleCount++;
        }
    }
    if (visibleCount > 1) {
        totalWidth += spacing_ * (visibleCount - 1);
    }
    
    hint.preferredWidth = marginLeft_ + totalWidth + marginRight_;
    hint.preferredHeight = marginTop_ + maxHeight + marginBottom_;
    
    return hint;
}

void HorizontalLayout::doLayout(int x, int y, int width, int height) {
    const std::vector<Control*>& controls_ = getControls();
    if (controls_.empty()) {
        return;
    }
    
    // Filter visible controls
    std::vector<Control*> visibleControls;
    for (Control* control : controls_) {
        if (control && control->GetVisible()) {
            visibleControls.push_back(control);
        }
    }
    
    if (visibleControls.empty()) {
        return;
    }
    
    // Calculate total preferred width
    int totalPreferredWidth = 0;
    for (Control* control : visibleControls) {
        SizeHint hint = getControlSizeHint(control);
        totalPreferredWidth += hint.preferredWidth;
    }
    
    // Add spacing
    if (visibleControls.size() > 1) {
        totalPreferredWidth += spacing_ * (visibleControls.size() - 1);
    }
    
    // Distribute space
    int currentX = x;
    int availableHeight = height;
    
    for (size_t i = 0; i < visibleControls.size(); ++i) {
        Control* control = visibleControls[i];
        SizeHint hint = getControlSizeHint(control);
        
        // Use preferred width, or distribute evenly if space is limited
        int controlWidth = hint.preferredWidth;
        if (totalPreferredWidth > width && i == visibleControls.size() - 1) {
            // Last control gets remaining space
            controlWidth = width - (currentX - x);
            if (controlWidth < hint.minWidth) {
                controlWidth = hint.minWidth;
            }
        }
        
        // Clamp to min/max
        if (controlWidth < hint.minWidth) {
            controlWidth = hint.minWidth;
        }
        if (controlWidth > hint.maxWidth) {
            controlWidth = hint.maxWidth;
        }
        
        // Use preferred height, clamped to available height
        int controlHeight = hint.preferredHeight;
        if (controlHeight > availableHeight) {
            controlHeight = availableHeight;
        }
        if (controlHeight < hint.minHeight) {
            controlHeight = hint.minHeight;
        }
        if (controlHeight > hint.maxHeight) {
            controlHeight = hint.maxHeight;
        }
        
        // Set control bounds (coordinates are relative to container, need to add container's position)
        int containerX = container_->GetLeft();
        int containerY = container_->GetTop();
        control->SetBounds(containerX + currentX, containerY + y, controlWidth, controlHeight);
        
        currentX += controlWidth + spacing_;
    }
}
