#ifndef CONTROL_H
#define CONTROL_H

#include "component.h"
#include <vector>

// Forward declaration
class Control;

// Forward declaration of SizeHint (defined in layout.h)
struct SizeHint;

// Base class for all visual controls (equivalent to TControl in Delphi)
// Inherits from Component and adds Parent support for visual hierarchy
class Control : public Component {
public:
    Control(Component* owner = nullptr, Control* parent = nullptr);
    virtual ~Control();
    
    // Non-copyable, movable
    Control(const Control&) = delete;
    Control& operator=(const Control&) = delete;
    Control(Control&&) noexcept;
    Control& operator=(Control&&) noexcept;
    
    // Parent management
    Control* GetParent() const { return parent_; }
    void SetParent(Control* parent);
    
    // Position and size
    int GetLeft() const { return left_; }
    int GetTop() const { return top_; }
    int GetWidth() const { return width_; }
    int GetHeight() const { return height_; }
    
    void SetLeft(int left);
    void SetTop(int top);
    void SetWidth(int width);
    void SetHeight(int height);
    void SetBounds(int left, int top, int width, int height);
    
    // Visibility
    bool GetVisible() const { return visible_; }
    void SetVisible(bool visible);
    
    // Control enumeration (visual children)
    int GetControlCount() const;
    Control* GetControl(int index) const;
    std::vector<Control*> GetControls() const;
    
    // Get root control (topmost parent, usually a Window)
    Control* GetRootControl() const;
    
    // Check if control is a child of another control
    bool IsChildOf(const Control* parent) const;
    
    // Platform-specific handle (opaque pointer) - overridden by derived classes
    virtual void* getNativeHandle() const { return nullptr; }

    // Context menu (right-click). Shows at (x,y) in parent coordinates.
    // itemsJson: [{"id":"copy","label":"Copy"},{"id":"-"},{"id":"paste","label":"Paste"}]
    void showContextMenu(int x, int y, const std::string& itemsJson,
                        void (*itemCallback)(const std::string& itemId, void* userData),
                        void* userData = nullptr);
    
    // Size hints for layout managers
    virtual SizeHint GetSizeHint() const;
    
protected:
    // Called when parent changes
    virtual void OnParentChanged(Control* oldParent, Control* newParent);
    
    // Called when bounds change
    virtual void OnBoundsChanged();
    
    // Called when visibility changes
    virtual void OnVisibleChanged();
    
private:
    Control* parent_;
    int left_, top_, width_, height_;
    bool visible_;
    std::vector<Control*> childControls_;
    
    void addChildControl(Control* control);
    void removeChildControl(Control* control);
    void validateParent(Control* parent) const;
    void updateChildCoordinates();
};

#endif // CONTROL_H
