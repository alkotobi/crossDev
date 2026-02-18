#include "../include/config_manager.h"
#include "../include/platform.h"
#include <cerrno>
#include <fstream>
#include <iostream>
#include <sstream>
#include <filesystem>

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

std::string ConfigManager::getAppName() {
#ifdef CROSSDEV_APP_NAME
    return std::string(CROSSDEV_APP_NAME);
#else
    return "CrossDev";
#endif
}

std::string ConfigManager::getConfigBaseDirectory() {
    std::string baseDir;
#ifdef _WIN32
    // Windows: %APPDATA%\CrossDev
    char appDataPath[MAX_PATH];
    if (SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, appDataPath) == S_OK) {
        baseDir = std::string(appDataPath) + "\\CrossDev";
    } else {
        baseDir = ".\\CrossDev";
    }
#elif __APPLE__
    #if TARGET_OS_MAC
        const char* home = getenv("HOME");
        if (home) {
            baseDir = std::string(home) + "/Library/Application Support/CrossDev";
        } else {
            baseDir = "./CrossDev";
        }
    #elif TARGET_OS_IPHONE
        baseDir = "./Documents/CrossDev";
    #endif
#elif __linux__
    const char* home = getenv("HOME");
    if (home) {
        baseDir = std::string(home) + "/.config/CrossDev";
    } else {
        struct passwd* pw = getpwuid(getuid());
        if (pw && pw->pw_dir) {
            baseDir = std::string(pw->pw_dir) + "/.config/CrossDev";
        } else {
            baseDir = "./CrossDev";
        }
    }
#else
    baseDir = "./CrossDev";
#endif
    return baseDir;
}

std::string ConfigManager::getConfigDirectory() {
    std::string base = getConfigBaseDirectory();
#ifdef _WIN32
    return base + "\\" + getAppName();
#else
    return base + "/" + getAppName();
#endif
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

namespace {
    namespace fs = std::filesystem;
}

// Resolve file path to absolute so the WebView document base is the file's directory (relative resources work).
std::string ConfigManager::resolveFilePathToAbsolute(const std::string& filename) {
    if (filename.empty()) return "";
    fs::path p(filename);
    if (p.is_absolute()) {
        try {
            if (fs::is_regular_file(p)) return fs::absolute(p).string();
        } catch (...) {}
        return "";
    }
    std::string candidates[] = { filename, "./" + filename, "../" + filename, "../../" + filename };
    for (const std::string& candidate : candidates) {
        try {
            fs::path cp(candidate);
            if (fs::is_regular_file(cp))
                return fs::absolute(cp).string();
        } catch (...) {}
    }
    return "";
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

// Legacy path: .../CrossDev/options.json (no app subfolder). Used for migration.
static std::string getLegacyOptionsFilePath() {
    std::string base = ConfigManager::getConfigBaseDirectory();
#ifdef _WIN32
    return base + "\\options.json";
#else
    return base + "/options.json";
#endif
}

bool ConfigManager::loadOptions() {
    std::cout << "\n[ConfigManager::loadOptions] ===== CALLED =====" << std::endl;
    std::cout << "[ConfigManager] optionsLoaded_ flag: " << (optionsLoaded_ ? "TRUE (cached)" : "FALSE (will read)") << std::endl;
    
    // CRITICAL FIX: Never skip reading! Always re-read from disk in loadOptions()
    // The reload handler needs fresh data, not cached data
    
    std::string optionsPath = getOptionsFilePath();
    std::string legacyPath = getLegacyOptionsFilePath();
    
    std::cout << "[ConfigManager] Options path: " << optionsPath << std::endl;
    
    // Migration: if new path doesn't exist but legacy path does, copy legacy -> app folder
    std::ifstream checkNew(optionsPath);
    bool newExists = checkNew.is_open();
    checkNew.close();
    if (!newExists) {
        std::cout << "[ConfigManager] New options file doesn't exist, checking legacy path..." << std::endl;
        std::ifstream legacy(legacyPath);
        if (legacy.is_open()) {
            legacy.close();
            std::cout << "[ConfigManager] Legacy path found, migrating..." << std::endl;
            if (ensureConfigDirectory()) {
                std::ifstream src(legacyPath, std::ios::binary);
                std::ofstream dst(optionsPath, std::ios::binary);
                if (src.is_open() && dst.is_open()) {
                    dst << src.rdbuf();
                    src.close();
                    dst.close();
                    std::cout << "Migrated options from " << legacyPath << " to " << optionsPath << std::endl;
                }
            }
        }
    }
    
    if (!ensureConfigDirectory()) {
        std::cerr << "Warning: Could not create config directory, using defaults" << std::endl;
        options_ = createDefaultOptions();
        optionsLoaded_ = true;
        std::cout << "[ConfigManager::loadOptions] Set to defaults, returning FALSE\n" << std::endl;
        return false;
    }
    
    std::cout << "[ConfigManager] About to read from: " << optionsPath << std::endl;
    std::ifstream file(optionsPath);
    
    if (file.is_open()) {
        try {
            std::cout << "[ConfigManager] File opened successfully, reading JSON..." << std::endl;
            file >> options_;
            file.close();
            std::cout << "[ConfigManager] ✓ Successfully read options from: " << optionsPath << std::endl;
            std::cout << "[ConfigManager] HTML method: " << getHtmlLoadingMethod() << std::endl;
            std::cout << "[ConfigManager] File path: " << getHtmlFilePath() << std::endl;
            std::cout << "[ConfigManager] URL: " << getHtmlUrl() << std::endl;
            optionsLoaded_ = true;
            std::cout << "[ConfigManager::loadOptions] Returning TRUE\n" << std::endl;
            return true;
        } catch (const std::exception& e) {
            std::cerr << "[ConfigManager] ✗ Error parsing options.json: " << e.what() << std::endl;
            file.close();
        }
    } else {
        std::cout << "[ConfigManager] ✗ Options file not found at: " << optionsPath << std::endl;
        std::cout << "[ConfigManager] Checking if file exists on disk..." << std::endl;
        std::ifstream existsCheck(optionsPath);
        if (!existsCheck.is_open()) {
            std::cout << "[ConfigManager] File does NOT exist on disk (permission or path issue?)" << std::endl;
        }
        existsCheck.close();
    }
    
    options_ = createDefaultOptions();
    optionsLoaded_ = true;
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
