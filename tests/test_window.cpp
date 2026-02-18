#include "../include/window.h"
#include <iostream>
#include <cassert>

// Test Window inherits from Control
void test_window_inheritance() {
    std::cout << "Test: Window inheritance...\n";
    
    // Window should inherit from Control
    Window* win = new Window(nullptr, nullptr, 10, 20, 300, 400, "Test Window");
    
    // Should be able to use Control methods
    assert(win->GetLeft() == 10);
    assert(win->GetTop() == 20);
    assert(win->GetWidth() == 300);
    assert(win->GetHeight() == 400);
    
    // Should be able to use Component methods
    assert(win->GetOwner() == nullptr);
    assert(win->GetParent() == nullptr);
    
    // Should be able to set bounds
    win->SetBounds(50, 60, 500, 600);
    assert(win->GetLeft() == 50);
    assert(win->GetTop() == 60);
    assert(win->GetWidth() == 500);
    assert(win->GetHeight() == 600);
    
    delete win;
    
    std::cout << "✓ Window inheritance test passed\n\n";
}

// Test Window can own components
void test_window_ownership() {
    std::cout << "Test: Window ownership...\n";
    
    Window window(nullptr, nullptr, 0, 0, 100, 100, "Owner Test");
    Component* child = new Component(&window);
    
    assert(window.GetComponentCount() == 1);
    assert(child->GetOwner() == &window);
    
    // Window destroyed, child should be auto-deleted
    // (tested by not crashing)
    
    std::cout << "✓ Window ownership test passed\n\n";
}

int main() {
    std::cout << "=== Window Refactoring Tests ===\n\n";
    
    try {
        test_window_inheritance();
        test_window_ownership();
        
        std::cout << "=== All tests passed! ===\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1;
    }
}
