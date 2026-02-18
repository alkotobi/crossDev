#include "../include/component.h"
#include <algorithm>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <typeinfo>
#include <cstring>

#ifndef NDEBUG
#define COMPONENT_DEBUG_LIFECYCLE 1
#endif

static const char* demangleTypeName(const char* mangled) {
#ifdef __GNUC__
    while (*mangled >= '0' && *mangled <= '9') ++mangled;
#elif defined(_MSC_VER)
    if (strncmp(mangled, "class ", 6) == 0) mangled += 6;
#endif
    return mangled;
}

void Component::debugLogLifecycleCreation(Component* self, Component* owner, Component* parent) {
#ifdef COMPONENT_DEBUG_LIFECYCLE
    std::cout << "[LIFECYCLE] CREATED name=\"" << self->GetName()
              << "\" owner=" << (owner ? ("\"" + owner->GetName() + "\"") : "(none)")
              << " parent=" << (parent ? ("\"" + parent->GetName() + "\"") : "(none)")
              << std::endl;
#else
    (void)self;
    (void)owner;
    (void)parent;
#endif
}

Component::Component(Component* owner)
    : owner_(nullptr), name_(""), destroying_(false) {
    SetOwner(owner);
}

Component::~Component() {
    destroying_ = true;

#ifdef COMPONENT_DEBUG_LIFECYCLE
    static int s_destructOrder = 0;
    int order = ++s_destructOrder;
    std::string ownerName = owner_ ? owner_->GetName() : "(none)";
    std::cout << "[LIFECYCLE] DESTROYED #" << order << " name=\"" << GetName()
              << "\" ordered_by=\"" << ownerName
              << "\" (owner destroying " << ownedComponents_.size() << " owned)" << std::endl;
    std::cout.flush();
#endif

    // Destroy all owned components (in reverse order)
    // This ensures dependencies are cleaned up properly
    while (!ownedComponents_.empty()) {
        Component* comp = ownedComponents_.back();
        ownedComponents_.pop_back();
#ifdef COMPONENT_DEBUG_LIFECYCLE
        std::string compName = comp->GetName();
        std::string triggerName = GetName();
        std::cout << "  -> DESTROYED #" << (++s_destructOrder) << " name=\"" << compName
                  << "\" ordered_by=\"" << triggerName << "\" (owner destructor)" << std::endl;
        std::cout.flush();
#endif
        delete comp;
    }

    // Remove self from owner's list
    if (owner_) {
        owner_->removeOwnedComponent(this);
    }
}

Component::Component(Component&& other) noexcept
    : owner_(other.owner_),
      name_(std::move(other.name_)),
      defaultNameCache_(std::move(other.defaultNameCache_)),
      ownedComponents_(std::move(other.ownedComponents_)),
      destroying_(other.destroying_) {
    // Update owner's list
    if (owner_) {
        owner_->removeOwnedComponent(&other);
        owner_->addOwnedComponent(this);
    }
    
    // Update owned components' owner pointers
    for (auto* comp : ownedComponents_) {
        comp->owner_ = this;
    }
    
    other.owner_ = nullptr;
    other.ownedComponents_.clear();
    other.defaultNameCache_.clear();
    other.destroying_ = false;
}

Component& Component::operator=(Component&& other) noexcept {
    if (this != &other) {
        // Clean up current state
        destroying_ = true;
        while (!ownedComponents_.empty()) {
            Component* comp = ownedComponents_.back();
            ownedComponents_.pop_back();
            delete comp;
        }
        if (owner_) {
            owner_->removeOwnedComponent(this);
        }
        
        // Move from other
        owner_ = other.owner_;
        name_ = std::move(other.name_);
        defaultNameCache_ = std::move(other.defaultNameCache_);
        ownedComponents_ = std::move(other.ownedComponents_);
        destroying_ = other.destroying_;
        
        // Update owner's list
        if (owner_) {
            owner_->removeOwnedComponent(&other);
            owner_->addOwnedComponent(this);
        }
        
        // Update owned components' owner pointers
        for (auto* comp : ownedComponents_) {
            comp->owner_ = this;
        }
        
        other.owner_ = nullptr;
        other.ownedComponents_.clear();
        other.defaultNameCache_.clear();
        other.destroying_ = false;
    }
    return *this;
}

