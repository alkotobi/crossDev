// Example demonstrating Delphi-style Owner/Parent component system
#include "../include/window.h"
#include "../include/button.h"
#include "../include/input_field.h"
#include "../include/container.h"
#include "../include/application.h"
#include <iostream>

int main() {
    try {
        // Create main window
        // Window(owner, parent, x, y, width, height, title)
        // For top-level window, owner and parent are nullptr
        Window window(nullptr, nullptr, 100, 100, 800, 600, "Component System Demo");
        
        // Create a container panel
        // Container(owner, parent, x, y, width, height)
        // Window owns the container (automatic cleanup)
        // Window is parent of container (visual hierarchy)
        Container panel(&window, &window, 10, 10, 300, 200);
        panel.setBackgroundColor(240, 240, 240); // Light gray
        panel.setBorderStyle(Container::BorderSingle);
        
        // Create buttons inside the container
        // Button(owner, parent, x, y, width, height, label)
        // Window owns buttons (automatic cleanup)
        // Container is parent of buttons (visual hierarchy - buttons appear in container)
        Button btn1(&window, &panel, 10, 10, 100, 30, "Button 1");
        Button btn2(&window, &panel, 10, 50, 100, 30, "Button 2");
        Button btn3(&window, &panel, 10, 90, 100, 30, "Button 3");
        
        // Create input field in the container
        InputField input(&window, &panel, 10, 130, 200, 30, "Enter text here");
        
        // Set button callbacks
        btn1.setCallback([&window](Control* parent) {
            std::cout << "Button 1 clicked! Parent: " << (parent ? "Container" : "None") << std::endl;
            input.setText("Button 1 was clicked");
        });
        
        btn2.setCallback([&window](Control* parent) {
            std::cout << "Button 2 clicked! Parent: " << (parent ? "Container" : "None") << std::endl;
            input.setText("Button 2 was clicked");
        });
        
        // Demonstrate component lookup by name
        btn1.SetName("btn1");
        btn2.SetName("btn2");
        btn3.SetName("btn3");
        input.SetName("input1");
        panel.SetName("panel1");
        
        // Find component by name
        Component* found = window.FindComponent("btn1");
        if (found) {
            std::cout << "Found component: " << found->GetName() << std::endl;
        }
        
        // Demonstrate component enumeration
        std::cout << "Window owns " << window.GetComponentCount() << " components:" << std::endl;
        for (int i = 0; i < window.GetComponentCount(); i++) {
            Component* comp = window.GetComponent(i);
            if (comp) {
                std::cout << "  - " << comp->GetName() << " (type: " << typeid(*comp).name() << ")" << std::endl;
            }
        }
        
        // Demonstrate control enumeration
        std::cout << "Window has " << window.GetControlCount() << " child controls:" << std::endl;
        for (int i = 0; i < window.GetControlCount(); i++) {
            Control* ctrl = window.GetControl(i);
            if (ctrl) {
                std::cout << "  - " << ctrl->GetName() << " at (" 
                         << ctrl->GetLeft() << ", " << ctrl->GetTop() << ")" << std::endl;
            }
        }
        
        std::cout << "Panel has " << panel.GetControlCount() << " child controls:" << std::endl;
        for (int i = 0; i < panel.GetControlCount(); i++) {
            Control* ctrl = panel.GetControl(i);
            if (ctrl) {
                std::cout << "  - " << ctrl->GetName() << " at (" 
                         << ctrl->GetLeft() << ", " << ctrl->GetTop() << ")" << std::endl;
            }
        }
        
        // Show the window
        window.show();
        
        std::cout << "\n=== Component System Demo ===" << std::endl;
        std::cout << "Window owns: panel, btn1, btn2, btn3, input (automatic cleanup)" << std::endl;
        std::cout << "Window parents: panel (visual hierarchy)" << std::endl;
        std::cout << "Panel parents: btn1, btn2, btn3, input (visual hierarchy)" << std::endl;
        std::cout << "\nWhen window is destroyed, all owned components are automatically cleaned up." << std::endl;
        std::cout << "Run the application to see the UI..." << std::endl;
        
        // Run the application event loop
        Application::getInstance().run();
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
