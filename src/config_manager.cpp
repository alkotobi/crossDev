#include "../include/config_manager.h"
#include "../include/platform.h"
#include <cerrno>
#include <fstream>
#include <iostream>
#include <sstream>

#ifdef _WIN32
    #include <windows.h>
    #include <shlobj.h>
    #include <direct.h>
    #define mkdir(path, mode) _mkdir(path)
#elif __APPLE__
    #include <TargetConditionals.h>
    #if TARGET_OS_MAC
        #include <unistd.h>
        #include <sys/stat.h>
    #endif
#elif __linux__
    #include <unistd.h>
    #include <sys/stat.h>
    #include <pwd.h>
#endif

ConfigManager& ConfigManager::getInstance() {
    static ConfigManager instance;
    return instance;
}

std::string ConfigManager::getConfigDirectory() {
    std::string configDir;
    
#ifdef _WIN32
    // Windows: %APPDATA%\CrossDev
    char appDataPath[MAX_PATH];
    if (SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, appDataPath) == S_OK) {
        configDir = std::string(appDataPath) + "\\CrossDev";
    } else {
        // Fallback to current directory
        configDir = ".\\CrossDev";
    }
#elif __APPLE__
    #if TARGET_OS_MAC
        // macOS: ~/Library/Application Support/CrossDev
        const char* home = getenv("HOME");
        if (home) {
            configDir = std::string(home) + "/Library/Application Support/CrossDev";
        } else {
            configDir = "./CrossDev";
        }
    #elif TARGET_OS_IPHONE
        // iOS: App's Documents directory (managed by iOS)
        // For iOS, we'll use a relative path that works in the app sandbox
        configDir = "./Documents/CrossDev";
    #endif
#elif __linux__
    // Linux: ~/.config/CrossDev
    const char* home = getenv("HOME");
    if (home) {
        configDir = std::string(home) + "/.config/CrossDev";
    } else {
        // Fallback for systems without HOME (shouldn't happen normally)
        struct passwd* pw = getpwuid(getuid());
        if (pw && pw->pw_dir) {
            configDir = std::string(pw->pw_dir) + "/.config/CrossDev";
        } else {
            configDir = "./CrossDev";
        }
    }
#else
    // Unknown platform - use current directory
    configDir = "./CrossDev";
#endif
    
    return configDir;
}

std::string ConfigManager::getOptionsFilePath() {
    return getConfigDirectory() + 
#ifdef _WIN32
           "\\options.json"
#else
           "/options.json"
#endif
           ;
}

bool ConfigManager::ensureConfigDirectory() {
    std::string configDir = getConfigDirectory();
    
#ifdef _WIN32
    // Create directory and parents on Windows (e.g. %APPDATA%\CrossDev)
    for (size_t i = 1; i < configDir.length(); ++i) {
        if (configDir[i] == '\\' || configDir[i] == '/') {
            std::string sub = configDir.substr(0, i);
            DWORD attrib = GetFileAttributesA(sub.c_str());
            if (attrib == INVALID_FILE_ATTRIBUTES) {
                if (_mkdir(sub.c_str()) != 0) {
                    DWORD err = GetLastError();
                    if (err != ERROR_ALREADY_EXISTS) {
                        std::cerr << "Failed to create config directory: " << sub << std::endl;
                        return false;
                    }
                }
            } else if (!(attrib & FILE_ATTRIBUTE_DIRECTORY)) {
                std::cerr << "Config path component is not a directory: " << sub << std::endl;
                return false;
            }
        }
    }
    if (_mkdir(configDir.c_str()) != 0) {
        DWORD attrib = GetFileAttributesA(configDir.c_str());
        if (attrib == INVALID_FILE_ATTRIBUTES || !(attrib & FILE_ATTRIBUTE_DIRECTORY)) {
            std::cerr << "Failed to create config directory: " << configDir << std::endl;
            return false;
        }
    }
#else
    // Create directory and parents on Unix (e.g. ~/Library/Application Support/CrossDev)
    for (size_t i = 1; i < configDir.length(); ++i) {
        if (configDir[i] == '/') {
            std::string sub = configDir.substr(0, i);
            struct stat info;
            if (stat(sub.c_str(), &info) != 0) {
                if (mkdir(sub.c_str(), 0755) != 0 && errno != EEXIST) {
                    std::cerr << "Failed to create config directory: " << sub << std::endl;
                    return false;
                }
            } else if (!(info.st_mode & S_IFDIR)) {
                std::cerr << "Config path component is not a directory: " << sub << std::endl;
                return false;
            }
        }
    }
    struct stat info;
    if (stat(configDir.c_str(), &info) != 0) {
        if (mkdir(configDir.c_str(), 0755) != 0) {
            std::cerr << "Failed to create config directory: " << configDir << std::endl;
            return false;
        }
    } else if (!(info.st_mode & S_IFDIR)) {
        std::cerr << "Config path exists but is not a directory: " << configDir << std::endl;
        return false;
    }
#endif
    
    return true;
}

static std::string tryLoadFromPaths(const std::string& filename) {
    std::string paths[] = {filename, "./" + filename, "../" + filename, "../../" + filename};
    for (const std::string& path : paths) {
        std::ifstream f(path);
        if (f.is_open()) {
            std::stringstream buffer;
            buffer << f.rdbuf();
            std::string content = buffer.str();
            f.close();
            if (!content.empty()) {
                return content;
            }
        }
    }
    return "";
}

