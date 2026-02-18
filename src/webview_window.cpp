#include "../include/webview_window.h"
#include "../include/application.h"
#include "../include/config_manager.h"
#include "../include/native_event_bus.h"
#include "../include/platform.h"
#include "platform/platform_impl.h"
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <iostream>

namespace {
    const char* DEFAULT_HTML = R"(<!DOCTYPE html>
<html><head><meta charset="UTF-8"><title>CrossDev</title></head>
<body style="font-family:sans-serif;padding:2em;margin:0;">
<h1>CrossDev</h1>
<p>Welcome. Configure content via options.json or pass HTML, URL, or file path to WebViewWindow.</p>
<p style="color:#666;font-size:0.9em;">Right-click for context menu. Main menu: File, Edit, View, Help — at the <b>top of the screen</b> (macOS) or below the title bar (Windows/Linux).</p>
<div id="events" style="margin-top:1em;padding:0.5em;background:#f5f5f5;border-radius:4px;font-size:0.85em;"></div>
<script>
var eventsDiv=document.getElementById('events');
function logEvent(name,payload){
  var p=document.createElement('p');
  p.textContent=new Date().toLocaleTimeString()+' '+name+(payload&&Object.keys(payload).length?' '+JSON.stringify(payload):'');
  eventsDiv.insertBefore(p,eventsDiv.firstChild);
}
if(window.CrossDev&&window.CrossDev.events){
  CrossDev.events.on('window:focus',function(){logEvent('window:focus');});
  CrossDev.events.on('window:blur',function(){logEvent('window:blur');});
  CrossDev.events.on('window:resize',function(p){logEvent('window:resize',p);});
  CrossDev.events.on('window:move',function(p){logEvent('window:move',p);});
  CrossDev.events.on('window:close',function(){logEvent('window:close');});
  CrossDev.events.on('window:close-request',function(){logEvent('window:close-request');});
  CrossDev.events.on('window:minimize',function(p){logEvent('window:minimize',p);});
  CrossDev.events.on('window:maximize',function(p){logEvent('window:maximize',p);});
  CrossDev.events.on('window:restore',function(p){logEvent('window:restore',p);});
  CrossDev.events.on('file:dropped',function(p){logEvent('file:dropped',p);});
  CrossDev.events.on('menu:item',function(p){logEvent('menu:item',p);});
  CrossDev.events.on('menu:context',function(p){logEvent('menu:context',p);});
  CrossDev.events.on('app:activate',function(){logEvent('app:activate');});
  CrossDev.events.on('app:deactivate',function(){logEvent('app:deactivate');});
  CrossDev.events.on('app:quit',function(){logEvent('app:quit');});
  CrossDev.events.on('theme:changed',function(p){logEvent('theme:changed',p);});
  CrossDev.events.on('key:shortcut',function(p){logEvent('key:shortcut',p);});
  CrossDev.events.on('app:open-file',function(p){logEvent('app:open-file',p);});
  logEvent('Events registered');
}
document.addEventListener('contextmenu',function(e){
  e.preventDefault();
  if(window.CrossDev&&window.CrossDev.invoke){
    window.CrossDev.invoke('showContextMenu',{x:e.clientX,y:e.clientY}).then(function(r){
      if(r&&r.itemId)logEvent('Context menu selected',r);
    }).catch(function(){});
  }
});
</script>
</body></html>)";
}

// Global: the first WebViewWindow created with owner=nullptr
static WebViewWindow* g_mainWebViewWindow = nullptr;
// Tracks which WebViewWindow has focus for Print (File menu prints focused window)
static WebViewWindow* g_focusedWebViewForPrint = nullptr;

WebViewWindow* WebViewWindow::GetMainWebViewWindow() {
    return g_mainWebViewWindow;
}

