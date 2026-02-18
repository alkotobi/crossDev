#include "../include/vertical_layout.h"
#include "../include/container.h"
#include "../include/control.h"
#include <algorithm>

VerticalLayout::VerticalLayout(Component* owner)
    : Layout(owner) {
    resetDefaultNameCache();
    debugLogLifecycleCreation(this, GetOwner(), nullptr);
}

SizeHint VerticalLayout::calculatePreferredSize() const {
    SizeHint hint;
    const std::vector<Control*>& controls_ = getControls();
    
    if (controls_.empty()) {
        hint.preferredWidth = marginLeft_ + marginRight_;
        hint.preferredHeight = marginTop_ + marginBottom_;
        return hint;
    }
    
    // Calculate preferred width (max of all control widths)
    int maxWidth = 0;
    int totalHeight = 0;
    
    for (Control* control : controls_) {
        if (!control || !control->GetVisible()) {
            continue;
        }
        
        SizeHint controlHint = getControlSizeHint(control);
        if (controlHint.preferredWidth > maxWidth) {
            maxWidth = controlHint.preferredWidth;
        }
        totalHeight += controlHint.preferredHeight;
    }
    
    // Add spacing between controls
    int visibleCount = 0;
    for (Control* control : controls_) {
        if (control && control->GetVisible()) {
            visibleCount++;
        }
    }
    if (visibleCount > 1) {
        totalHeight += spacing_ * (visibleCount - 1);
    }
    
    hint.preferredWidth = marginLeft_ + maxWidth + marginRight_;
    hint.preferredHeight = marginTop_ + totalHeight + marginBottom_;
    
    return hint;
}

void VerticalLayout::doLayout(int x, int y, int width, int height) {
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
    
    // Calculate total preferred height
    int totalPreferredHeight = 0;
    for (Control* control : visibleControls) {
        SizeHint hint = getControlSizeHint(control);
        totalPreferredHeight += hint.preferredHeight;
    }
    
    // Add spacing
    if (visibleControls.size() > 1) {
        totalPreferredHeight += spacing_ * (visibleControls.size() - 1);
    }
    
    // Distribute space
    int currentY = y;
    int availableWidth = width;
    
    for (size_t i = 0; i < visibleControls.size(); ++i) {
        Control* control = visibleControls[i];
        SizeHint hint = getControlSizeHint(control);
        
        // Use preferred height, or distribute evenly if space is limited
        int controlHeight = hint.preferredHeight;
        if (totalPreferredHeight > height && i == visibleControls.size() - 1) {
            // Last control gets remaining space
            controlHeight = height - (currentY - y);
            if (controlHeight < hint.minHeight) {
                controlHeight = hint.minHeight;
            }
        }
        
        // Clamp to min/max
        if (controlHeight < hint.minHeight) {
            controlHeight = hint.minHeight;
        }
        if (controlHeight > hint.maxHeight) {
            controlHeight = hint.maxHeight;
        }
        
        // Use preferred width, clamped to available width
        int controlWidth = hint.preferredWidth;
        if (controlWidth > availableWidth) {
            controlWidth = availableWidth;
        }
        if (controlWidth < hint.minWidth) {
            controlWidth = hint.minWidth;
        }
        if (controlWidth > hint.maxWidth) {
            controlWidth = hint.maxWidth;
        }
        
        // Set control bounds (coordinates are relative to container, need to add container's position)
        int containerX = container_->GetLeft();
        int containerY = container_->GetTop();
        control->SetBounds(containerX + x, containerY + currentY, controlWidth, controlHeight);
        
        currentY += controlHeight + spacing_;
    }
}
