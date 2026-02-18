#include "../include/window.h"
#include "platform/platform_impl.h"
#include <stdexcept>

Window::Window(Component* owner, Control* parent, int x, int y, int width, int height, const std::string& title)
    : Control(owner, parent), nativeHandle_(nullptr), title_(title) {
    // Set bounds using Control's methods
    SetBounds(x, y, width, height);
    createNativeWindow();
    resetDefaultNameCache();
    debugLogLifecycleCreation(this, GetOwner(), GetParent());
}

Window::~Window() {
    if (nativeHandle_) {
        destroyNativeWindow();
    }
}

Window::Window(Window&& other) noexcept
    : Control(std::move(other)),
      nativeHandle_(other.nativeHandle_),
      title_(std::move(other.title_)) {
    other.nativeHandle_ = nullptr;
}

Window& Window::operator=(Window&& other) noexcept {
    if (this != &other) {
        if (nativeHandle_) {
            destroyNativeWindow();
        }
        
        Control::operator=(std::move(other));
        nativeHandle_ = other.nativeHandle_;
        title_ = std::move(other.title_);
        
        other.nativeHandle_ = nullptr;
    }
    return *this;
}

void Window::createNativeWindow() {
    nativeHandle_ = platform::createWindow(GetLeft(), GetTop(), GetWidth(), GetHeight(), title_, this);
    if (!nativeHandle_) {
        throw std::runtime_error("Failed to create native window");
    }
}

void Window::destroyNativeWindow() {
    if (nativeHandle_) {
        platform::destroyWindow(nativeHandle_);
        nativeHandle_ = nullptr;
    }
}

void Window::show() {
    if (!nativeHandle_) {
        return;
    }
    showNativeWindow();
    SetVisible(true);
}

void Window::hide() {
    if (!nativeHandle_) {
        return;
    }
    hideNativeWindow();
    SetVisible(false);
}

void Window::showNativeWindow() {
    platform::showWindow(nativeHandle_);
}

void Window::hideNativeWindow() {
    platform::hideWindow(nativeHandle_);
}

void Window::setTitle(const std::string& title) {
    title_ = title;
    if (nativeHandle_) {
        setNativeTitle(title);
    }
}

void Window::setNativeTitle(const std::string& title) {
    platform::setWindowTitle(nativeHandle_, title);
}

bool Window::isVisible() const {
    if (!nativeHandle_) {
        return false;
    }
    return GetVisible() && platform::isWindowVisible(nativeHandle_);
}

void Window::maximize() {
    if (nativeHandle_) {
        platform::maximizeWindow(nativeHandle_);
    }
}

void Window::OnParentChanged(Control* oldParent, Control* newParent) {
    Control::OnParentChanged(oldParent, newParent);
    // Windows typically don't have parents, but if they do, we might need to update native window
    (void)oldParent;
    (void)newParent;
}

void Window::OnBoundsChanged() {
    Control::OnBoundsChanged();
    updateNativeWindowBounds();
}

void Window::OnVisibleChanged() {
    Control::OnVisibleChanged();
    if (nativeHandle_) {
        if (GetVisible()) {
            showNativeWindow();
        } else {
            hideNativeWindow();
        }
    }
}

void Window::updateNativeWindowBounds() {
    if (nativeHandle_) {
        // Update native window position and size
        // Note: Platform-specific implementations may need to be updated to support this
        // For now, we'll just recreate if needed, or platform can handle resize events
    }
}

void Window::setMainMenu(const std::string& menuJson,
                        void (*itemCallback)(const std::string& itemId, void* userData),
                        void* userData) {
    if (nativeHandle_ && itemCallback) {
        platform::setWindowMainMenu(nativeHandle_, menuJson, itemCallback, userData);
    }
}