void Component::SetOwner(Component* owner) {
    if (owner_ == owner) {
        return;
    }
    
    // Validate: can't own self or create circular ownership
    if (owner == this) {
        throw std::runtime_error("Component cannot own itself");
    }
    
    // Check for circular ownership
    Component* check = owner;
    while (check) {
        if (check == this) {
            throw std::runtime_error("Circular ownership detected");
        }
        check = check->owner_;
    }
    
    // Remove from old owner
    if (owner_) {
        owner_->removeOwnedComponent(this);
        Removed();
    }
    
    // Add to new owner
    owner_ = owner;
    if (owner_) {
        owner_->addOwnedComponent(this);
        Inserted();
    }
}

const std::string& Component::GetName() const {
    if (!name_.empty()) {
        return name_;
    }
    if (defaultNameCache_.empty()) {
        defaultNameCache_ = generateDefaultName();
    }
    return defaultNameCache_;
}

void Component::SetName(const std::string& name) {
    if (name_ == name) {
        return;
    }
    
    validateName(name);
    
    // Check if name is already used by sibling
    if (owner_) {
        Component* existing = owner_->FindComponent(name);
        if (existing && existing != this) {
            throw std::runtime_error("Component name already exists: " + name);
        }
    }
    
    name_ = name;
    defaultNameCache_.clear();
}

Component* Component::FindComponent(const std::string& name) const {
    if (name.empty()) {
        return nullptr;
    }
    
    // Search in owned components (use GetName so default names are found)
    for (Component* comp : ownedComponents_) {
        if (comp->GetName() == name) {
            return comp;
        }
        // Recursive search
        Component* found = comp->FindComponent(name);
        if (found) {
            return found;
        }
    }
    
    return nullptr;
}

int Component::GetComponentCount() const {
    return static_cast<int>(ownedComponents_.size());
}

Component* Component::GetComponent(int index) const {
    if (index < 0 || index >= static_cast<int>(ownedComponents_.size())) {
        return nullptr;
    }
    return ownedComponents_[index];
}

std::vector<Component*> Component::GetComponents() const {
    return ownedComponents_;
}

void Component::Notification(Component* component, bool inserting) {
    // Override in derived classes if needed
    (void)component;
    (void)inserting;
}

void Component::Inserted() {
    // Override in derived classes if needed
}

void Component::Removed() {
    // Override in derived classes if needed
}

void Component::addOwnedComponent(Component* component) {
    if (!component) {
        return;
    }
    
    // Check if already added
    auto it = std::find(ownedComponents_.begin(), ownedComponents_.end(), component);
    if (it != ownedComponents_.end()) {
        return;
    }
    
    ownedComponents_.push_back(component);
    Notification(component, true);
}

void Component::removeOwnedComponent(Component* component) {
    if (!component) {
        return;
    }
    
    auto it = std::find(ownedComponents_.begin(), ownedComponents_.end(), component);
    if (it != ownedComponents_.end()) {
        Notification(component, false);
        ownedComponents_.erase(it);
    }
}

void Component::validateName(const std::string& name) const {
    // Basic validation: non-empty, no special characters that might cause issues
    if (name.empty()) {
        return; // Empty name is allowed (will use default)
    }
    
    // Check for invalid characters (basic check)
    for (char c : name) {
        if (c == '\0' || c == '\n' || c == '\r') {
            throw std::runtime_error("Component name contains invalid characters");
        }
    }
}

std::string Component::generateDefaultName() const {
    // Generate default name: ClassName + serial number (e.g. Button1, WebViewWindow2)
    // Uses per-type static counter so it works during base-class construction
    // and produces human-readable names for debugging/logging
    const char* typeName = typeid(*this).name();

#ifdef __GNUC__
    while (*typeName >= '0' && *typeName <= '9') ++typeName;
#elif defined(_MSC_VER)
    if (strncmp(typeName, "class ", 6) == 0) typeName += 6;
#endif

    static std::map<std::string, int> s_typeCounters;
    std::string baseName = typeName;
    int n = ++s_typeCounters[baseName];

    std::ostringstream oss;
    oss << baseName << n;
    return oss.str();
}
