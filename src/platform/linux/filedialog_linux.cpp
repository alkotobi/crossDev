// Linux file dialog implementation
#include "../../../include/platform.h"
#include "../platform_impl.h"
#include <string>
#include <gtk/gtk.h>

#ifdef PLATFORM_LINUX

namespace platform {

bool showOpenFileDialog(void* windowHandle, const std::string& title, const std::string& filter, std::string& selectedPath) {
    // Ensure GTK is initialized
    if (!gtk_is_initialized()) {
        gtk_init(nullptr, nullptr);
    }
    
    GtkWidget* dialog;
    GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
    
    dialog = gtk_file_chooser_dialog_new(
        title.empty() ? "Open HTML File" : title.c_str(),
        nullptr,
        action,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Open", GTK_RESPONSE_ACCEPT,
        NULL
    );
    
    // Set filter for HTML files
    GtkFileFilter* htmlFilter = gtk_file_filter_new();
    gtk_file_filter_set_name(htmlFilter, "HTML Files");
    gtk_file_filter_add_pattern(htmlFilter, "*.html");
    gtk_file_filter_add_pattern(htmlFilter, "*.htm");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), htmlFilter);
    
    // Add all files filter
    GtkFileFilter* allFilter = gtk_file_filter_new();
    gtk_file_filter_set_name(allFilter, "All Files");
    gtk_file_filter_add_pattern(allFilter, "*");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), allFilter);
    
    bool result = false;
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char* filename;
        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        selectedPath = filename;
        g_free(filename);
        result = true;
    }
    
    gtk_widget_destroy(dialog);
    
    // Process pending events
    while (gtk_events_pending()) {
        gtk_main_iteration();
    }
    
    return result;
}

} // namespace platform

#endif // PLATFORM_LINUX
