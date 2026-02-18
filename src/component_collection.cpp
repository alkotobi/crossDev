#include "../include/component_collection.h"
#include <algorithm>
#include <stdexcept>

ComponentCollection::ComponentCollection() {
}

ComponentCollection::~ComponentCollection() {
    Clear();
}

ComponentCollection::ComponentCollection(ComponentCollection&& other) noexcept
    : components_(std::move(other.components_)),
      nameMap_(std::move(other.nameMap_)),
      indexMap_(std::move(other.indexMap_)) {
}

ComponentCollection& ComponentCollection::operator=(ComponentCollection&& other) noexcept {
    if (this != &other) {
        Clear();
        components_ = std::move(other.components_);
        nameMap_ = std::move(other.nameMap_);
        indexMap_ = std::move(other.indexMap_);
    }
    return *this;
}

void ComponentCollection::Add(Component* component) {
    if (!component) {
        return;
    }
    
    // Check if already added
    if (Contains(component)) {
        return;
    }
    
    // Add to vector
    size_t index = components_.size();
    components_.push_back(component);
    
    // Add to index map
    indexMap_[component] = index;
    
    // Add to name map if component has a name
    const std::string& name = component->GetName();
    if (!name.empty()) {
        // Check for name collision
        if (nameMap_.find(name) != nameMap_.end()) {
            // Name already exists, but we'll allow it (component system handles uniqueness)
            // Just update the map
        }
        nameMap_[name] = component;
    }
}

void ComponentCollection::Add(Component* component, const std::string& name) {
    if (!component) {
        return;
    }
    
    // Set name first
    if (!name.empty()) {
        component->SetName(name);
    }
    
    // Then add
    Add(component);
}

void ComponentCollection::Remove(Component* component) {
    if (!component) {
        return;
    }
    
    // Find in vector
    auto it = std::find(components_.begin(), components_.end(), component);
    if (it == components_.end()) {
        return; // Not found
    }
    
    // Remove from name map
    const std::string& name = component->GetName();
    if (!name.empty()) {
        nameMap_.erase(name);
    }
    
    // Remove from index map
    indexMap_.erase(component);
    
    // Remove from vector
    size_t removedIndex = std::distance(components_.begin(), it);
    components_.erase(it);
    
    // Update indices for components after the removed one
    updateIndexMap();
}

Component* ComponentCollection::Get(size_t index) const {
    if (index >= components_.size()) {
        return nullptr;
    }
    return components_[index];
}

Component* ComponentCollection::Find(const std::string& name) const {
    if (name.empty()) {
        return nullptr;
    }
    
    auto it = nameMap_.find(name);
    if (it != nameMap_.end()) {
        return it->second;
    }
    return nullptr;
}

bool ComponentCollection::Contains(Component* component) const {
    if (!component) {
        return false;
    }
    return indexMap_.find(component) != indexMap_.end();
}

bool ComponentCollection::Contains(const std::string& name) const {
    if (name.empty()) {
        return false;
    }
    return nameMap_.find(name) != nameMap_.end();
}

void ComponentCollection::Clear() {
    components_.clear();
    nameMap_.clear();
    indexMap_.clear();
}

size_t ComponentCollection::IndexOf(Component* component) const {
    if (!component) {
        return static_cast<size_t>(-1); // Invalid index
    }
    
    auto it = indexMap_.find(component);
    if (it != indexMap_.end()) {
        return it->second;
    }
    return static_cast<size_t>(-1); // Not found
}

void ComponentCollection::updateIndexMap() {
    indexMap_.clear();
    for (size_t i = 0; i < components_.size(); ++i) {
        indexMap_[components_[i]] = i;
    }
}
