// Linux button implementation
#include "../../../include/platform.h"
#include "../platform_impl.h"
#include <string>
#include <gtk/gtk.h>

#ifdef PLATFORM_LINUX

namespace platform {

struct WindowData {
    Display* display;
    Window window;
    GtkWidget* gtkWindow;
    bool visible;
    std::string title;
};

struct ButtonData {
    GtkWidget* button;
    void* userData;
    void (*callback)(void*);
};

static void button_clicked_callback(GtkWidget* widget, gpointer userData) {
    ButtonData* data = static_cast<ButtonData*>(userData);
    if (data && data->callback) {
        data->callback(data->userData);
    }
}

void* createButton(void* parentHandle, int x, int y, int width, int height, const std::string& label, void* userData) {
    if (!parentHandle) {
        return nullptr;
    }
    
    GtkWidget* parentWidget = nullptr;
    
    // Get parent widget - could be GtkWidget (from Container) or WindowData
    if (GTK_IS_WIDGET((GtkWidget*)parentHandle)) {
        parentWidget = (GtkWidget*)parentHandle;
    } else {
        // Try to get from WindowData
        WindowData* windowData = static_cast<WindowData*>(parentHandle);
        if (windowData && windowData->gtkWindow) {
            parentWidget = windowData->gtkWindow;
        } else {
            return nullptr;
        }
    }
    
    ButtonData* buttonData = new ButtonData;
    buttonData->userData = userData;
    buttonData->callback = nullptr;
    
    buttonData->button = gtk_button_new_with_label(label.c_str());
    gtk_widget_set_size_request(buttonData->button, width, height);
    
    g_signal_connect(buttonData->button, "clicked", G_CALLBACK(button_clicked_callback), buttonData);
    
    if (parentWidget) {
        GtkWidget* fixed = gtk_fixed_new();
        gtk_container_add(GTK_CONTAINER(parentWidget), fixed);
        gtk_fixed_put(GTK_FIXED(fixed), buttonData->button, x, y);
        gtk_widget_show(buttonData->button);
    }
    
    return buttonData;
}

void destroyButton(void* buttonHandle) {
    if (buttonHandle) {
        ButtonData* data = static_cast<ButtonData*>(buttonHandle);
        if (data->button) {
            gtk_widget_destroy(data->button);
        }
        delete data;
    }
}

void setButtonCallback(void* buttonHandle, void (*callback)(void*)) {
    if (buttonHandle) {
        ButtonData* data = static_cast<ButtonData*>(buttonHandle);
        data->callback = callback;
    }
}

} // namespace platform

#endif // PLATFORM_LINUX
