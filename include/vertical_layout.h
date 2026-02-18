#ifndef VERTICAL_LAYOUT_H
#define VERTICAL_LAYOUT_H

#include "layout.h"

// Vertical layout manager - arranges controls vertically (like QVBoxLayout)
class VerticalLayout : public Layout {
public:
    VerticalLayout(Component* owner = nullptr);
    ~VerticalLayout() override = default;
    
    // Calculate preferred size (sum of all control heights + spacing + margins)
    SizeHint calculatePreferredSize() const override;
    
    // Perform vertical layout
    void doLayout(int x, int y, int width, int height) override;
};

#endif // VERTICAL_LAYOUT_H
