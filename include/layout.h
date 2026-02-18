#ifndef LAYOUT_H
#define LAYOUT_H

#include "component.h"
#include <vector>

// Forward declarations
class Control;
class Container;

// Size hint structure for controls
struct SizeHint {
    int minWidth;
    int minHeight;
    int preferredWidth;
    int preferredHeight;
    int maxWidth;
    int maxHeight;
    
    SizeHint() 
        : minWidth(0), minHeight(0),
          preferredWidth(100), preferredHeight(30),
          maxWidth(10000), maxHeight(10000) {}
    
    SizeHint(int prefW, int prefH)
        : minWidth(0), minHeight(0),
          preferredWidth(prefW), preferredHeight(prefH),
          maxWidth(10000), maxHeight(10000) {}
};

// Base class for all layout managers (similar to QLayout in Qt)
// Layouts manage the positioning and sizing of controls within a container
class Layout : public Component {
public:
    Layout(Component* owner = nullptr);
    virtual ~Layout();
    
    // Non-copyable, movable
    Layout(const Layout&) = delete;
    Layout& operator=(const Layout&) = delete;
    Layout(Layout&&) noexcept;
    Layout& operator=(Layout&&) noexcept;
    
    // Set the container this layout manages
    void setContainer(Container* container);
    Container* getContainer() const { return container_; }
    
    // Add a control to the layout
    void addControl(Control* control);
    
    // Remove a control from the layout
    void removeControl(Control* control);
    
    // Get all controls in this layout
    const std::vector<Control*>& getControls() const { return controls_; }
    
    // Get number of controls
    int getControlCount() const { return static_cast<int>(controls_.size()); }
    
    // Spacing between controls
    void setSpacing(int spacing);
    int getSpacing() const { return spacing_; }
    
    // Margins (left, top, right, bottom)
    void setMargins(int left, int top, int right, int bottom);
    void getMargins(int& left, int& top, int& right, int& bottom) const;
    
    // Update the layout (called when container size changes or controls added/removed)
    void updateLayout();
    
protected:
    // Protected access for derived classes
    std::vector<Control*>& getControlsMutable() { return controls_; }
    
    // Calculate preferred size for this layout (must be implemented by derived classes)
    virtual SizeHint calculatePreferredSize() const = 0;
    
    // Perform the actual layout (position and size all controls) (must be implemented by derived classes)
    virtual void doLayout(int x, int y, int width, int height) = 0;
    
protected:
    // Called when layout needs to be invalidated
    void invalidateLayout();
    
    // Helper to get size hint from a control
    SizeHint getControlSizeHint(Control* control) const;
    
protected:
    Container* container_;  // Protected so derived classes can access
    int spacing_;  // Protected so derived classes can access
    int marginLeft_, marginTop_, marginRight_, marginBottom_;  // Protected so derived classes can access
    
private:
    std::vector<Control*> controls_;
    bool needsUpdate_;
    
    void onContainerBoundsChanged();
};

#endif // LAYOUT_H