WebViewWindow::WebViewWindow(Component* owner, int x, int y, int width, int height, const std::string& title,
                             WebViewContentType type, const std::string& content)
    : Component(owner), window_(nullptr), webView_(nullptr), onDestroyCallback_(nullptr) {
    // First one with null owner becomes the main window
    if (!owner && !g_mainWebViewWindow) {
        g_mainWebViewWindow = this;
    }
    // Create the window (resizable by default in platform implementations)
    window_ = std::make_unique<Window>(nullptr, nullptr, x, y, width, height, title);
    
    if (!window_ || !window_->getNativeHandle()) {
        throw std::runtime_error("Failed to create window for WebViewWindow");
    }
    
    // Create WebView that fills the entire window
    webView_ = std::make_unique<WebView>(window_.get(), window_.get(), 0, 0, width, height);
    
    if (!webView_ || !webView_->getNativeHandle()) {
        throw std::runtime_error("Failed to create WebView for WebViewWindow");
    }
    
    // Load initial content based on type
    if (type == WebViewContentType::Html && !content.empty()) {
        webView_->loadHTMLString(content);
    } else if (type == WebViewContentType::Url && !content.empty()) {
        webView_->loadURL(content);
    } else if (type == WebViewContentType::File && !content.empty()) {
        // Always load via file URL so the document base is the file's directory (relative JS/CSS work).
        std::string absolutePath = ConfigManager::resolveFilePathToAbsolute(content);
        if (!absolutePath.empty()) {
            webView_->loadHTMLFile(absolutePath);
        } else {
            webView_->loadHTMLString(
                "<html><body><h1>File not found</h1><p>Check filePath in options.json: " + content + "</p></body></html>");
        }
    } else {
        webView_->loadHTMLString(DEFAULT_HTML);
    }
    
    registerResizeCallback();
    registerMoveCallback();
    registerFileDropCallback();
    registerStateCallback();
    registerEventCallbacks();
    if (owner) {
        registerCloseCallback();  // Child: delete self when user closes window
    } else {
        registerMainWindowCloseCallback();  // Main: quit app when user closes window
        registerMainMenu();  // Main window: File/Edit menu
    }
    resetDefaultNameCache();
    debugLogLifecycleCreation(this, GetOwner(), nullptr);
}

void WebViewWindow::setOnDestroyCallback(std::function<void()> callback) {
    onDestroyCallback_ = std::move(callback);
}

WebViewWindow::~WebViewWindow() {
    if (onDestroyCallback_) {
        auto cb = std::move(onDestroyCallback_);
        cb();
    }
#ifndef NDEBUG
    const char* trigger = "?";
    if (g_mainWebViewWindow == this) {
        trigger = "main window (Application::quit -> main exit)";
    } else if (!GetOwner()) {
        trigger = "close callback (user clicked X)";
    } else {
        trigger = "owner destructor (Component cascade)";
    }
    std::cout << "[WebViewWindow] DESTROYED this=" << (void*)this
              << " owner=" << (void*)GetOwner()
              << " | triggered by: " << trigger << std::endl;
#endif
    if (g_mainWebViewWindow == this) {
        g_mainWebViewWindow = nullptr;
    }
    if (webView_) {
        NativeEventBus::getInstance().unsubscribe(webView_.get());
    }
    // Unique pointers and Component will clean up owned components
}

void WebViewWindow::show() {
    if (window_) {
        window_->show();
    }
}

void WebViewWindow::hide() {
    if (window_) {
        window_->hide();
    }
}

void WebViewWindow::setTitle(const std::string& title) {
    if (window_) {
        window_->setTitle(title);
    }
}

bool WebViewWindow::isVisible() const {
    if (window_) {
        return window_->isVisible();
    }
    return false;
}

void WebViewWindow::loadHTMLFile(const std::string& filePath) {
    if (webView_) {
        webView_->loadHTMLFile(filePath);
    }
}

void WebViewWindow::loadHTMLString(const std::string& html) {
    if (webView_) {
        webView_->loadHTMLString(html);
    }
}

void WebViewWindow::loadURL(const std::string& url) {
    if (webView_) {
        webView_->loadURL(url);
    }
}

void WebViewWindow::setCreateWindowCallback(std::function<void(const std::string& title)> callback) {
    if (webView_) {
        webView_->setCreateWindowCallback(callback);
    }
}

