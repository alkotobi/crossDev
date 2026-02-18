#include "../include/component_collection.h"
#include "../include/component.h"
#include <iostream>
#include <cassert>
#include <vector>

// Test ComponentCollection class
void test_basic_operations() {
    std::cout << "Test 1: Basic operations...\n";
    
    ComponentCollection collection;
    Component comp1(nullptr);
    Component comp2(nullptr);
    Component comp3(nullptr);
    
    // Add components
    collection.Add(&comp1);
    collection.Add(&comp2);
    collection.Add(&comp3);
    
    assert(collection.Count() == 3);
    assert(collection.Get(0) == &comp1);
    assert(collection.Get(1) == &comp2);
    assert(collection.Get(2) == &comp3);
    
    std::cout << "✓ Test 1 passed\n\n";
}

void test_name_lookup() {
    std::cout << "Test 2: Name-based lookup...\n";
    
    ComponentCollection collection;
    Component comp1(nullptr);
    Component comp2(nullptr);
    Component comp3(nullptr);
    
    comp1.SetName("Component1");
    comp2.SetName("Component2");
    comp3.SetName("Component3");
    
    collection.Add(&comp1);
    collection.Add(&comp2);
    collection.Add(&comp3);
    
    assert(collection.Find("Component1") == &comp1);
    assert(collection.Find("Component2") == &comp2);
    assert(collection.Find("Component3") == &comp3);
    assert(collection.Find("NonExistent") == nullptr);
    
    assert(collection.Contains("Component1"));
    assert(collection.Contains("Component2"));
    assert(!collection.Contains("NonExistent"));
    
    std::cout << "✓ Test 2 passed\n\n";
}

void test_remove() {
    std::cout << "Test 3: Remove operations...\n";
    
    ComponentCollection collection;
    Component comp1(nullptr);
    Component comp2(nullptr);
    Component comp3(nullptr);
    
    comp1.SetName("Component1");
    comp2.SetName("Component2");
    comp3.SetName("Component3");
    
    collection.Add(&comp1);
    collection.Add(&comp2);
    collection.Add(&comp3);
    
    assert(collection.Count() == 3);
    
    // Remove middle component
    collection.Remove(&comp2);
    
    assert(collection.Count() == 2);
    assert(collection.Get(0) == &comp1);
    assert(collection.Get(1) == &comp3);
    assert(collection.Find("Component2") == nullptr);
    assert(collection.Find("Component1") == &comp1);
    assert(collection.Find("Component3") == &comp3);
    
    // Remove first component
    collection.Remove(&comp1);
    assert(collection.Count() == 1);
    assert(collection.Get(0) == &comp3);
    
    // Remove last component
    collection.Remove(&comp3);
    assert(collection.Count() == 0);
    
    std::cout << "✓ Test 3 passed\n\n";
}

void test_index_of() {
    std::cout << "Test 4: IndexOf operations...\n";
    
    ComponentCollection collection;
    Component comp1(nullptr);
    Component comp2(nullptr);
    Component comp3(nullptr);
    
    collection.Add(&comp1);
    collection.Add(&comp2);
    collection.Add(&comp3);
    
    assert(collection.IndexOf(&comp1) == 0);
    assert(collection.IndexOf(&comp2) == 1);
    assert(collection.IndexOf(&comp3) == 2);
    
    Component comp4(nullptr);
    assert(collection.IndexOf(&comp4) == static_cast<size_t>(-1));
    
    std::cout << "✓ Test 4 passed\n\n";
}

void test_contains() {
    std::cout << "Test 5: Contains operations...\n";
    
    ComponentCollection collection;
    Component comp1(nullptr);
    Component comp2(nullptr);
    
    comp1.SetName("Comp1");
    comp2.SetName("Comp2");
    
    collection.Add(&comp1);
    
    assert(collection.Contains(&comp1));
    assert(!collection.Contains(&comp2));
    assert(collection.Contains("Comp1"));
    assert(!collection.Contains("Comp2"));
    assert(!collection.Contains("NonExistent"));
    
    std::cout << "✓ Test 5 passed\n\n";
}

void test_clear() {
    std::cout << "Test 6: Clear operations...\n";
    
    ComponentCollection collection;
    Component comp1(nullptr);
    Component comp2(nullptr);
    Component comp3(nullptr);
    
    comp1.SetName("Comp1");
    comp2.SetName("Comp2");
    comp3.SetName("Comp3");
    
    collection.Add(&comp1);
    collection.Add(&comp2);
    collection.Add(&comp3);
    
    assert(collection.Count() == 3);
    
    collection.Clear();
    
    assert(collection.Count() == 0);
    assert(collection.Find("Comp1") == nullptr);
    assert(collection.Find("Comp2") == nullptr);
    assert(collection.Find("Comp3") == nullptr);
    
    std::cout << "✓ Test 6 passed\n\n";
}

void test_add_with_name() {
    std::cout << "Test 7: Add with name...\n";
    
    ComponentCollection collection;
    Component comp1(nullptr);
    
    collection.Add(&comp1, "MyComponent");
    
    assert(comp1.GetName() == "MyComponent");
    assert(collection.Find("MyComponent") == &comp1);
    
    std::cout << "✓ Test 7 passed\n\n";
}

void test_duplicate_prevention() {
    std::cout << "Test 8: Duplicate prevention...\n";
    
    ComponentCollection collection;
    Component comp1(nullptr);
    
    collection.Add(&comp1);
    collection.Add(&comp1); // Add again
    
    assert(collection.Count() == 1); // Should still be 1
    
    std::cout << "✓ Test 8 passed\n\n";
}

int main() {
    std::cout << "=== ComponentCollection Class Tests ===\n\n";
    
    try {
        test_basic_operations();
        test_name_lookup();
        test_remove();
        test_index_of();
        test_contains();
        test_clear();
        test_add_with_name();
        test_duplicate_prevention();
        
        std::cout << "=== All tests passed! ===\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1;
    }
}
