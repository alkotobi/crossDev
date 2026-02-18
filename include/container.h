#ifndef CONTAINER_H
#define CONTAINER_H

#include "control.h"
#include <string>

// Forward declaration
class Layout;

// Container class - equivalent to Panel in Delphi
// A visual container that can own and parent other controls
class Container : public Control {
public:
    // Constructor: Container(owner, parent, x, y, width, height, flipped=false)
    Container(Component* owner = nullptr, Control* parent = nullptr,
              int x = 0, int y = 0, int width = 200, int height = 200, bool flipped = false);
    ~Container();
    
    // Non-copyable, movable
    Container(const Container&) = delete;
    Container& operator=(const Container&) = delete;
    Container(Container&&) noexcept;
    Container& operator=(Container&&) noexcept;
    
    // Background color (RGB values 0-255)
    void setBackgroundColor(int red, int green, int blue);
    void getBackgroundColor(int& red, int& green, int& blue) const;
    
    // Border style
    enum BorderStyle {
        BorderNone = 0,
        BorderSingle = 1,
        BorderDouble = 2
    };
    
    void setBorderStyle(BorderStyle style);
    BorderStyle getBorderStyle() const;
    
    // Layout management
    void setLayout(Layout* layout);
    Layout* getLayout() const { return layout_; }
    
    // Platform-specific handle (opaque pointer)
    void* getNativeHandle() const override { return nativeHandle_; }
    
protected:
    // Override Control virtual methods
    void OnParentChanged(Control* oldParent, Control* newParent) override;
    void OnBoundsChanged() override;
    void OnVisibleChanged() override;
    
private:
    void* nativeHandle_;
    int bgRed_, bgGreen_, bgBlue_;
    bool flipped_;
    BorderStyle borderStyle_;
    Layout* layout_;
    
    // Platform-specific implementation
    void createNativeContainer();
    void destroyNativeContainer();
    void updateNativeContainerBounds();
    void updateNativeContainerAppearance();
};

#endif // CONTAINER_H