void WebViewWindow::setMessageCallback(std::function<void(const std::string& jsonMessage)> callback) {
    if (webView_) {
        webView_->setMessageCallback(callback);
    }
}

void WebViewWindow::postMessageToJavaScript(const std::string& jsonMessage) {
    if (webView_) {
        webView_->postMessageToJavaScript(jsonMessage);
    }
}

void WebViewWindow::onWindowResize(int newWidth, int newHeight) {
    if (webView_ && window_) {
        // Update WebView bounds using Control's SetBounds method
        // This demonstrates the component system - bounds are managed by Control
        webView_->SetBounds(0, 0, newWidth, newHeight);
        // The OnBoundsChanged() will call updateNativeWebViewBounds() which uses platform::resizeWebView
        // Emit window:resize event for JS listeners
        std::string payload = "{\"width\":" + std::to_string(newWidth) + ",\"height\":" + std::to_string(newHeight) + "}";
        NativeEventBus::getInstance().emitTo(webView_.get(), "window:resize", payload);
    }
}

void WebViewWindow::registerResizeCallback() {
    if (window_ && window_->getNativeHandle()) {
        platform::setWindowResizeCallback(
            window_->getNativeHandle(),
            [](int width, int height, void* userData) {
                WebViewWindow* self = static_cast<WebViewWindow*>(userData);
                if (self) {
                    self->onWindowResize(width, height);
                }
            },
            this
        );
    }
}

void WebViewWindow::onWindowMove(int x, int y) {
    if (webView_) {
        std::string payload = "{\"x\":" + std::to_string(x) + ",\"y\":" + std::to_string(y) + "}";
        NativeEventBus::getInstance().emitTo(webView_.get(), "window:move", payload);
    }
}

void WebViewWindow::registerMoveCallback() {
    if (window_ && window_->getNativeHandle()) {
        platform::setWindowMoveCallback(
            window_->getNativeHandle(),
            [](int x, int y, void* userData) {
                WebViewWindow* self = static_cast<WebViewWindow*>(userData);
                if (self) {
                    self->onWindowMove(x, y);
                }
            },
            this
        );
    }
}

void WebViewWindow::registerFileDropCallback() {
    if (window_ && window_->getNativeHandle()) {
        platform::setWindowFileDropCallback(
            window_->getNativeHandle(),
            [](const std::string& pathsJson, void* userData) {
                WebViewWindow* self = static_cast<WebViewWindow*>(userData);
                if (self && self->getWebView()) {
                    NativeEventBus::getInstance().emitTo(self->getWebView(), "file:dropped", pathsJson);
                }
            },
            this
        );
    }
}

void WebViewWindow::registerStateCallback() {
    if (window_ && window_->getNativeHandle()) {
        platform::setWindowStateCallback(
            window_->getNativeHandle(),
            [](const char* state, void* userData) {
                WebViewWindow* self = static_cast<WebViewWindow*>(userData);
                if (self && self->getWebView()) {
                    std::string eventName = "window:";
                    eventName += state;
                    std::string payload = "{\"state\":\"" + std::string(state) + "\"}";
                    NativeEventBus::getInstance().emitTo(self->getWebView(), eventName, payload);
                }
            },
            this
        );
    }
}

void WebViewWindow::registerCloseCallback() {
    if (window_ && window_->getNativeHandle() && GetOwner()) {
        platform::setWindowCloseCallback(
            window_->getNativeHandle(),
            [](void* userData) {
                WebViewWindow* self = static_cast<WebViewWindow*>(userData);
                if (self && g_focusedWebViewForPrint == self) g_focusedWebViewForPrint = nullptr;
                if (self && self->getWebView()) {
                    NativeEventBus::getInstance().emitTo(self->getWebView(), "window:close-request", "{}");
                    NativeEventBus::getInstance().emitTo(self->getWebView(), "window:close", "{}");
                }
                if (self) {
                    self->SetOwner(nullptr);
                    delete self;
                }
            },
            this
        );
    }
}

