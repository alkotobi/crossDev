#ifndef SETTINGS_EMBED_H
#define SETTINGS_EMBED_H

#include <string>

// Returns embedded Settings HTML (from cross_dev/settings.html at build time).
// Used by create_window_handler when className == "settings" with no url/html/file.
const std::string& getEmbeddedSettingsHtml();

#endif // SETTINGS_EMBED_H
