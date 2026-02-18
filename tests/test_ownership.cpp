// Comprehensive tests for Owner/Parent component system
#include "../include/component.h"
#include "../include/control.h"
#include "../include/window.h"
#include "../include/button.h"
#include "../include/container.h"
#include <iostream>
#include <cassert>
#include <vector>

// Test 1: Owner-based automatic cleanup
void test_owner_cleanup() {
    std::cout << "Test 1: Owner-based automatic cleanup...\n";
    
    {
        Window window(nullptr, nullptr, 0, 0, 400, 300, "Test");
        
        // Create components owned by window
        Button* btn1 = new Button(&window, &window, 10, 10, 100, 30, "Button 1");
        Button* btn2 = new Button(&window, &window, 10, 50, 100, 30, "Button 2");
        Container* panel = new Container(&window, &window, 120, 10, 200, 200);
        
        assert(window.GetComponentCount() == 3);
        assert(btn1->GetOwner() == &window);
        assert(btn2->GetOwner() == &window);
        assert(panel->GetOwner() == &window);
        
        // Window goes out of scope - all owned components should be automatically destroyed
        // (We can't easily test this without tracking, but memory should be freed)
    }
    
    std::cout << "✓ Test 1 passed\n\n";
}

// Test 2: Parent-based visual hierarchy
void test_parent_hierarchy() {
    std::cout << "Test 2: Parent-based visual hierarchy...\n";
    
    Window window(nullptr, nullptr, 0, 0, 400, 300, "Test");
    Container panel(&window, &window, 10, 10, 300, 200);
    
    // Buttons owned by window, but parented to panel
    Button btn1(&window, &panel, 5, 5, 100, 30, "Button 1");
    Button btn2(&window, &panel, 5, 45, 100, 30, "Button 2");
    
    // Verify parent relationships
    assert(btn1.GetParent() == &panel);
    assert(btn2.GetParent() == &panel);
    assert(panel.GetControlCount() == 2);
    assert(window.GetControlCount() == 1); // Only panel is direct child of window
    
    // Verify owner relationships
    assert(btn1.GetOwner() == &window);
    assert(btn2.GetOwner() == &window);
    assert(panel.GetOwner() == &window);
    assert(window.GetComponentCount() == 3); // panel, btn1, btn2
    
    std::cout << "✓ Test 2 passed\n\n";
}

// Test 3: Owner and Parent can be different
void test_owner_parent_different() {
    std::cout << "Test 3: Owner and Parent can be different...\n";
    
    Window window(nullptr, nullptr, 0, 0, 400, 300, "Test");
    Container panel1(&window, &window, 10, 10, 150, 200);
    Container panel2(&window, &window, 170, 10, 150, 200);
    
    // Button owned by window, but parented to panel1
    Button btn(&window, &panel1, 10, 10, 100, 30, "Button");
    
    assert(btn.GetOwner() == &window);
    assert(btn.GetParent() == &panel1);
    assert(window.GetComponentCount() == 3); // panel1, panel2, btn
    assert(panel1.GetControlCount() == 1);
    assert(panel2.GetControlCount() == 0);
    
    // Change parent (but owner stays the same)
    btn.SetParent(&panel2);
    
    assert(btn.GetOwner() == &window); // Owner unchanged
    assert(btn.GetParent() == &panel2); // Parent changed
    assert(panel1.GetControlCount() == 0);
    assert(panel2.GetControlCount() == 1);
    
    std::cout << "✓ Test 3 passed\n\n";
}

// Test 4: Nested ownership and parenting
void test_nested_hierarchy() {
    std::cout << "Test 4: Nested ownership and parenting...\n";
    
    Window window(nullptr, nullptr, 0, 0, 400, 300, "Test");
    
    // Create nested containers
    Container outerPanel(&window, &window, 10, 10, 350, 250);
    Container innerPanel(&window, &outerPanel, 10, 10, 150, 150);
    
    // Buttons in inner panel
    Button btn1(&window, &innerPanel, 5, 5, 100, 30, "Button 1");
    Button btn2(&window, &innerPanel, 5, 45, 100, 30, "Button 2");
    
    // Verify hierarchy
    assert(window.GetComponentCount() == 4); // outerPanel, innerPanel, btn1, btn2
    assert(window.GetControlCount() == 1); // outerPanel
    assert(outerPanel.GetControlCount() == 1); // innerPanel
    assert(innerPanel.GetControlCount() == 2); // btn1, btn2
    
    // Verify ownership chain
    assert(outerPanel.GetOwner() == &window);
    assert(innerPanel.GetOwner() == &window);
    assert(btn1.GetOwner() == &window);
    assert(btn2.GetOwner() == &window);
    
    // Verify parent chain
    assert(outerPanel.GetParent() == &window);
    assert(innerPanel.GetParent() == &outerPanel);
    assert(btn1.GetParent() == &innerPanel);
    assert(btn2.GetParent() == &innerPanel);
    
    std::cout << "✓ Test 4 passed\n\n";
}

// Test 5: Component lookup by name
void test_component_lookup() {
    std::cout << "Test 5: Component lookup by name...\n";
    
    Window window(nullptr, nullptr, 0, 0, 400, 300, "Test");
    Container panel(&window, &window, 10, 10, 300, 200);
    Button btn1(&window, &panel, 10, 10, 100, 30, "Button 1");
    Button btn2(&window, &panel, 10, 50, 100, 30, "Button 2");
    
    // Set names
    panel.SetName("mainPanel");
    btn1.SetName("button1");
    btn2.SetName("button2");
    
    // Find components
    Component* found = window.FindComponent("mainPanel");
    assert(found == &panel);
    
    found = window.FindComponent("button1");
    assert(found == &btn1);
    
    found = window.FindComponent("button2");
    assert(found == &btn2);
    
    found = window.FindComponent("nonexistent");
    assert(found == nullptr);
    
    std::cout << "✓ Test 5 passed\n\n";
}

// Test 6: Component enumeration
void test_component_enumeration() {
    std::cout << "Test 6: Component enumeration...\n";
    
    Window window(nullptr, nullptr, 0, 0, 400, 300, "Test");
    Container panel(&window, &window, 10, 10, 300, 200);
    Button btn1(&window, &panel, 10, 10, 100, 30, "Button 1");
    Button btn2(&window, &panel, 10, 50, 100, 30, "Button 2");
    Button btn3(&window, &window, 10, 220, 100, 30, "Button 3");
    
    // Enumerate owned components
    assert(window.GetComponentCount() == 4);
    auto components = window.GetComponents();
    assert(components.size() == 4);
    
    // Enumerate child controls
    assert(window.GetControlCount() == 2); // panel and btn3
    auto controls = window.GetControls();
    assert(controls.size() == 2);
    
    assert(panel.GetControlCount() == 2); // btn1 and btn2
    auto panelControls = panel.GetControls();
    assert(panelControls.size() == 2);
    
    std::cout << "✓ Test 6 passed\n\n";
}

int main() {
    std::cout << "=== Owner/Parent Component System Tests ===\n\n";
    
    try {
        test_owner_cleanup();
        test_parent_hierarchy();
        test_owner_parent_different();
        test_nested_hierarchy();
        test_component_lookup();
        test_component_enumeration();
        
        std::cout << "=== All tests passed! ===\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1;
    }
}
