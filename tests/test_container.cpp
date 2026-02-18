#include "../include/container.h"
#include "../include/window.h"
#include "../include/button.h"
#include <iostream>
#include <cassert>

// Test Container inherits from Control
void test_container_inheritance() {
    std::cout << "Test: Container inheritance...\n";
    
    Window window(nullptr, nullptr, 0, 0, 400, 300, "Test");
    Container* panel = new Container(&window, &window, 10, 20, 200, 150);
    
    // Should be able to use Control methods
    assert(panel->GetLeft() == 10);
    assert(panel->GetTop() == 20);
    assert(panel->GetWidth() == 200);
    assert(panel->GetHeight() == 150);
    
    // Should be able to use Component methods
    assert(panel->GetOwner() == &window);
    assert(panel->GetParent() == &window);
    
    delete panel;
    
    std::cout << "✓ Container inheritance test passed\n\n";
}

// Test Container can parent other controls
void test_container_parenting() {
    std::cout << "Test: Container parenting...\n";
    
    Window window(nullptr, nullptr, 0, 0, 400, 300, "Test");
    Container panel(&window, &window, 10, 10, 300, 200);
    
    // Create button with Container as parent
    Button btn(&window, &panel, 5, 5, 100, 30, "Click Me");
    
    assert(btn.GetParent() == &panel);
    assert(panel.GetControlCount() == 1);
    assert(panel.GetControl(0) == &btn);
    
    std::cout << "✓ Container parenting test passed\n\n";
}

// Test Container background color
void test_container_background() {
    std::cout << "Test: Container background color...\n";
    
    Window window(nullptr, nullptr, 0, 0, 400, 300, "Test");
    Container panel(&window, &window, 10, 10, 200, 150);
    
    int r, g, b;
    panel.getBackgroundColor(r, g, b);
    assert(r == 255 && g == 255 && b == 255); // Default white
    
    panel.setBackgroundColor(200, 150, 100);
    panel.getBackgroundColor(r, g, b);
    assert(r == 200 && g == 150 && b == 100);
    
    std::cout << "✓ Container background color test passed\n\n";
}

// Test Container border style
void test_container_border() {
    std::cout << "Test: Container border style...\n";
    
    Window window(nullptr, nullptr, 0, 0, 400, 300, "Test");
    Container panel(&window, &window, 10, 10, 200, 150);
    
    assert(panel.getBorderStyle() == Container::BorderNone);
    
    panel.setBorderStyle(Container::BorderSingle);
    assert(panel.getBorderStyle() == Container::BorderSingle);
    
    panel.setBorderStyle(Container::BorderDouble);
    assert(panel.getBorderStyle() == Container::BorderDouble);
    
    std::cout << "✓ Container border style test passed\n\n";
}

int main() {
    std::cout << "=== Container Class Tests ===\n\n";
    
    try {
        test_container_inheritance();
        test_container_parenting();
        test_container_background();
        test_container_border();
        
        std::cout << "=== All tests passed! ===\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1;
    }
}