void WebViewWindow::closeAllOwnedWebViewWindows() {
    // Collect child WebViewWindows first (we'll modify the list by deleting)
    std::vector<WebViewWindow*> children;
    for (int i = 0; i < GetComponentCount(); ++i) {
        Component* comp = GetComponent(i);
        if (WebViewWindow* w = dynamic_cast<WebViewWindow*>(comp)) {
            children.push_back(w);
        }
    }
    for (WebViewWindow* w : children) {
        if (w->getWindow() && w->getWindow()->getNativeHandle()) {
            // Clear close callback so destroyWindow won't queue a redundant delete
            platform::setWindowCloseCallback(w->getWindow()->getNativeHandle(), nullptr, nullptr);
        }
        w->SetOwner(nullptr);
        delete w;
    }
}

namespace {
void onWindowFocus(void* userData) {
    WebViewWindow* self = static_cast<WebViewWindow*>(userData);
    if (self) g_focusedWebViewForPrint = self;
    if (self && self->getWebView()) {
        NativeEventBus::getInstance().emitTo(self->getWebView(), "window:focus", "{}");
    }
}
void onWindowBlur(void* userData) {
    WebViewWindow* self = static_cast<WebViewWindow*>(userData);
    if (self && g_focusedWebViewForPrint == self) g_focusedWebViewForPrint = nullptr;
    if (self && self->getWebView()) {
        NativeEventBus::getInstance().emitTo(self->getWebView(), "window:blur", "{}");
    }
}
}

namespace {
void onAppActivate(void* userData) {
    (void)userData;
    NativeEventBus::getInstance().emitToAll("app:activate", "{}");
}
void onAppDeactivate(void* userData) {
    (void)userData;
    NativeEventBus::getInstance().emitToAll("app:deactivate", "{}");
}
void onThemeChange(const char* theme, void* userData) {
    (void)userData;
    std::string payload = std::string("{\"theme\":\"") + theme + "\"}";
    NativeEventBus::getInstance().emitToAll("theme:changed", payload);
}
void onKeyShortcut(const std::string& payloadJson, void* userData) {
    (void)userData;
    NativeEventBus::getInstance().emitToAll("key:shortcut", payloadJson);
}
void onAppOpenFile(const std::string& path, void* userData) {
    (void)userData;
    nlohmann::json j;
    j["path"] = path;
    NativeEventBus::getInstance().emitToAll("app:open-file", j.dump());
}
}

void WebViewWindow::registerEventCallbacks() {
    if (!window_ || !webView_ || !window_->getNativeHandle()) return;
    NativeEventBus::getInstance().subscribe(webView_.get());
    platform::setWindowFocusCallback(window_->getNativeHandle(), onWindowFocus, this);
    platform::setWindowBlurCallback(window_->getNativeHandle(), onWindowBlur, this);
    if (!GetOwner()) {
        platform::setAppActivateCallback(onAppActivate, nullptr);
        platform::setAppDeactivateCallback(onAppDeactivate, nullptr);
        platform::setThemeChangeCallback(onThemeChange, nullptr);
        platform::setKeyShortcutCallback(onKeyShortcut, nullptr);
        platform::setAppOpenFileCallback(onAppOpenFile, nullptr);
    }
}

