#ifndef WINDOW_H
#define WINDOW_H

#include "control.h"
#include <string>
#include <memory>

// Platform-agnostic Window interface
// Window inherits from Control, so it can own and parent other components
class Window : public Control {
public:
    // Constructor: Window(owner, parent, x, y, width, height, title)
    // For top-level windows, owner and parent are typically nullptr
    Window(Component* owner = nullptr, Control* parent = nullptr, 
           int x = 0, int y = 0, int width = 800, int height = 600, 
           const std::string& title = "");
    ~Window();
    
    // Non-copyable, movable
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
    Window(Window&&) noexcept;
    Window& operator=(Window&&) noexcept;
    
    void show();
    void hide();
    std::string getTitle() const { return title_; }
    void setTitle(const std::string& title);
    bool isVisible() const;
    void maximize();

    // Main menu bar (macOS menu bar, Windows/Linux window menu).
    // menuJson: [{"id":"file","label":"File","items":[{"id":"new","label":"New"},{"id":"-"},{"id":"quit","label":"Quit"}]}]
    // Use {"id":"-"} or {"label":"-"} for separators.
    void setMainMenu(const std::string& menuJson,
                    void (*itemCallback)(const std::string& itemId, void* userData),
                    void* userData = nullptr);

    // Platform-specific handle (opaque pointer) - exposed for components
    void* getNativeHandle() const override { return nativeHandle_; }
    
protected:
    // Override Control virtual methods
    void OnParentChanged(Control* oldParent, Control* newParent) override;
    void OnBoundsChanged() override;
    void OnVisibleChanged() override;
    
private:
    void* nativeHandle_;
    std::string title_;
    
    // Platform-specific implementation
    void createNativeWindow();
    void destroyNativeWindow();
    void showNativeWindow();
    void hideNativeWindow();
    void setNativeTitle(const std::string& title);
    void updateNativeWindowBounds();
};

#endif // WINDOW_H