static std::string readDemoHtmlContent() {
    std::string content = tryLoadFromPaths("demo.html");
    if (!content.empty()) {
        std::cout << "Loaded demo.html for options.json" << std::endl;
    } else {
        std::cerr << "Warning: Could not find demo.html to copy into options.json" << std::endl;
    }
    return content;
}

std::string ConfigManager::tryLoadFileContent(const std::string& filename) {
    return tryLoadFromPaths(filename);
}

nlohmann::json ConfigManager::createDefaultOptions() {
    nlohmann::json defaultOptions;
    
    // HTML loading configuration - default to loading from embedded JSON content
    defaultOptions["htmlLoading"] = nlohmann::json::object();
    defaultOptions["htmlLoading"]["method"] = "html";  // "file", "url", or "html"
    defaultOptions["htmlLoading"]["filePath"] = "demo.html";  // Path to HTML file (when method is "file")
    defaultOptions["htmlLoading"]["url"] = "";  // URL to load (when method is "url")
    defaultOptions["htmlLoading"]["htmlContent"] = readDemoHtmlContent();  // Copy from demo.html on first run
    defaultOptions["htmlLoading"]["preloadPath"] = "";  // Custom preload script path; empty = use built-in bridge
    
    return defaultOptions;
}

bool ConfigManager::loadOptions() {
    if (optionsLoaded_) {
        return true;
    }
    
    // Ensure config directory exists
    if (!ensureConfigDirectory()) {
        std::cerr << "Warning: Could not create config directory, using defaults" << std::endl;
        options_ = createDefaultOptions();
        optionsLoaded_ = true;
        return false;
    }
    
    std::string optionsPath = getOptionsFilePath();
    std::ifstream file(optionsPath);
    
    if (file.is_open()) {
        try {
            file >> options_;
            file.close();
            std::cout << "Loaded options from: " << optionsPath << std::endl;
            optionsLoaded_ = true;
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Error parsing options.json: " << e.what() << std::endl;
            file.close();
        }
    } else {
        std::cout << "Options file not found, creating default: " << optionsPath << std::endl;
    }
    
    // Create default options
    options_ = createDefaultOptions();
    optionsLoaded_ = true;
    
    // Save default options to file
    saveOptions();
    
    return true;
}

bool ConfigManager::saveOptions() {
    if (!ensureConfigDirectory()) {
        std::cerr << "Error: Could not create config directory" << std::endl;
        return false;
    }
    
    std::string optionsPath = getOptionsFilePath();
    std::ofstream file(optionsPath);
    
    if (!file.is_open()) {
        std::cerr << "Error: Could not open options file for writing: " << optionsPath << std::endl;
        return false;
    }
    
    try {
        // Write with pretty printing (indentation)
        file << options_.dump(4);
        file.close();
        std::cout << "Saved options to: " << optionsPath << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error writing options.json: " << e.what() << std::endl;
        file.close();
        return false;
    }
}

std::string ConfigManager::getHtmlLoadingMethod() const {
    if (options_.contains("htmlLoading") && 
        options_["htmlLoading"].contains("method") &&
        options_["htmlLoading"]["method"].is_string()) {
        std::string method = options_["htmlLoading"]["method"].get<std::string>();
        if (method == "file" || method == "url" || method == "html") {
            return method;
        }
    }
    return "file";  // Default
}

void ConfigManager::setHtmlLoadingMethod(const std::string& method) {
    if (method != "file" && method != "url" && method != "html") {
        std::cerr << "Warning: Invalid HTML loading method: " << method 
                  << ". Must be 'file', 'url', or 'html'" << std::endl;
        return;
    }
    
    if (!options_.contains("htmlLoading")) {
        options_["htmlLoading"] = nlohmann::json::object();
    }
    options_["htmlLoading"]["method"] = method;
}

std::string ConfigManager::getHtmlFilePath() const {
    if (options_.contains("htmlLoading") && 
        options_["htmlLoading"].contains("filePath") &&
        options_["htmlLoading"]["filePath"].is_string()) {
        return options_["htmlLoading"]["filePath"].get<std::string>();
    }
    return "demo.html";  // Default
}

std::string ConfigManager::getHtmlUrl() const {
    if (options_.contains("htmlLoading") && 
        options_["htmlLoading"].contains("url") &&
        options_["htmlLoading"]["url"].is_string()) {
        return options_["htmlLoading"]["url"].get<std::string>();
    }
    return "";  // Default
}

std::string ConfigManager::getHtmlContent() const {
    if (options_.contains("htmlLoading") && 
        options_["htmlLoading"].contains("htmlContent") &&
        options_["htmlLoading"]["htmlContent"].is_string()) {
        return options_["htmlLoading"]["htmlContent"].get<std::string>();
    }
    return "";  // Default
}

std::string ConfigManager::getPreloadPath() const {
    if (options_.contains("htmlLoading") && 
        options_["htmlLoading"].contains("preloadPath") &&
        options_["htmlLoading"]["preloadPath"].is_string()) {
        return options_["htmlLoading"]["preloadPath"].get<std::string>();
    }
    return "";  // Default: use built-in
}

std::string ConfigManager::getPreloadScriptContent() {
    std::string path = getInstance().getPreloadPath();
    if (path.empty()) return "";
    return tryLoadFileContent(path);
}
