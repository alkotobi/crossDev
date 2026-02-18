// Linux input field implementation
#include "../../../include/platform.h"
#include "../platform_impl.h"
#include <string>
#include <gtk/gtk.h>

#ifdef PLATFORM_LINUX

namespace platform {

void* createInputField(void* parentHandle, int x, int y, int width, int height, const std::string& placeholder) {
    if (!parentHandle) {
        return nullptr;
    }
    
    GtkWidget* parentWidget = nullptr;
    
    // Get parent widget - could be GtkWidget (from Container) or WindowData
    if (GTK_IS_WIDGET((GtkWidget*)parentHandle)) {
        parentWidget = (GtkWidget*)parentHandle;
    } else {
        // Try to get from WindowData
        struct WindowData {
            Display* display;
            Window window;
            GtkWidget* gtkWindow;
            bool visible;
            std::string title;
        };
        
        WindowData* windowData = static_cast<WindowData*>(parentHandle);
        if (windowData && windowData->gtkWindow) {
            parentWidget = windowData->gtkWindow;
        } else {
            return nullptr;
        }
    }
    
    GtkWidget* entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry), placeholder.c_str());
    gtk_widget_set_size_request(entry, width, height);
    
    if (parentWidget) {
        GtkWidget* fixed = gtk_fixed_new();
        gtk_container_add(GTK_CONTAINER(parentWidget), fixed);
        gtk_fixed_put(GTK_FIXED(fixed), entry, x, y);
        gtk_widget_show(entry);
    }
    
    return entry;
}

void destroyInputField(void* inputHandle) {
    if (inputHandle) {
        GtkWidget* entry = (GtkWidget*)inputHandle;
        gtk_widget_destroy(entry);
    }
}

void setInputText(void* inputHandle, const std::string& text) {
    if (inputHandle) {
        GtkWidget* entry = (GtkWidget*)inputHandle;
        gtk_entry_set_text(GTK_ENTRY(entry), text.c_str());
    }
}

std::string getInputText(void* inputHandle) {
    if (inputHandle) {
        GtkWidget* entry = (GtkWidget*)inputHandle;
        const char* text = gtk_entry_get_text(GTK_ENTRY(entry));
        if (text) {
            return std::string(text);
        }
    }
    return "";
}

} // namespace platform

#endif // PLATFORM_LINUX
