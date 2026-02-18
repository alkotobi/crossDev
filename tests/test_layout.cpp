#include "../include/layout.h"
#include "../include/vertical_layout.h"
#include "../include/horizontal_layout.h"
#include "../include/container.h"
#include "../include/button.h"
#include "../include/window.h"
#include "../include/control.h"
#include <iostream>
#include <cassert>

void test_vertical_layout_basic() {
    std::cout << "Test 1: VerticalLayout basic operations...\n";
    
    Window window(nullptr, nullptr, 0, 0, 800, 600, "Test");
    Container container(&window, &window, 10, 10, 300, 400);
    
    VerticalLayout layout(&window);
    layout.setSpacing(10);
    layout.setMargins(5, 5, 5, 5);
    
    Button btn1(&window, &container, 0, 0, 0, 0, "Button 1");
    Button btn2(&window, &container, 0, 0, 0, 0, "Button 2");
    Button btn3(&window, &container, 0, 0, 0, 0, "Button 3");
    
    layout.addControl(&btn1);
    layout.addControl(&btn2);
    layout.addControl(&btn3);
    
    container.setLayout(&layout);
    
    // Layout should position buttons vertically
    assert(layout.getControlCount() == 3);
    assert(layout.getSpacing() == 10);
    
    int left, top, right, bottom;
    layout.getMargins(left, top, right, bottom);
    assert(left == 5 && top == 5 && right == 5 && bottom == 5);
    
    std::cout << "✓ Test 1 passed\n\n";
}

void test_horizontal_layout_basic() {
    std::cout << "Test 2: HorizontalLayout basic operations...\n";
    
    Window window(nullptr, nullptr, 0, 0, 800, 600, "Test");
    Container container(&window, &window, 10, 10, 400, 100);
    
    HorizontalLayout layout(&window);
    layout.setSpacing(5);
    
    Button btn1(&window, &container, 0, 0, 0, 0, "OK");
    Button btn2(&window, &container, 0, 0, 0, 0, "Cancel");
    
    layout.addControl(&btn1);
    layout.addControl(&btn2);
    
    container.setLayout(&layout);
    
    assert(layout.getControlCount() == 2);
    assert(layout.getSpacing() == 5);
    
    std::cout << "✓ Test 2 passed\n\n";
}

void test_layout_add_remove() {
    std::cout << "Test 3: Layout add/remove controls...\n";
    
    Window window(nullptr, nullptr, 0, 0, 800, 600, "Test");
    Container container(&window, &window, 10, 10, 300, 400);
    
    VerticalLayout layout(&window);
    
    Button btn1(&window, &container, 0, 0, 0, 0, "Button 1");
    Button btn2(&window, &container, 0, 0, 0, 0, "Button 2");
    
    layout.addControl(&btn1);
    assert(layout.getControlCount() == 1);
    
    layout.addControl(&btn2);
    assert(layout.getControlCount() == 2);
    
    layout.removeControl(&btn1);
    assert(layout.getControlCount() == 1);
    
    layout.removeControl(&btn2);
    assert(layout.getControlCount() == 0);
    
    std::cout << "✓ Test 3 passed\n\n";
}

void test_size_hint() {
    std::cout << "Test 4: SizeHint operations...\n";
    
    Window window(nullptr, nullptr, 0, 0, 800, 600, "Test");
    Button btn(&window, &window, 10, 10, 100, 30, "Test");
    
    SizeHint hint = btn.GetSizeHint();
    assert(hint.preferredWidth == 100);
    assert(hint.preferredHeight == 30);
    assert(hint.minWidth >= 0);
    assert(hint.minHeight >= 0);
    
    std::cout << "✓ Test 4 passed\n\n";
}

void test_layout_margins() {
    std::cout << "Test 5: Layout margins...\n";
    
    Window window(nullptr, nullptr, 0, 0, 800, 600, "Test");
    VerticalLayout layout(&window);
    
    layout.setMargins(10, 20, 30, 40);
    
    int left, top, right, bottom;
    layout.getMargins(left, top, right, bottom);
    assert(left == 10);
    assert(top == 20);
    assert(right == 30);
    assert(bottom == 40);
    
    std::cout << "✓ Test 5 passed\n\n";
}

int main() {
    std::cout << "=== Layout System Tests ===\n\n";
    
    try {
        test_vertical_layout_basic();
        test_horizontal_layout_basic();
        test_layout_add_remove();
        test_size_hint();
        test_layout_margins();
        
        std::cout << "=== All tests passed! ===\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1;
    }
}
