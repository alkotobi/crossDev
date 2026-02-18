#ifndef HORIZONTAL_LAYOUT_H
#define HORIZONTAL_LAYOUT_H

#include "layout.h"

// Horizontal layout manager - arranges controls horizontally (like QHBoxLayout)
class HorizontalLayout : public Layout {
public:
    HorizontalLayout(Component* owner = nullptr);
    ~HorizontalLayout() override = default;
    
    // Calculate preferred size (sum of all control widths + spacing + margins)
    SizeHint calculatePreferredSize() const override;
    
    // Perform horizontal layout
    void doLayout(int x, int y, int width, int height) override;
};

#endif // HORIZONTAL_LAYOUT_H
