# CrossDev Examples

This directory contains example applications demonstrating the CrossDev component and layout systems.

## Examples

### layout_demo.cpp

A comprehensive demonstration of the component system and layout features:

**Features Demonstrated:**
- ✅ Component ownership (automatic cleanup)
- ✅ Parent-based visual hierarchy
- ✅ VerticalLayout (arranges controls vertically)
- ✅ HorizontalLayout (arranges controls horizontally)
- ✅ Nested layouts (containers within containers)
- ✅ Component naming and lookup
- ✅ Component enumeration
- ✅ Owner and Parent separation
- ✅ Layout spacing and margins
- ✅ Button callbacks
- ✅ InputField integration with layouts

**How to Build and Run:**

```bash
cd build
cmake ..
make layout_demo

# Run the demo (requires platform libraries)
./examples/layout_demo
```

**Note:** The demo requires full platform libraries. If you encounter build errors due to sandbox restrictions, you can:
1. Build outside the sandboxed environment
2. Or review the code to understand the API usage

**What You'll See:**

1. **Vertical Layout Panel** (left side)
   - 4 buttons arranged vertically
   - 10px spacing between buttons
   - 10px margins around the panel

2. **Horizontal Layout Panel** (top right)
   - 3 buttons (OK, Cancel, Apply) arranged horizontally
   - 5px spacing between buttons

3. **Nested Layouts** (right side)
   - Outer container with vertical layout
   - Inner container with horizontal layout
   - Demonstrates containers within containers

4. **Form Panel** (bottom)
   - Name input field
   - Email input field
   - Submit button
   - All arranged vertically with automatic sizing

5. **Component Lookup**
   - Demonstrates finding components by name
   - Shows component enumeration

**Key Concepts:**

- **Owner**: Determines memory management. When owner is destroyed, all owned components are automatically destroyed.
- **Parent**: Determines visual hierarchy. Controls are displayed within their parent.
- **Layout**: Automatically positions and sizes controls within a container.
- **Naming**: Components can have names for easy lookup.

### component_demo.cpp

A simpler example focusing on the basic component ownership and parent relationships.

## Code Structure

```cpp
// Create window (top-level)
Window window(nullptr, nullptr, 100, 100, 800, 600, "Demo");

// Create container (owned by window, parented to window)
Container* panel = new Container(&window, &window, 10, 10, 300, 200);

// Create layout (owned by window)
VerticalLayout* layout = new VerticalLayout(&window);
layout->setSpacing(10);
layout->setMargins(5, 5, 5, 5);

// Create buttons (owned by window, parented to panel)
Button* btn1 = new Button(&window, panel, 0, 0, 0, 0, "Button 1");
Button* btn2 = new Button(&window, panel, 0, 0, 0, 0, "Button 2");

// Add buttons to layout
layout->addControl(btn1);
layout->addControl(btn2);

// Set layout on container
panel->setLayout(layout);

// All components automatically cleaned up when window is destroyed!
```

## Notes

- All components are automatically cleaned up when their owner is destroyed
- Layouts automatically recalculate when container size changes
- Components can be found by name using `FindComponent()`
- Owner and Parent can be different for flexible memory and visual management