namespace {
void onMainMenuItem(const std::string& itemId, void* userData) {
    WebViewWindow* self = static_cast<WebViewWindow*>(userData);
    if (!self || !self->getWebView()) return;
    // Emit menu:item event for JS listeners (payload: id) - use JSON for proper escaping
    nlohmann::json j;
    j["id"] = itemId;
    NativeEventBus::getInstance().emitTo(self->getWebView(), "menu:item", j.dump());
    if (itemId == "quit") {
        NativeEventBus::getInstance().emitToAll("app:quit", "{}");
        Application::getInstance().quit();
        return;
    }
    // Edit menu: execute native clipboard/selection commands in the WebView
    if (itemId == "undo") {
        platform::executeWebViewScript(self->getWebView()->getNativeHandle(), "document.execCommand('undo');");
        return;
    }
    if (itemId == "redo") {
        platform::executeWebViewScript(self->getWebView()->getNativeHandle(), "document.execCommand('redo');");
        return;
    }
    if (itemId == "cut") {
        platform::executeWebViewScript(self->getWebView()->getNativeHandle(), "document.execCommand('cut');");
        return;
    }
    if (itemId == "copy") {
        platform::executeWebViewScript(self->getWebView()->getNativeHandle(), "document.execCommand('copy');");
        return;
    }
    if (itemId == "paste") {
        platform::executeWebViewScript(self->getWebView()->getNativeHandle(), "document.execCommand('paste');");
        return;
    }
    if (itemId == "selectAll") {
        platform::executeWebViewScript(self->getWebView()->getNativeHandle(), "document.execCommand('selectAll');");
        return;
    }
    // View menu: Reload
    if (itemId == "reload") {
        platform::executeWebViewScript(self->getWebView()->getNativeHandle(), "location.reload();");
        return;
    }
    // View menu: Inspect Element - open developer tools
    if (itemId == "inspect") {
        platform::openInspector(self->getWebView()->getNativeHandle());
        return;
    }
    // File menu: Print - print the focused window's content, else main window
    if (itemId == "print") {
        WebViewWindow* target = g_focusedWebViewForPrint;
        if (!target || !target->getWebView()) target = self;
        platform::printWebView(target->getWebView()->getNativeHandle());
        return;
    }
    // View menu: Settings - open embedded options editor (local HTML, not from server)
    if (itemId == "settings") {
        std::string script = R"(
(function(){
  if(typeof window.CrossDev==='undefined'||!window.CrossDev.invoke)return;
  window.CrossDev.invoke('createWindow',{className:'settings',title:'Settings',isSingleton:true}).catch(function(e){console.error('Failed to open Settings:',e);});
})();
)";
        platform::executeWebViewScript(self->getWebView()->getNativeHandle(), script);
        return;
    }
    // Zoom: apply via document.documentElement.style.zoom
    if (itemId == "zoomIn" || itemId == "zoomOut" || itemId == "zoomReset") {
        std::string script =
            "(function(){"
            "var el=document.documentElement;"
            "var z=parseFloat(el.style.zoom||el.getAttribute('data-zoom')||'1');"
            "if('" + itemId + "'==='zoomIn')z=Math.min(2,z+0.1);"
            "else if('" + itemId + "'==='zoomOut')z=Math.max(0.5,z-0.1);"
            "else z=1;"
            "el.style.zoom=z;"
            "el.setAttribute('data-zoom',String(z));"
            "})();";
        platform::executeWebViewScript(self->getWebView()->getNativeHandle(), script);
        return;
    }
    // File, Help: post to JS for app to handle
    self->postMessageToJavaScript("{\"type\":\"menu\",\"id\":\"" + itemId + "\"}");
}
}

