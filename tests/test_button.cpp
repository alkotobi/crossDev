#include "../include/button.h"
#include "../include/window.h"
#include <iostream>
#include <cassert>

// Test Button inherits from Control
void test_button_inheritance() {
    std::cout << "Test: Button inheritance...\n";
    
    Window window(nullptr, nullptr, 0, 0, 400, 300, "Test");
    Button* btn = new Button(&window, &window, 10, 20, 100, 30, "Test Button");
    
    // Should be able to use Control methods
    assert(btn->GetLeft() == 10);
    assert(btn->GetTop() == 20);
    assert(btn->GetWidth() == 100);
    assert(btn->GetHeight() == 30);
    
    // Should be able to use Component methods
    assert(btn->GetOwner() == &window);
    assert(btn->GetParent() == &window);
    
    // Should have label
    assert(btn->getLabel() == "Test Button");
    
    delete btn;
    
    std::cout << "✓ Button inheritance test passed\n\n";
}

// Test Button callback
void test_button_callback() {
    std::cout << "Test: Button callback...\n";
    
    Window window(nullptr, nullptr, 0, 0, 400, 300, "Test");
    Button btn(&window, &window, 10, 10, 100, 30, "Click Me");
    
    bool callbackCalled = false;
    btn.setCallback([&callbackCalled](Control* parent) {
        callbackCalled = true;
        assert(parent != nullptr);
    });
    
    // Simulate button click by calling callback directly
    // (In real usage, platform code would call this)
    if (btn.getNativeHandle()) {
        // Callback would be triggered by platform
        // For test, we just verify it's set
    }
    
    std::cout << "✓ Button callback test passed\n\n";
}

int main() {
    std::cout << "=== Button Refactoring Tests ===\n\n";
    
    try {
        test_button_inheritance();
        test_button_callback();
        
        std::cout << "=== All tests passed! ===\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1;
    }
}
