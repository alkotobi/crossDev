#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <string>
#include <nlohmann/json.hpp>

// Configuration manager for application options
// Handles platform-specific config directory locations
class ConfigManager {
public:
    // Get the singleton instance
    static ConfigManager& getInstance();
    
    // Get the platform-specific config directory path
    static std::string getConfigDirectory();
    
    // Get the full path to options.json
    static std::string getOptionsFilePath();
    
    // Load options from file (creates default if doesn't exist)
    bool loadOptions();
    
    // Save options to file
    bool saveOptions();
    
    // Get options JSON object
    nlohmann::json& getOptions() { return options_; }
    const nlohmann::json& getOptions() const { return options_; }
    
    // Get specific option value
    template<typename T>
    T getOption(const std::string& key, const T& defaultValue = T{}) const {
        if (options_.contains(key)) {
            return options_[key].get<T>();
        }
        return defaultValue;
    }
    
    // Set specific option value
    template<typename T>
    void setOption(const std::string& key, const T& value) {
        options_[key] = value;
    }
    
    // Get HTML loading method: "file", "url", or "html"
    std::string getHtmlLoadingMethod() const;
    
    // Set HTML loading method
    void setHtmlLoadingMethod(const std::string& method);
    
    // Get HTML file path (if method is "file")
    std::string getHtmlFilePath() const;
    
    // Get HTML URL (if method is "url")
    std::string getHtmlUrl() const;
    
    // Get HTML content (if method is "html")
    std::string getHtmlContent() const;
    
    // Get preload script path (empty = use built-in bridge). Path is relative to cwd or absolute.
    std::string getPreloadPath() const;
    
    // Get preload script content. If preloadPath is set and file exists, returns file content; else empty (use built-in).
    static std::string getPreloadScriptContent();
    
    // Try to load file content from standard locations (cwd, ., .., ../..)
    static std::string tryLoadFileContent(const std::string& filename);
    
private:
    ConfigManager() = default;
    ~ConfigManager() = default;
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;
    
    // Create default options structure
    nlohmann::json createDefaultOptions();
    
    // Ensure config directory exists
    bool ensureConfigDirectory();
    
    nlohmann::json options_;
    bool optionsLoaded_ = false;
};

#endif // CONFIG_MANAGER_H
