#ifndef COMPONENT_COLLECTION_H
#define COMPONENT_COLLECTION_H

#include "component.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

// Efficient component collection with O(1) name lookup and O(1) index access
// Provides better performance than std::vector for large component hierarchies
class ComponentCollection {
public:
    ComponentCollection();
    ~ComponentCollection();
    
    // Non-copyable, movable
    ComponentCollection(const ComponentCollection&) = delete;
    ComponentCollection& operator=(const ComponentCollection&) = delete;
    ComponentCollection(ComponentCollection&&) noexcept;
    ComponentCollection& operator=(ComponentCollection&&) noexcept;
    
    // Add component (by name if provided)
    void Add(Component* component);
    void Add(Component* component, const std::string& name);
    
    // Remove component
    void Remove(Component* component);
    
    // Get component by index (O(1))
    Component* Get(size_t index) const;
    
    // Get component by name (O(1) average case)
    Component* Find(const std::string& name) const;
    
    // Get count
    size_t Count() const { return components_.size(); }
    
    // Check if contains component
    bool Contains(Component* component) const;
    bool Contains(const std::string& name) const;
    
    // Get all components (for iteration)
    const std::vector<Component*>& GetAll() const { return components_; }
    
    // Clear all components (doesn't delete them, just removes from collection)
    void Clear();
    
    // Get index of component
    size_t IndexOf(Component* component) const;
    
private:
    std::vector<Component*> components_;  // For index-based access
    std::unordered_map<std::string, Component*> nameMap_;  // For name-based lookup
    std::unordered_map<Component*, size_t> indexMap_;  // For reverse lookup (component -> index)
    
    void updateIndexMap();
};

#endif // COMPONENT_COLLECTION_H