void WebViewWindow::registerMainMenu() {
    if (!window_ || !window_->getNativeHandle()) return;
#ifdef PLATFORM_MACOS
    // macOS: app menu first (named after the app), with Preferences and Quit per HIG
    std::string appLabel = window_->getTitle().empty() ? "Cars" : window_->getTitle();
    using json = nlohmann::json;
    json appItems = json::array();
    appItems.push_back({{"id", "about"}, {"label", "About"}});
    appItems.push_back({{"id", "settings"}, {"label", "Preferences..."}, {"shortcut", "Cmd+,"}});
    appItems.push_back({{"id", "-"}});
    appItems.push_back({{"id", "quit"}, {"label", "Quit"}, {"shortcut", "Cmd+Q"}});
    json fileItems = json::array();
    fileItems.push_back({{"id", "new"}, {"label", "New"}, {"shortcut", "Cmd+N"}});
    fileItems.push_back({{"id", "-"}});
    fileItems.push_back({{"id", "open"}, {"label", "Open"}, {"shortcut", "Cmd+O"}});
    fileItems.push_back({{"id", "save"}, {"label", "Save"}, {"shortcut", "Cmd+S"}});
    fileItems.push_back({{"id", "saveAs"}, {"label", "Save As..."}, {"shortcut", "Cmd+Shift+S"}});
    fileItems.push_back({{"id", "-"}});
    fileItems.push_back({{"id", "print"}, {"label", "Print..."}, {"shortcut", "Cmd+P"}});
    json editItems = json::array();
    editItems.push_back({{"id", "undo"}, {"label", "Undo"}, {"shortcut", "Cmd+Z"}});
    editItems.push_back({{"id", "redo"}, {"label", "Redo"}, {"shortcut", "Cmd+Shift+Z"}});
    editItems.push_back({{"id", "-"}});
    editItems.push_back({{"id", "cut"}, {"label", "Cut"}, {"shortcut", "Cmd+X"}});
    editItems.push_back({{"id", "copy"}, {"label", "Copy"}, {"shortcut", "Cmd+C"}});
    editItems.push_back({{"id", "paste"}, {"label", "Paste"}, {"shortcut", "Cmd+V"}});
    editItems.push_back({{"id", "-"}});
    editItems.push_back({{"id", "selectAll"}, {"label", "Select All"}, {"shortcut", "Cmd+A"}});
    json viewItems = json::array();
    viewItems.push_back({{"id", "reload"}, {"label", "Reload"}, {"shortcut", "Cmd+R"}});
    viewItems.push_back({{"id", "inspect"}, {"label", "Inspect Element"}, {"shortcut", "Cmd+Option+I"}});
    viewItems.push_back({{"id", "zoomIn"}, {"label", "Zoom In"}, {"shortcut", "Cmd++"}});
    viewItems.push_back({{"id", "zoomOut"}, {"label", "Zoom Out"}, {"shortcut", "Cmd+-"}});
    viewItems.push_back({{"id", "zoomReset"}, {"label", "Reset Zoom"}, {"shortcut", "Cmd+0"}});
    json helpItems = json::array();
    helpItems.push_back({{"id", "about"}, {"label", "About"}});
    helpItems.push_back({{"id", "-"}});
    helpItems.push_back({{"id", "docs"}, {"label", "Documentation"}});
    json menus = json::array();
    menus.push_back({{"id", "app"}, {"label", appLabel}, {"items", appItems}});
    menus.push_back({{"id", "file"}, {"label", "File"}, {"items", fileItems}});
    menus.push_back({{"id", "edit"}, {"label", "Edit"}, {"items", editItems}});
    menus.push_back({{"id", "view"}, {"label", "View"}, {"items", viewItems}});
    menus.push_back({{"id", "help"}, {"label", "Help"}, {"items", helpItems}});
    std::string menuJson = menus.dump();
    window_->setMainMenu(menuJson, onMainMenuItem, this);
#elif defined(PLATFORM_WINDOWS)
    // Windows: Settings under Edit (Preferences convention)
    const char* menuJson = R"([
        {"id":"file","label":"File","items":[
            {"id":"new","label":"New","shortcut":"Ctrl+N"},
            {"id":"-"},
            {"id":"open","label":"Open","shortcut":"Ctrl+O"},
            {"id":"save","label":"Save","shortcut":"Ctrl+S"},
            {"id":"saveAs","label":"Save As...","shortcut":"Ctrl+Shift+S"},
            {"id":"-"},
            {"id":"print","label":"Print...","shortcut":"Ctrl+P"},
            {"id":"-"},
            {"id":"quit","label":"Quit","shortcut":"Ctrl+Q"}
        ]},
        {"id":"edit","label":"Edit","items":[
            {"id":"undo","label":"Undo","shortcut":"Ctrl+Z"},
            {"id":"redo","label":"Redo","shortcut":"Ctrl+Shift+Z"},
            {"id":"-"},
            {"id":"cut","label":"Cut","shortcut":"Ctrl+X"},
            {"id":"copy","label":"Copy","shortcut":"Ctrl+C"},
            {"id":"paste","label":"Paste","shortcut":"Ctrl+V"},
            {"id":"-"},
            {"id":"selectAll","label":"Select All","shortcut":"Ctrl+A"},
            {"id":"-"},
            {"id":"settings","label":"Preferences...","shortcut":"Ctrl+,"}
        ]},
        {"id":"view","label":"View","items":[
            {"id":"reload","label":"Reload","shortcut":"Ctrl+R"},
            {"id":"inspect","label":"Inspect Element","shortcut":"Ctrl+Shift+I"},
            {"id":"zoomIn","label":"Zoom In","shortcut":"Ctrl++"},
            {"id":"zoomOut","label":"Zoom Out","shortcut":"Ctrl+-"},
            {"id":"zoomReset","label":"Reset Zoom","shortcut":"Ctrl+0"}
        ]},
        {"id":"help","label":"Help","items":[
            {"id":"about","label":"About"},
            {"id":"-"},
            {"id":"docs","label":"Documentation"}
        ]}
    ])";
    window_->setMainMenu(menuJson, onMainMenuItem, this);
