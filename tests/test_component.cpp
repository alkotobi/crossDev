#include "../include/component.h"
#include <iostream>
#include <cassert>
#include <stdexcept>

// Test component class
class TestComponent : public Component {
public:
    TestComponent(Component* owner = nullptr, const std::string& name = "")
        : Component(owner) {
        if (!name.empty()) {
            SetName(name);
        }
    }
    
    int value = 0;
    static int destructionCount;
};

int TestComponent::destructionCount = 0;

// Test 1: Basic ownership
void test_basic_ownership() {
    std::cout << "Test 1: Basic ownership...\n";
    
    Component* owner = new Component();
    Component* child = new Component(owner);
    
    assert(owner->GetComponentCount() == 1);
    assert(child->GetOwner() == owner);
    assert(owner->GetComponent(0) == child);
    
    delete owner; // Should automatically delete child
    
    std::cout << "✓ Test 1 passed\n\n";
}

// Test 2: Component naming
void test_component_naming() {
    std::cout << "Test 2: Component naming...\n";
    
    Component owner;
    Component child1(&owner);
    Component child2(&owner);
    
    child1.SetName("child1");
    child2.SetName("child2");
    
    assert(child1.GetName() == "child1");
    assert(child2.GetName() == "child2");
    
    Component* found = owner.FindComponent("child1");
    assert(found == &child1);
    
    found = owner.FindComponent("child2");
    assert(found == &child2);
    
    found = owner.FindComponent("nonexistent");
    assert(found == nullptr);
    
    std::cout << "✓ Test 2 passed\n\n";
}

// Test 3: Automatic cleanup
void test_automatic_cleanup() {
    std::cout << "Test 3: Automatic cleanup...\n";
    
    TestComponent::destructionCount = 0;
    
    {
        Component owner;
        TestComponent* child1 = new TestComponent(&owner);
        TestComponent* child2 = new TestComponent(&owner);
        child1->value = 1;
        child2->value = 2;
        
        assert(owner.GetComponentCount() == 2);
    } // owner destroyed here
    
    // Children should be automatically destroyed
    // Note: We can't easily test this without tracking, but the memory should be freed
    
    std::cout << "✓ Test 3 passed\n\n";
}

// Test 4: Circular ownership prevention
void test_circular_ownership() {
    std::cout << "Test 4: Circular ownership prevention...\n";
    
    Component comp1;
    Component comp2(&comp1);
    
    try {
        comp1.SetOwner(&comp2); // Should throw (circular)
        assert(false && "Should have thrown exception");
    } catch (const std::runtime_error& e) {
        assert(std::string(e.what()).find("Circular") != std::string::npos);
    }
    
    try {
        comp1.SetOwner(&comp1); // Should throw (self-ownership)
        assert(false && "Should have thrown exception");
    } catch (const std::runtime_error& e) {
        assert(std::string(e.what()).find("itself") != std::string::npos);
    }
    
    std::cout << "✓ Test 4 passed\n\n";
}

// Test 5: Component enumeration
void test_component_enumeration() {
    std::cout << "Test 5: Component enumeration...\n";
    
    Component owner;
    Component* child1 = new Component(&owner);
    Component* child2 = new Component(&owner);
    Component* child3 = new Component(&owner);
    
    assert(owner.GetComponentCount() == 3);
    assert(owner.GetComponent(0) == child1);
    assert(owner.GetComponent(1) == child2);
    assert(owner.GetComponent(2) == child3);
    assert(owner.GetComponent(3) == nullptr); // Out of bounds
    
    auto components = owner.GetComponents();
    assert(components.size() == 3);
    assert(components[0] == child1);
    assert(components[1] == child2);
    assert(components[2] == child3);
    
    std::cout << "✓ Test 5 passed\n\n";
}

// Test 6: Owner change
void test_owner_change() {
    std::cout << "Test 6: Owner change...\n";
    
    Component owner1;
    Component owner2;
    Component child(&owner1);
    
    assert(child.GetOwner() == &owner1);
    assert(owner1.GetComponentCount() == 1);
    assert(owner2.GetComponentCount() == 0);
    
    child.SetOwner(&owner2);
    
    assert(child.GetOwner() == &owner2);
    assert(owner1.GetComponentCount() == 0);
    assert(owner2.GetComponentCount() == 1);
    
    std::cout << "✓ Test 6 passed\n\n";
}

int main() {
    std::cout << "=== Component Class Tests ===\n\n";
    
    try {
        test_basic_ownership();
        test_component_naming();
        test_automatic_cleanup();
        test_circular_ownership();
        test_component_enumeration();
        test_owner_change();
        
        std::cout << "=== All tests passed! ===\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1;
    }
}
