#include "../include/control.h"
#include <iostream>
#include <cassert>
#include <stdexcept>

// Test control class
class TestControl : public Control {
public:
    TestControl(Component* owner = nullptr, Control* parent = nullptr)
        : Control(owner, parent) {}
    
    int boundsChangedCount = 0;
    int parentChangedCount = 0;
    
protected:
    void OnBoundsChanged() override {
        boundsChangedCount++;
        Control::OnBoundsChanged();
    }
    
    void OnParentChanged(Control* oldParent, Control* newParent) override {
        parentChangedCount++;
        Control::OnParentChanged(oldParent, newParent);
    }
};

// Test 1: Basic parent relationship
void test_basic_parent() {
    std::cout << "Test 1: Basic parent relationship...\n";
    
    Control parent;
    TestControl child(nullptr, &parent);
    
    assert(child.GetParent() == &parent);
    assert(parent.GetControlCount() == 1);
    assert(parent.GetControl(0) == &child);
    
    std::cout << "✓ Test 1 passed\n\n";
}

// Test 2: Position and size
void test_position_size() {
    std::cout << "Test 2: Position and size...\n";
    
    TestControl ctrl;
    
    ctrl.SetLeft(10);
    ctrl.SetTop(20);
    ctrl.SetWidth(150);  // Changed from default 100
    ctrl.SetHeight(200);
    
    assert(ctrl.GetLeft() == 10);
    assert(ctrl.GetTop() == 20);
    assert(ctrl.GetWidth() == 150);
    assert(ctrl.GetHeight() == 200);
    assert(ctrl.boundsChangedCount == 4);
    
    ctrl.SetBounds(5, 15, 50, 75);
    assert(ctrl.GetLeft() == 5);
    assert(ctrl.GetTop() == 15);
    assert(ctrl.GetWidth() == 50);
    assert(ctrl.GetHeight() == 75);
    assert(ctrl.boundsChangedCount == 5);
    
    std::cout << "✓ Test 2 passed\n\n";
}

// Test 3: Visibility
void test_visibility() {
    std::cout << "Test 3: Visibility...\n";
    
    Control ctrl;
    
    assert(ctrl.GetVisible() == true); // Default
    
    ctrl.SetVisible(false);
    assert(ctrl.GetVisible() == false);
    
    ctrl.SetVisible(true);
    assert(ctrl.GetVisible() == true);
    
    std::cout << "✓ Test 3 passed\n\n";
}

// Test 4: Parent change
void test_parent_change() {
    std::cout << "Test 4: Parent change...\n";
    
    Control parent1;
    Control parent2;
    TestControl child(nullptr, &parent1);
    
    assert(child.GetParent() == &parent1);
    assert(parent1.GetControlCount() == 1);
    assert(parent2.GetControlCount() == 0);
    int initialCount = child.parentChangedCount; // Should be 1 from constructor
    
    child.SetParent(&parent2);
    
    assert(child.GetParent() == &parent2);
    assert(parent1.GetControlCount() == 0);
    assert(parent2.GetControlCount() == 1);
    assert(child.parentChangedCount == initialCount + 1); // Incremented by 1
    
    std::cout << "✓ Test 4 passed\n\n";
}

// Test 5: Circular parent prevention
void test_circular_parent() {
    std::cout << "Test 5: Circular parent prevention...\n";
    
    Control parent;
    Control child(nullptr, &parent);
    Control grandchild(nullptr, &child);
    
    try {
        parent.SetParent(&grandchild); // Should throw (circular)
        assert(false && "Should have thrown exception");
    } catch (const std::runtime_error& e) {
        assert(std::string(e.what()).find("Circular") != std::string::npos);
    }
    
    try {
        parent.SetParent(&parent); // Should throw (self-parent)
        assert(false && "Should have thrown exception");
    } catch (const std::runtime_error& e) {
        assert(std::string(e.what()).find("itself") != std::string::npos);
    }
    
    std::cout << "✓ Test 5 passed\n\n";
}

// Test 6: Root control
void test_root_control() {
    std::cout << "Test 6: Root control...\n";
    
    Control root;
    Control child1(nullptr, &root);
    Control child2(nullptr, &child1);
    Control child3(nullptr, &child2);
    
    assert(root.GetRootControl() == &root);
    assert(child1.GetRootControl() == &root);
    assert(child2.GetRootControl() == &root);
    assert(child3.GetRootControl() == &root);
    
    std::cout << "✓ Test 6 passed\n\n";
}

// Test 7: IsChildOf
void test_is_child_of() {
    std::cout << "Test 7: IsChildOf...\n";
    
    Control root;
    Control child1(nullptr, &root);
    Control child2(nullptr, &child1);
    
    assert(child1.IsChildOf(&root) == true);
    assert(child2.IsChildOf(&root) == true);
    assert(child2.IsChildOf(&child1) == true);
    assert(root.IsChildOf(&child1) == false);
    
    std::cout << "✓ Test 7 passed\n\n";
}

// Test 8: Owner and Parent can be different
void test_owner_parent_different() {
    std::cout << "Test 8: Owner and Parent can be different...\n";
    
    Component owner;
    Control parent;
    Control child(&owner, &parent);
    
    assert(child.GetOwner() == &owner);
    assert(child.GetParent() == &parent);
    assert(owner.GetComponentCount() == 1);
    assert(parent.GetControlCount() == 1);
    
    std::cout << "✓ Test 8 passed\n\n";
}

int main() {
    std::cout << "=== Control Class Tests ===\n\n";
    
    try {
        test_basic_parent();
        test_position_size();
        test_visibility();
        test_parent_change();
        test_circular_parent();
        test_root_control();
        test_is_child_of();
        test_owner_parent_different();
        
        std::cout << "=== All tests passed! ===\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1;
    }
}