#else
    // Linux: Settings under Edit (Preferences convention)
    const char* menuJson = R"([
        {"id":"file","label":"File","items":[
            {"id":"new","label":"New","shortcut":"Ctrl+N"},
            {"id":"-"},
            {"id":"open","label":"Open","shortcut":"Ctrl+O"},
            {"id":"save","label":"Save","shortcut":"Ctrl+S"},
            {"id":"saveAs","label":"Save As...","shortcut":"Ctrl+Shift+S"},
            {"id":"-"},
            {"id":"print","label":"Print...","shortcut":"Ctrl+P"},
            {"id":"-"},
            {"id":"quit","label":"Quit","shortcut":"Ctrl+Q"}
        ]},
        {"id":"edit","label":"Edit","items":[
            {"id":"undo","label":"Undo","shortcut":"Ctrl+Z"},
            {"id":"redo","label":"Redo","shortcut":"Ctrl+Shift+Z"},
            {"id":"-"},
            {"id":"cut","label":"Cut","shortcut":"Ctrl+X"},
            {"id":"copy","label":"Copy","shortcut":"Ctrl+C"},
            {"id":"paste","label":"Paste","shortcut":"Ctrl+V"},
            {"id":"-"},
            {"id":"selectAll","label":"Select All","shortcut":"Ctrl+A"},
            {"id":"-"},
            {"id":"settings","label":"Preferences...","shortcut":"Ctrl+,"}
        ]},
        {"id":"view","label":"View","items":[
            {"id":"reload","label":"Reload","shortcut":"Ctrl+R"},
            {"id":"inspect","label":"Inspect Element","shortcut":"Ctrl+Shift+I"},
            {"id":"zoomIn","label":"Zoom In","shortcut":"Ctrl++"},
            {"id":"zoomOut","label":"Zoom Out","shortcut":"Ctrl+-"},
            {"id":"zoomReset","label":"Reset Zoom","shortcut":"Ctrl+0"}
        ]},
        {"id":"help","label":"Help","items":[
            {"id":"about","label":"About"},
            {"id":"-"},
            {"id":"docs","label":"Documentation"}
        ]}
    ])";
    window_->setMainMenu(menuJson, onMainMenuItem, this);
#endif
}

void WebViewWindow::setMainMenu(const std::string& menuJson,
                                void (*itemCallback)(const std::string& itemId, void* userData),
                                void* userData) {
    if (window_ && itemCallback) {
        window_->setMainMenu(menuJson, itemCallback, userData);
    }
}

void WebViewWindow::registerMainWindowCloseCallback() {
    if (window_ && window_->getNativeHandle() && !GetOwner()) {
        platform::setWindowCloseCallback(
            window_->getNativeHandle(),
            [](void* userData) {
                WebViewWindow* mainWin = static_cast<WebViewWindow*>(userData);
                if (mainWin) {
                    if (mainWin->getWebView()) {
                        NativeEventBus::getInstance().emitTo(mainWin->getWebView(), "window:close-request", "{}");
                        NativeEventBus::getInstance().emitTo(mainWin->getWebView(), "window:close", "{}");
                        NativeEventBus::getInstance().emitToAll("app:quit", "{}");
                    }
                    // Destroy children while run loop is active (faster WebKit teardown)
                    mainWin->closeAllOwnedWebViewWindows();
                }
                Application::getInstance().quit();
            },
            this
        );
    }
}
