// Linux container implementation
#include "../../../include/platform.h"
#include "../platform_impl.h"
#include <string>
#include <gtk/gtk.h>

#ifdef PLATFORM_LINUX

namespace platform {

void* createContainer(void* parentHandle, int x, int y, int width, int height, bool flipped) {
    (void)flipped;
    if (!parentHandle) {
        return nullptr;
    }
    
    GtkWidget* parentWidget = nullptr;
    
    // Get parent widget - could be GtkWindow or GtkWidget
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
    
    GtkWidget* container = gtk_fixed_new();
    gtk_widget_set_size_request(container, width, height);
    
    if (GTK_IS_CONTAINER(parentWidget)) {
        gtk_container_add(GTK_CONTAINER(parentWidget), container);
        gtk_fixed_move(GTK_FIXED(container), container, x, y);
        gtk_widget_show(container);
    }
    
    return container;
}

void destroyContainer(void* containerHandle) {
    if (containerHandle) {
        GtkWidget* container = (GtkWidget*)containerHandle;
        if (GTK_IS_WIDGET(container)) {
            gtk_widget_destroy(container);
        }
    }
}

void resizeContainer(void* containerHandle, int x, int y, int width, int height) {
    if (containerHandle) {
        GtkWidget* container = (GtkWidget*)containerHandle;
        if (GTK_IS_WIDGET(container)) {
            gtk_widget_set_size_request(container, width, height);
            // Note: GTK Fixed container positioning is handled by parent
        }
    }
}

void showContainer(void* containerHandle) {
    if (containerHandle) {
        GtkWidget* container = (GtkWidget*)containerHandle;
        if (GTK_IS_WIDGET(container)) {
            gtk_widget_show(container);
        }
    }
}

void hideContainer(void* containerHandle) {
    if (containerHandle) {
        GtkWidget* container = (GtkWidget*)containerHandle;
        if (GTK_IS_WIDGET(container)) {
            gtk_widget_hide(container);
        }
    }
}

void bringContainerToFront(void* containerHandle) {
    (void)containerHandle;
    /* GTK doesn't have z-order like Cocoa; layout order determines draw order */
}

void setContainerBackgroundColor(void* containerHandle, int red, int green, int blue) {
    if (containerHandle) {
        GtkWidget* container = (GtkWidget*)containerHandle;
        if (GTK_IS_WIDGET(container)) {
            GdkRGBA color;
            color.red = red / 255.0;
            color.green = green / 255.0;
            color.blue = blue / 255.0;
            color.alpha = 1.0;
            gtk_widget_override_background_color(container, GTK_STATE_FLAG_NORMAL, &color);
        }
    }
}

void setContainerBorderStyle(void* containerHandle, int borderStyle) {
    if (containerHandle) {
        GtkWidget* container = (GtkWidget*)containerHandle;
        if (GTK_IS_WIDGET(container)) {
            // GTK doesn't have direct border style, but we can use CSS
            // For now, this is a placeholder
            (void)borderStyle; // Suppress unused parameter warning
        }
    }
}

} // namespace platform

#endif // PLATFORM_LINUX
