#include "../include/window.h"
#include "../include/button.h"
#include "../include/input_field.h"
#include "../include/container.h"
#include "../include/vertical_layout.h"
#include "../include/horizontal_layout.h"
#include "../include/component.h"
#include "../include/application.h"
#include <iostream>
#include <string>

// Example demonstrating the component system and layout features
int main() {
    std::cout << "=== CrossDev Component & Layout System Demo ===\n\n";
    
    // Create main window (top-level, no owner/parent)
    Window window(nullptr, nullptr, 100, 100, 800, 600, "Component & Layout Demo");
    window.show();
    
    // Set window name for lookup
    window.SetName("mainWindow");
    
    // ============================================
    // Example 1: Vertical Layout with Buttons
    // ============================================
    std::cout << "Example 1: Vertical Layout with Buttons\n";
    
    Container* panel1 = new Container(&window, &window, 10, 10, 200, 300);
    panel1->SetName("buttonPanel");
    panel1->setBackgroundColor(240, 240, 240);
    panel1->setBorderStyle(Container::BorderSingle);
    
    // Create vertical layout
    VerticalLayout* vLayout = new VerticalLayout(&window);
    vLayout->setSpacing(10);
    vLayout->setMargins(10, 10, 10, 10);
    
    // Create buttons (bounds will be managed by layout)
    Button* btn1 = new Button(&window, panel1, 0, 0, 0, 0, "Button 1");
    Button* btn2 = new Button(&window, panel1, 0, 0, 0, 0, "Button 2");
    Button* btn3 = new Button(&window, panel1, 0, 0, 0, 0, "Button 3");
    Button* btn4 = new Button(&window, panel1, 0, 0, 0, 0, "Button 4");
    
    // Set names for lookup
    btn1->SetName("btn1");
    btn2->SetName("btn2");
    btn3->SetName("btn3");
    btn4->SetName("btn4");
    
    // Add buttons to layout
    vLayout->addControl(btn1);
    vLayout->addControl(btn2);
    vLayout->addControl(btn3);
    vLayout->addControl(btn4);
    
    // Set layout on container
    panel1->setLayout(vLayout);
    
    // Set button callbacks
    btn1->setCallback([&window](Control* parent) {
        std::cout << "Button 1 clicked! Parent: " << parent->GetName() << "\n";
    });
    
    btn2->setCallback([&window](Control* parent) {
        std::cout << "Button 2 clicked! Parent: " << parent->GetName() << "\n";
    });
    
    std::cout << "✓ Created vertical layout with 4 buttons\n";
    std::cout << "  - Layout spacing: " << vLayout->getSpacing() << "px\n";
    std::cout << "  - Layout controls: " << vLayout->getControlCount() << "\n\n";
    
    // ============================================
    // Example 2: Horizontal Layout with Buttons
    // ============================================
    std::cout << "Example 2: Horizontal Layout with Buttons\n";
    
    Container* panel2 = new Container(&window, &window, 220, 10, 300, 100);
    panel2->SetName("horizontalPanel");
    panel2->setBackgroundColor(220, 240, 255);
    panel2->setBorderStyle(Container::BorderSingle);
    
    // Create horizontal layout
    HorizontalLayout* hLayout = new HorizontalLayout(&window);
    hLayout->setSpacing(5);
    hLayout->setMargins(10, 10, 10, 10);
    
    // Create buttons
    Button* okBtn = new Button(&window, panel2, 0, 0, 0, 0, "OK");
    Button* cancelBtn = new Button(&window, panel2, 0, 0, 0, 0, "Cancel");
    Button* applyBtn = new Button(&window, panel2, 0, 0, 0, 0, "Apply");
    
    okBtn->SetName("okButton");
    cancelBtn->SetName("cancelButton");
    applyBtn->SetName("applyButton");
    
    hLayout->addControl(okBtn);
    hLayout->addControl(cancelBtn);
    hLayout->addControl(applyBtn);
    
    panel2->setLayout(hLayout);
    
    std::cout << "✓ Created horizontal layout with 3 buttons\n";
    std::cout << "  - Layout spacing: " << hLayout->getSpacing() << "px\n";
    std::cout << "  - Layout controls: " << hLayout->getControlCount() << "\n\n";
    
    // ============================================
    // Example 3: Nested Layouts (Container in Container)
    // ============================================
    std::cout << "Example 3: Nested Layouts\n";
    
    Container* outerPanel = new Container(&window, &window, 530, 10, 250, 300);
    outerPanel->SetName("outerPanel");
    outerPanel->setBackgroundColor(255, 250, 240);
    outerPanel->setBorderStyle(Container::BorderDouble);
    
    // Outer vertical layout
    VerticalLayout* outerLayout = new VerticalLayout(&window);
    outerLayout->setSpacing(10);
    outerLayout->setMargins(10, 10, 10, 10);
    
    // Title label (simulated with button)
    Button* titleLabel = new Button(&window, outerPanel, 0, 0, 0, 0, "Settings");
    titleLabel->SetName("titleLabel");
    
    // Inner container with horizontal layout
    Container* innerPanel = new Container(&window, outerPanel, 0, 0, 0, 0);
    innerPanel->SetName("innerPanel");
    innerPanel->setBackgroundColor(255, 255, 255);
    innerPanel->setBorderStyle(Container::BorderSingle);
    
    HorizontalLayout* innerLayout = new HorizontalLayout(&window);
    innerLayout->setSpacing(5);
    innerLayout->setMargins(5, 5, 5, 5);
    
    Button* saveBtn = new Button(&window, innerPanel, 0, 0, 0, 0, "Save");
    Button* resetBtn = new Button(&window, innerPanel, 0, 0, 0, 0, "Reset");
    
    innerLayout->addControl(saveBtn);
    innerLayout->addControl(resetBtn);
    innerPanel->setLayout(innerLayout);
    
    // Add to outer layout
    outerLayout->addControl(titleLabel);
    outerLayout->addControl(innerPanel);
    outerPanel->setLayout(outerLayout);
    
    std::cout << "✓ Created nested layouts\n";
    std::cout << "  - Outer panel: " << outerPanel->GetName() << "\n";
    std::cout << "  - Inner panel: " << innerPanel->GetName() << "\n";
    std::cout << "  - Inner panel owner: " << (innerPanel->GetOwner() ? innerPanel->GetOwner()->GetName() : "none") << "\n";
    std::cout << "  - Inner panel parent: " << (innerPanel->GetParent() ? innerPanel->GetParent()->GetName() : "none") << "\n\n";
    
    // ============================================
    // Example 4: Component Lookup by Name
    // ============================================
    std::cout << "Example 4: Component Lookup by Name\n";
    
    // Find components by name
    Component* foundBtn = window.FindComponent("btn1");
    if (foundBtn) {
        Button* btn = dynamic_cast<Button*>(foundBtn);
        if (btn) {
            std::cout << "✓ Found button by name: " << btn->getLabel() << "\n";
        }
    }
    
    Component* foundPanel = window.FindComponent("buttonPanel");
    if (foundPanel) {
        Container* panel = dynamic_cast<Container*>(foundPanel);
        if (panel) {
            std::cout << "✓ Found panel by name: " << panel->GetName() << "\n";
            std::cout << "  - Panel has " << panel->GetComponentCount() << " owned components\n";
        }
    }
    
    std::cout << "\n";
    
    // ============================================
    // Example 5: Owner vs Parent Demonstration
    // ============================================
    std::cout << "Example 5: Owner vs Parent Separation\n";
    
    Container* demoPanel = new Container(&window, &window, 10, 320, 300, 100);
    demoPanel->SetName("demoPanel");
    demoPanel->setBackgroundColor(240, 255, 240);
    
    // Button owned by window, but parented to panel
    Button* demoBtn = new Button(&window, demoPanel, 10, 10, 100, 30, "Demo Button");
    demoBtn->SetName("demoButton");
    
    std::cout << "✓ Created button with different Owner and Parent\n";
    std::cout << "  - Button owner: " << (demoBtn->GetOwner() ? demoBtn->GetOwner()->GetName() : "none") << "\n";
    std::cout << "  - Button parent: " << (demoBtn->GetParent() ? demoBtn->GetParent()->GetName() : "none") << "\n";
    std::cout << "  - When window is destroyed, button is automatically cleaned up\n";
    std::cout << "  - Button is visually displayed in demoPanel\n\n";
    
    // ============================================
    // Example 6: Component Enumeration
    // ============================================
    std::cout << "Example 6: Component Enumeration\n";
    
    std::cout << "Window owns " << window.GetComponentCount() << " components:\n";
    for (int i = 0; i < window.GetComponentCount(); ++i) {
        Component* comp = window.GetComponent(i);
        if (comp) {
            std::cout << "  [" << i << "] " << comp->GetName() 
                      << " (type: " << typeid(*comp).name() << ")\n";
        }
    }
    
    std::cout << "\n";
    
    // ============================================
    // Example 7: Layout with Input Fields
    // ============================================
    std::cout << "Example 7: Layout with Input Fields\n";
    
    Container* formPanel = new Container(&window, &window, 320, 120, 200, 200);
    formPanel->SetName("formPanel");
    formPanel->setBackgroundColor(255, 255, 255);
    formPanel->setBorderStyle(Container::BorderSingle);
    
    VerticalLayout* formLayout = new VerticalLayout(&window);
    formLayout->setSpacing(8);
    formLayout->setMargins(10, 10, 10, 10);
    
    InputField* nameField = new InputField(&window, formPanel, 0, 0, 0, 0, "Enter name");
    InputField* emailField = new InputField(&window, formPanel, 0, 0, 0, 0, "Enter email");
    Button* submitBtn = new Button(&window, formPanel, 0, 0, 0, 0, "Submit");
    
    nameField->SetName("nameField");
    emailField->SetName("emailField");
    submitBtn->SetName("submitButton");
    
    formLayout->addControl(nameField);
    formLayout->addControl(emailField);
    formLayout->addControl(submitBtn);
    
    formPanel->setLayout(formLayout);
    
    submitBtn->setCallback([nameField, emailField](Control* parent) {
        std::cout << "\n=== Form Submitted ===\n";
        std::cout << "Name: " << nameField->getText() << "\n";
        std::cout << "Email: " << emailField->getText() << "\n";
        std::cout << "=====================\n\n";
    });
    
    std::cout << "✓ Created form with vertical layout\n";
    std::cout << "  - Name input field\n";
    std::cout << "  - Email input field\n";
    std::cout << "  - Submit button\n\n";
    
    // ============================================
    // Summary
    // ============================================
    std::cout << "=== Summary ===\n";
    std::cout << "✓ Component ownership system (automatic cleanup)\n";
    std::cout << "✓ Parent-based visual hierarchy\n";
    std::cout << "✓ Vertical and Horizontal layouts\n";
    std::cout << "✓ Nested layouts (containers in containers)\n";
    std::cout << "✓ Component naming and lookup\n";
    std::cout << "✓ Component enumeration\n";
    std::cout << "✓ Owner and Parent can be different\n";
    std::cout << "✓ Layout spacing and margins\n";
    std::cout << "\n";
    std::cout << "Total components owned by window: " << window.GetComponentCount() << "\n";
    std::cout << "All components will be automatically cleaned up when window is destroyed.\n";
    std::cout << "\n";
    std::cout << "Demo UI created! The window should be visible.\n";
    std::cout << "Click buttons to see callbacks in action.\n";
    std::cout << "Try resizing the window to see layout updates.\n";
    
    // Run the application event loop to keep the window open
    std::cout << "\nWindow is now visible. Close the window to exit.\n";
    std::cout << "Or press Ctrl+C in the terminal to exit.\n\n";
    
    // Initialize and run the application event loop
    Application::getInstance().run();
    
    // Cleanup happens automatically when window goes out of scope
    return 0;
}
