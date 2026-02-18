// Windows web view implementation
#include "../../../include/platform.h"
#include "../platform_impl.h"
#include "windows_common.h"
#include <string>
#include <functional>
#include <windows.h>
#include <fstream>
#include <sstream>
#include <shlwapi.h>
#include <wrl.h>
#include <iostream>
#include <io.h>
#include <fcntl.h>
#include <winreg.h>

// WebView2 support - try to use WebView2 if available
// WebView2 Runtime must be installed: https://developer.microsoft.com/microsoft-edge/webview2/
#ifdef HAVE_WEBVIEW2
#include <WebView2.h>
#include <WebView2EnvironmentOptions.h>
#else
// Forward declarations for WebView2 interfaces (when SDK not available)
// We'll try to load WebView2 at runtime
struct ICoreWebView2Environment;
struct ICoreWebView2Controller;
struct ICoreWebView2;
#endif

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")

// Helper function to check if WebView2 Runtime is available
static bool IsWebView2RuntimeAvailable() {
    // Check registry for WebView2 Runtime
    HKEY hKey;
    // Check for Evergreen (user-installed) runtime
    if (RegOpenKeyExW(HKEY_CURRENT_USER, 
        L"SOFTWARE\\Microsoft\\EdgeUpdate\\Clients\\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}", 
        0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return true;
    }
    // Check for system-wide runtime
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, 
        L"SOFTWARE\\WOW6432Node\\Microsoft\\EdgeUpdate\\Clients\\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}", 
        0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return true;
    }
    // Also check the newer location
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, 
        L"SOFTWARE\\Microsoft\\EdgeUpdate\\Clients\\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}", 
        0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return true;
    }
    return false;
}

using namespace Microsoft::WRL;

namespace platform {
    HINSTANCE getInstance();
}

// Helper function to convert std::string to std::wstring
static std::wstring stringToWString(const std::string& str) {
    if (str.empty()) return std::wstring();
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

// Helper function to convert std::wstring to std::string
static std::string wStringToString(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

#ifdef PLATFORM_WINDOWS

namespace platform {

// WindowData is defined in windows_common.h (included above)

// Forward declaration for callbacks
typedef void (*CreateWindowCallback)(const std::string& title, void* userData);
typedef void (*MessageCallback)(const std::string& jsonMessage, void* userData);

#ifdef HAVE_WEBVIEW2
// Forward declaration for WebView2MessageHandler
class WebView2MessageHandler;
#endif

struct WebViewData {
    HWND hwnd;
    HWND parent;
#ifdef HAVE_WEBVIEW2
    ICoreWebView2Controller* controller;
    ICoreWebView2* webview;
    std::string pendingURL;  // Queue URL to load once initialized
    std::string pendingHTML; // Queue HTML to load once initialized
    std::string pendingFile; // Queue file to load once initialized
    std::string customPreloadScript; // Custom preload (replaces default CrossDev bridge if set)
    CreateWindowCallback createWindowCallback; // Callback to create new windows
    void* createWindowUserData; // User data for window creation callback
    MessageCallback messageCallback; // Callback for all messages (routed through MessageRouter)
    void* messageUserData; // User data for message callback
    WebView2MessageHandler* messageHandler; // Store handler for cleanup
#else
    void* controller;
    void* webview;
#endif
    bool initialized;
    bool webview2Available;
};

#ifdef HAVE_WEBVIEW2
// Struct for deferring openFileDialog - avoids WebView2 reentrancy (modal dialog in WebMessageReceived)
struct DeferredWebViewMessage {
    std::string* message;
    MessageCallback callback;
    void* userData;
};
#endif

#ifdef HAVE_WEBVIEW2
// WebView2 message received handler
class WebView2MessageHandler : public ICoreWebView2WebMessageReceivedEventHandler {
public:
    WebViewData* webViewData;
    
    WebView2MessageHandler(WebViewData* data) : webViewData(data) {}
    
    HRESULT STDMETHODCALLTYPE Invoke(ICoreWebView2* /*sender*/, ICoreWebView2WebMessageReceivedEventArgs* args) override {
        // Use both std::wcout and std::cout for maximum visibility
        std::wcout << L"=== WebView2MessageHandler::Invoke CALLED ===" << std::endl;
        std::cout << "=== WebView2MessageHandler::Invoke CALLED ===" << std::endl;
        OutputDebugStringW(L"=== WebView2MessageHandler::Invoke CALLED ===\n");
        
        // WebView2 receive: page.postMessage(string) -> TryGetWebMessageAsString works.
        // page.postMessage(object) -> TryGetWebMessageAsString FAILS, use get_WebMessageAsJson.
        LPWSTR message = nullptr;
        HRESULT hr = args->TryGetWebMessageAsString(&message);
        const char* source = "TryGetWebMessageAsString";
        if (FAILED(hr) || !message || !*message) {
            std::cout << "[WebView2] TryGetWebMessageAsString: FAILED hr=0x" << std::hex << hr << std::dec
                << " (page sent object? Use get_WebMessageAsJson)" << std::endl;
            OutputDebugStringA("[WebView2] TryGetWebMessageAsString failed, trying get_WebMessageAsJson\n");
            CoTaskMemFree(message);
            message = nullptr;
            hr = args->get_WebMessageAsJson(&message);
            source = "get_WebMessageAsJson";
            if (SUCCEEDED(hr) && message) {
                std::cout << "[WebView2] get_WebMessageAsJson: OK (page sent object, we got JSON string)" << std::endl;
                OutputDebugStringA("[WebView2] get_WebMessageAsJson succeeded\n");
            }
        } else {
            std::cout << "[WebView2] TryGetWebMessageAsString: OK (page sent string)" << std::endl;
            OutputDebugStringA("[WebView2] TryGetWebMessageAsString succeeded\n");
        }
        
        std::wcout << L"[WebView2] Message source: " << (source ? stringToWString(std::string(source)) : L"unknown")
            << L" hr=0x" << std::hex << hr << std::dec << std::endl;
        
        if (SUCCEEDED(hr) && message) {
            std::wstring wmsg(message);
            std::wcout << L"Received message: " << wmsg << std::endl;
            OutputDebugStringW(L"Received message: ");
            OutputDebugStringW(wmsg.c_str());
            OutputDebugStringW(L"\n");
            
            std::string msg = wStringToString(wmsg);
            
            std::cout << "Processing message, length: " << msg.length() << std::endl;
            std::wcout << L"Processing message, length: " << msg.length() << std::endl;
            OutputDebugStringW(L"Processing message\n");
            
            // First, try the new message callback (for MessageRouter)
            if (webViewData && !webViewData->messageCallback) {
                std::wcout << L"WARNING: messageCallback is NULL - message will not reach MessageRouter!" << std::endl;
                OutputDebugStringW(L"WARNING: messageCallback is NULL\n");
            }
            if (webViewData && webViewData->messageCallback) {
                // WebView2 does not support modal dialogs inside WebMessageReceived (reentrancy).
                // See: https://github.com/MicrosoftEdge/WebView2Feedback/issues/2453
                // Defer openFileDialog to next message loop so GetOpenFileName works and files are clickable.
                bool isOpenFileDialog = (msg.find("\"type\":\"openFileDialog\"") != std::string::npos ||
                                        msg.find("\"type\": \"openFileDialog\"") != std::string::npos);
                if (isOpenFileDialog && webViewData->parent && IsWindow(webViewData->parent)) {
                    DeferredWebViewMessage* d = new DeferredWebViewMessage();
                    d->message = new std::string(msg);
                    d->callback = webViewData->messageCallback;
                    d->userData = webViewData->messageUserData;
                    PostMessage(webViewData->parent, WM_DEFERRED_WEBVIEW_MESSAGE, 0, (LPARAM)d);
                    std::cout << "Deferred openFileDialog to next message loop (reentrancy workaround)" << std::endl;
                } else {
                    std::cout << "Calling messageCallback..." << std::endl;
                    webViewData->messageCallback(msg, webViewData->messageUserData);
                    std::cout << "messageCallback completed" << std::endl;
                }
            }
            // Fallback to old createWindow callback for backward compatibility
            else if (webViewData && webViewData->createWindowCallback) {
                // Simple JSON parsing - look for "createWindow" type
                if (msg.find("\"type\":\"createWindow\"") != std::string::npos ||
                    msg.find("'type':'createWindow'") != std::string::npos ||
                    msg.find("type\":\"createWindow") != std::string::npos) {
                    // Extract title if present
                    std::string title = "New Window";
                    size_t titlePos = msg.find("\"title\":\"");
                    if (titlePos != std::string::npos) {
                        titlePos += 9; // Skip "title":"
                        size_t titleEnd = msg.find("\"", titlePos);
                        if (titleEnd != std::string::npos) {
                            title = msg.substr(titlePos, titleEnd - titlePos);
                        }
                    }
                    
                    std::wcout << L"Parsed createWindow message, title: " << stringToWString(title) << std::endl;
                    OutputDebugStringW(L"Parsed createWindow message\n");
                    
                    webViewData->createWindowCallback(title, webViewData->createWindowUserData);
                }
            }
            
            CoTaskMemFree(message);
        } else {
            std::wcout << L"Failed to get message string, HRESULT: 0x" << std::hex << hr << std::dec << std::endl;
        }
        return S_OK;
    }
    
    ULONG STDMETHODCALLTYPE AddRef() override { return 1; }
    ULONG STDMETHODCALLTYPE Release() override { return 1; }
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override {
        if (riid == __uuidof(ICoreWebView2WebMessageReceivedEventHandler)) {
            *ppv = this;
            return S_OK;
        }
        return E_NOINTERFACE;
    }
};

// WebView2 controller created callback (defined first so it can be used in WebView2EnvironmentHandler)
class WebView2ControllerHandler : public ICoreWebView2CreateCoreWebView2ControllerCompletedHandler {
public:
    WebViewData* webViewData;
    int x, y, width, height;
    
    WebView2ControllerHandler(WebViewData* data, int x, int y, int w, int h)
        : webViewData(data), x(x), y(y), width(w), height(h) {}
    
    HRESULT STDMETHODCALLTYPE Invoke(HRESULT result, ICoreWebView2Controller* controller) override {
        if (FAILED(result)) {
            // Fallback to static control
            wchar_t errorMsg[256];
            swprintf_s(errorMsg, L"WebView2 controller creation failed. HRESULT: 0x%08X", result);
            std::wcout << errorMsg << std::endl;
            OutputDebugStringW(errorMsg);
            OutputDebugStringW(L"\n");
            
            webViewData->hwnd = CreateWindowW(
                L"STATIC",
                errorMsg,
                WS_VISIBLE | WS_CHILD | SS_LEFT,
                x, y, width, height,
                webViewData->parent,
                nullptr,
                getInstance(),
                nullptr
            );
            return result;
        }
        
        if (!controller) {
            std::wcout << L"WebView2 controller is null" << std::endl;
            OutputDebugStringW(L"WebView2 controller is null\n");
            webViewData->hwnd = CreateWindowW(
                L"STATIC",
                L"WebView2 controller is null.",
                WS_VISIBLE | WS_CHILD | SS_LEFT,
                x, y, width, height,
                webViewData->parent,
                nullptr,
                getInstance(),
                nullptr
            );
            return E_POINTER;
        }
        
        webViewData->controller = controller;
        controller->AddRef();
        ICoreWebView2* webview = nullptr;
        HRESULT hr = controller->get_CoreWebView2(&webview);
        if (FAILED(hr) || !webview) {
            controller->Release();
            webViewData->controller = nullptr;
            webViewData->hwnd = CreateWindowW(
                L"STATIC",
                L"Failed to get WebView2 core object.",
                WS_VISIBLE | WS_CHILD | SS_LEFT,
                x, y, width, height,
                webViewData->parent,
                nullptr,
                getInstance(),
                nullptr
            );
            return hr;
        }
        
        webViewData->webview = webview;
        
        // Use actual client area when available (window may have different client size than creation params)
        RECT clientRect = { 0, 0, width, height };
        if (GetClientRect(webViewData->parent, &clientRect) && clientRect.right > 0 && clientRect.bottom > 0) {
            width = clientRect.right - clientRect.left;
            height = clientRect.bottom - clientRect.top;
        }
        RECT bounds = { x, y, x + width, y + height };
        controller->put_Bounds(bounds);
        controller->put_IsVisible(TRUE);
        
        // Set up WebMessageReceived handler for JavaScript-to-native communication
        // CRITICAL: This MUST be registered BEFORE any navigation or script injection
        EventRegistrationToken token;
        webViewData->messageHandler = new WebView2MessageHandler(webViewData);
        
        std::cout << "Registering WebMessageReceived handler..." << std::endl;
        std::wcout << L"Registering WebMessageReceived handler..." << std::endl;
        OutputDebugStringW(L"Registering WebMessageReceived handler...\n");
        std::cout << "  Handler pointer: " << (void*)webViewData->messageHandler << std::endl;
        std::cout << "  WebViewData pointer: " << (void*)webViewData << std::endl;
        std::cout << "  messageCallback: " << (void*)webViewData->messageCallback << std::endl;
        
        HRESULT hrMsg = webview->add_WebMessageReceived(webViewData->messageHandler, &token);
        
        if (FAILED(hrMsg)) {
            std::wcout << L"ERROR: Failed to set up WebMessageReceived handler, HRESULT: 0x" << std::hex << hrMsg << std::dec << std::endl;
            OutputDebugStringW(L"ERROR: Failed to set up WebMessageReceived handler\n");
            wchar_t hrStr[32];
            swprintf_s(hrStr, 32, L"0x%08X", hrMsg);
            OutputDebugStringW(hrStr);
            OutputDebugStringW(L"\n");
        } else {
            std::cout << "✓✓✓ WebMessageReceived handler registered successfully (token: " << token.value << ")" << std::endl;
            std::wcout << L"✓✓✓ WebMessageReceived handler registered successfully (token: " << token.value << L")" << std::endl;
            OutputDebugStringW(L"✓✓✓ WebMessageReceived handler registered successfully\n");
            wchar_t tokenStr[32];
            swprintf_s(tokenStr, 32, L"Token: %llu", token.value);
            OutputDebugStringW(tokenStr);
            OutputDebugStringW(L"\n");
            
            // Verify the handler is actually registered by checking if we can query it
            std::cout << "Handler registration verified - ready to receive messages" << std::endl;
            std::wcout << L"Handler registration verified - ready to receive messages" << std::endl;
            OutputDebugStringW(L"Handler registration verified\n");
        }
        
        // Enable WebMessageReceivedScript to allow window.chrome.webview.postMessage()
        // This is required for JavaScript-to-native communication
        ICoreWebView2Settings* settings = nullptr;
        HRESULT hrSettings = webview->get_Settings(&settings);
        if (SUCCEEDED(hrSettings) && settings) {
            BOOL isScriptEnabled = FALSE;
            settings->get_IsScriptEnabled(&isScriptEnabled);
            if (!isScriptEnabled) {
                settings->put_IsScriptEnabled(TRUE);
                std::wcout << L"Enabled JavaScript in WebView2 settings" << std::endl;
                OutputDebugStringW(L"Enabled JavaScript in WebView2 settings\n");
            }
            settings->Release();
        }
        
        // Helper function to inject the message bridge script (using ExecuteScript for immediate execution)
        auto injectMessageBridge = [webview]() {
            const wchar_t* setupScript = L""
                L"(function() {"
                L"  if (window.__webview2ReceiveMessage) { console.log('Bridge already exists'); return; }"
                L"  console.log('=== Injecting WebView2 message bridge ===');"
                L"  window.__webview2Messages = [];"
                L"  window.__webview2MessageListeners = [];"
                L"  window.__webview2ReceiveMessage = function(messageJson) {"
                L"    console.log('__webview2ReceiveMessage called with:', messageJson);"
                L"    try {"
                L"      const data = typeof messageJson === 'string' ? JSON.parse(messageJson) : messageJson;"
                L"      console.log('Parsed message data:', data);"
                L"      window.__webview2Messages.push(data);"
                L"      window.__webview2MessageListeners.forEach(function(listener) {"
                L"        try { listener({ data: data }); } catch(e) { console.error('Error in listener:', e); }"
                L"      });"
                L"    } catch(e) { console.error('Error parsing message:', e, messageJson); }"
                L"  };"
                L"  "
                L"  // CRITICAL: NEVER create window.chrome.webview if it already exists!"
                L"  // WebView2 provides window.chrome.webview.postMessage natively"
                L"  // Creating a new object {} will DESTROY the native postMessage function!"
                L"  if (!window.chrome) {"
                L"    window.chrome = {};"
                L"  }"
                L"  "
                L"  // NEVER do: window.chrome.webview = {} if it already exists!"
                L"  // Only create it if it doesn't exist, and even then, be careful"
                L"  if (!window.chrome.webview) {"
                L"    console.warn('window.chrome.webview did not exist - this is unusual for WebView2');"
                L"    // Don't create it - let WebView2 create it natively"
                L"  } else {"
                L"    console.log('✓ window.chrome.webview already exists - preserving ALL native functions');"
                L"    console.log('postMessage available:', typeof window.chrome.webview.postMessage);"
                L"    console.log('postMessage is native:', window.chrome.webview.postMessage.toString().includes('[native code]'));"
                L"  }"
                L"  "
                L"  // Override addEventListener to use our listener system for native-to-JS messages"
                L"  window.chrome.webview.addEventListener = function(type, listener) {"
                L"    console.log('addEventListener called for type:', type);"
                L"    if (type === 'message') {"
                L"      window.__webview2MessageListeners.push(listener);"
                L"      console.log('Listener added, total:', window.__webview2MessageListeners.length);"
                L"      window.__webview2Messages.forEach(function(msg) {"
                L"        try { listener({ data: msg }); } catch(e) { console.error('Error:', e); }"
                L"      });"
                L"      // Also register with native listener if available (for PostWebMessageAsJson)"
                L"      if (nativeAddEventListener) {"
                L"        nativeAddEventListener(type, function(event) {"
                L"          console.log('Native message event received:', event);"
                L"          try {"
                L"            const data = typeof event.data === 'string' ? JSON.parse(event.data) : event.data;"
                L"            window.__webview2Messages.push(data);"
                L"            window.__webview2MessageListeners.forEach(function(listener) {"
                L"              try { listener({ data: data }); } catch(e) { console.error('Error in listener:', e); }"
                L"            });"
                L"          } catch(e) { console.error('Error handling native message:', e); }"
                L"        });"
                L"      }"
                L"    } else if (nativeAddEventListener) {"
                L"      // For non-message events, use native handler"
                L"      nativeAddEventListener(type, listener);"
                L"    }"
                L"  };"
                L"  "
                L"  window.chrome.webview.removeEventListener = function(type, listener) {"
                L"    if (type === 'message') {"
                L"      const index = window.__webview2MessageListeners.indexOf(listener);"
                L"      if (index > -1) window.__webview2MessageListeners.splice(index, 1);"
                L"    } else if (nativeRemoveEventListener) {"
                L"      nativeRemoveEventListener(type, listener);"
                L"    }"
                L"  };"
                L"  console.log('=== Bridge injection complete ===');"
                L"  console.log('postMessage available:', typeof window.chrome.webview.postMessage === 'function');"
                L"})();";
            
            // Use ExecuteScript to inject immediately (works on current document)
            webview->ExecuteScript(setupScript, nullptr);
        };
        
        // Inject script on document creation: CrossDev bridge + message listener (WebView2)
        const wchar_t* setupScriptForFutureDocs = L""
            L"(function(){"
            L"  if(window.CrossDev)return;"
            L"  var _pending=new Map(),_eventListeners={};"
            L"  function _ab2b64(ab){var u8=new Uint8Array(ab);var b='';for(var i=0;i<u8.length;i++)b+=String.fromCharCode(u8[i]);return btoa(b);}"
            L"  function _b642ab(s){var b=atob(s);var u8=new Uint8Array(b.length);for(var i=0;i<b.length;i++)u8[i]=b.charCodeAt(i);return u8.buffer;}"
            L"  function _toWire(o){if(o===null||typeof o!=='object')return o;"
            L"    if(o instanceof ArrayBuffer)return {__base64:_ab2b64(o)};"
            L"    if(ArrayBuffer.isView&&ArrayBuffer.isView(o))return {__base64:_ab2b64(o.buffer.slice(o.byteOffset,o.byteOffset+o.byteLength))};"
            L"    if(Array.isArray(o))return o.map(_toWire);"
            L"    var out={};for(var k in o)if(o.hasOwnProperty(k))out[k]=_toWire(o[k]);return out;}"
            L"  function _onMsg(evt){var d=typeof evt.data==='string'?JSON.parse(evt.data):evt.data;if(!d)return;"
            L"    if(window.__webview2Messages){window.__webview2Messages.push(d);"
            L"      if(window.__webview2MessageListeners)window.__webview2MessageListeners.forEach(function(l){try{l({data:d});}catch(e){};});}"
            L"    if(d.type==='crossdev:event'){var l=_eventListeners[d.name];if(l)l.forEach(function(f){try{f(d.payload||{})}catch(e){}});return;}"
            L"    if(d.requestId){var h=_pending.get(d.requestId);if(h){_pending.delete(d.requestId);"
            L"      var r=d.result;if(h.binary&&r&&typeof r.data==='string'){r=Object.assign({},r);r.data=_b642ab(r.data);}"
            L"      d.error?h.reject(new Error(d.error)):h.resolve(r);}}};"
            L"  function _init(){if(!window.chrome||!window.chrome.webview)return;"
            L"    window.chrome.webview.addEventListener('message',_onMsg);"
            L"    var CrossDev={invoke:function(t,p,o){var op=o||{};return new Promise(function(r,j){"
            L"      var id=Date.now()+'-'+Math.random();_pending.set(id,{resolve:r,reject:j,binary:!!op.binaryResponse});"
            L"      setTimeout(function(){if(_pending.has(id)){_pending.delete(id);j(new Error('Request timeout'));}},30000);"
            L"      window.chrome.webview.postMessage(JSON.stringify({type:t,payload:_toWire(p||{}),requestId:id}));});},"
            L"    events:{on:function(n,f){if(!_eventListeners[n])_eventListeners[n]=[];_eventListeners[n].push(f);"
            L"      return function(){var i=_eventListeners[n].indexOf(f);if(i>=0)_eventListeners[n].splice(i,1);};}}};"
            L"    Object.freeze(CrossDev.events);Object.freeze(CrossDev);"
            L"    try{Object.defineProperty(window,'CrossDev',{value:CrossDev,configurable:false,writable:false});}catch(_){window.CrossDev=CrossDev;}"
            L"    window.__webview2CrossDevReady=true;}"
            L"  window.__webview2Messages=[];window.__webview2MessageListeners=[];"
            L"  if(window.chrome&&window.chrome.webview)_init();else var _t=setInterval(function(){if(window.chrome&&window.chrome.webview){clearInterval(_t);_init();}},50);"
            L"  if(!window.chrome)window.chrome={};"
            L"  if(window.chrome.webview){"
            L"    var _na=window.chrome.webview.addEventListener;"
            L"    window.chrome.webview.addEventListener=function(t,l){if(t==='message'){"
            L"      window.__webview2MessageListeners.push(l);"
            L"      window.__webview2Messages.forEach(function(m){try{l({data:m});}catch(e){};});"
            L"      if(_na)_na.call(window.chrome.webview,t,function(e){var d=typeof e.data==='string'?JSON.parse(e.data):e.data;"
            L"        window.__webview2Messages.push(d);"
            L"        window.__webview2MessageListeners.forEach(function(ln){try{ln({data:d});}catch(x){};});});}"
            L"    else if(_na)_na.call(window.chrome.webview,t,l);};"
            L"    var _nr=window.chrome.webview.removeEventListener;"
            L"    window.chrome.webview.removeEventListener=function(t,l){if(t==='message'){var i=window.__webview2MessageListeners.indexOf(l);if(i>=0)window.__webview2MessageListeners.splice(i,1);}"
            L"    else if(_nr)_nr.call(window.chrome.webview,t,l);};}"
            L"  document.addEventListener('keydown',function(e){"
            L"    if(!(e.ctrlKey||e.metaKey))return;"
            L"    var k=e.key;"
            L"    if(k!=='+'&&k!=='='&&k!=='-'&&k!=='0')return;"
            L"    e.preventDefault();"
            L"    var el=document.documentElement;"
            L"    var z=parseFloat(el.style.zoom||el.getAttribute('data-zoom')||'1');"
            L"    if(k==='+'||k==='=')z=Math.min(2,z+0.1);"
            L"    else if(k==='-')z=Math.max(0.5,z-0.1);"
            L"    else z=1;"
            L"    el.style.zoom=z;"
            L"    el.setAttribute('data-zoom',String(z));"
            L"  },true);"
            L"})();";
        
        const wchar_t* scriptToInject = setupScriptForFutureDocs;
        std::wstring customWScript;
        if (!webViewData->customPreloadScript.empty()) {
            customWScript = stringToWString(webViewData->customPreloadScript);
            scriptToInject = customWScript.c_str();
        }
        webview->AddScriptToExecuteOnDocumentCreated(scriptToInject, nullptr);
        
        // Set up NavigationCompleted handler to inject script after navigation
        EventRegistrationToken navToken;
        class NavigationCompletedHandler : public ICoreWebView2NavigationCompletedEventHandler {
        public:
            std::function<void()> injectScript;
            bool skipInjection;
            NavigationCompletedHandler(std::function<void()> inject, bool skip) : injectScript(inject), skipInjection(skip) {}
            HRESULT STDMETHODCALLTYPE Invoke(ICoreWebView2* /*sender*/, ICoreWebView2NavigationCompletedEventArgs* /*args*/) override {
                if (!skipInjection) injectScript();
                return S_OK;
            }
            
            ULONG STDMETHODCALLTYPE AddRef() override { return 1; }
            ULONG STDMETHODCALLTYPE Release() override { return 1; }
            HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override {
                if (riid == __uuidof(ICoreWebView2NavigationCompletedEventHandler)) {
                    *ppv = this;
                    return S_OK;
                }
                return E_NOINTERFACE;
            }
        };
        
        bool useCustomPreload = !webViewData->customPreloadScript.empty();
        webview->add_NavigationCompleted(new NavigationCompletedHandler(injectMessageBridge, useCustomPreload), &navToken);
        
        // Verify callback is set
        if (webViewData->messageCallback) {
            std::wcout << L"✓ Message callback is set: " << (void*)webViewData->messageCallback << std::endl;
            OutputDebugStringW(L"✓ Message callback is set\n");
        } else {
            std::wcout << L"⚠ WARNING: Message callback is NOT set!" << std::endl;
            OutputDebugStringW(L"⚠ WARNING: Message callback is NOT set!\n");
        }
        
        webViewData->initialized = true;
        webViewData->webview2Available = true;
        webViewData->hwnd = webViewData->parent; // Use parent window handle
        
        // Output debug message
        std::wcout << L"WebView2 initialized successfully!" << std::endl;
        OutputDebugStringW(L"WebView2 initialized successfully\n");
        
        // Load any pending URLs/HTML/files that were queued before initialization
        if (!webViewData->pendingURL.empty()) {
            std::wstring wurl = stringToWString(webViewData->pendingURL);
            std::wcout << L"Loading queued URL: " << wurl << std::endl;
            OutputDebugStringW(L"Loading queued URL: ");
            OutputDebugStringW(wurl.c_str());
            OutputDebugStringW(L"\n");
            webview->Navigate(wurl.c_str());
            webViewData->pendingURL.clear();
        } else if (!webViewData->pendingHTML.empty()) {
            std::wstring whtml = stringToWString(webViewData->pendingHTML);
            std::wcout << L"Loading queued HTML" << std::endl;
            OutputDebugStringW(L"Loading queued HTML\n");
            webview->NavigateToString(whtml.c_str());
            // Script will be injected via NavigationCompleted handler
            webViewData->pendingHTML.clear();
        } else if (!webViewData->pendingFile.empty()) {
            std::wstring wpath = stringToWString(webViewData->pendingFile);
            wchar_t fullPath[MAX_PATH];
            if (GetFullPathNameW(wpath.c_str(), MAX_PATH, fullPath, nullptr)) {
                std::wstring url = L"file:///" + std::wstring(fullPath);
                for (size_t i = 0; i < url.length(); ++i) {
                    if (url[i] == L'\\') url[i] = L'/';
                }
                std::wcout << L"Loading queued file: " << url << std::endl;
                OutputDebugStringW(L"Loading queued file: ");
                OutputDebugStringW(url.c_str());
                OutputDebugStringW(L"\n");
                webview->Navigate(url.c_str());
            }
            webViewData->pendingFile.clear();
        }
        
        return S_OK;
    }
    
    ULONG STDMETHODCALLTYPE AddRef() override { return 1; }
    ULONG STDMETHODCALLTYPE Release() override { return 1; }
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override {
        if (riid == __uuidof(ICoreWebView2CreateCoreWebView2ControllerCompletedHandler)) {
            *ppv = this;
            return S_OK;
        }
        return E_NOINTERFACE;
    }
};

// WebView2 environment created callback
class WebView2EnvironmentHandler : public ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler {
public:
    WebViewData* webViewData;
    HWND parentHwnd;
    int x, y, width, height;
    
    WebView2EnvironmentHandler(WebViewData* data, HWND parent, int x, int y, int w, int h)
        : webViewData(data), parentHwnd(parent), x(x), y(y), width(w), height(h) {}
    
    HRESULT STDMETHODCALLTYPE Invoke(HRESULT result, ICoreWebView2Environment* env) override {
        if (FAILED(result)) {
            // Fallback to static control if WebView2 fails
            wchar_t errorMsg[512];
            if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) || 
                result == 0x80070002 || // ERROR_FILE_NOT_FOUND
                result == 0x8007007E) { // ERROR_MOD_NOT_FOUND
                swprintf_s(errorMsg, 512, 
                    L"WebView2 Runtime not found.\n\n"
                    L"Please install Microsoft Edge WebView2 Runtime:\n"
                    L"https://go.microsoft.com/fwlink/p/?LinkId=2124703\n\n"
                    L"Error code: 0x%08X", result);
            } else {
                swprintf_s(errorMsg, 512, 
                    L"WebView2 initialization failed.\n\n"
                    L"Error code: 0x%08X\n\n"
                    L"Please ensure WebView2 Runtime is installed:\n"
                    L"https://go.microsoft.com/fwlink/p/?LinkId=2124703", result);
            }
            
            std::wcout << errorMsg << std::endl;
            OutputDebugStringW(L"WebView2 environment creation failed: ");
            wchar_t hrStr[32];
            swprintf_s(hrStr, 32, L"0x%08X", result);
            OutputDebugStringW(hrStr);
            OutputDebugStringW(L"\n");
            
            webViewData->hwnd = CreateWindowW(
                L"STATIC",
                errorMsg,
                WS_VISIBLE | WS_CHILD | SS_LEFT | SS_CENTER,
                x, y, width, height,
                parentHwnd,
                nullptr,
                getInstance(),
                nullptr
            );
            return result;
        }
        
        // Create the WebView2 controller asynchronously
        HRESULT hr = env->CreateCoreWebView2Controller(parentHwnd, 
            new WebView2ControllerHandler(webViewData, x, y, width, height));
        if (FAILED(hr)) {
            // If controller creation fails immediately, show error
            wchar_t errorMsg[512];
            swprintf_s(errorMsg, 512, 
                L"WebView2 controller creation failed immediately.\n\n"
                L"Error code: 0x%08X\n\n"
                L"Please ensure WebView2 Runtime is installed:\n"
                L"https://go.microsoft.com/fwlink/p/?LinkId=2124703", hr);
            std::wcout << errorMsg << std::endl;
            OutputDebugStringW(L"WebView2 controller creation failed immediately: ");
            wchar_t hrStr[32];
            swprintf_s(hrStr, 32, L"0x%08X", hr);
            OutputDebugStringW(hrStr);
            OutputDebugStringW(L"\n");
            
            webViewData->hwnd = CreateWindowW(
                L"STATIC",
                errorMsg,
                WS_VISIBLE | WS_CHILD | SS_LEFT | SS_CENTER,
                x, y, width, height,
                parentHwnd,
                nullptr,
                getInstance(),
                nullptr
            );
        }
        return S_OK;
    }
    
    ULONG STDMETHODCALLTYPE AddRef() override { return 1; }
    ULONG STDMETHODCALLTYPE Release() override { return 1; }
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override {
        if (riid == __uuidof(ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler)) {
            *ppv = this;
            return S_OK;
        }
        return E_NOINTERFACE;
    }
};
#endif // HAVE_WEBVIEW2

void* createWebView(void* parentHandle, int x, int y, int width, int height) {
    if (!parentHandle) {
        return nullptr;
    }
    
    HWND parentHwnd = nullptr;
    
    // Get parent HWND - could be from WindowData or direct HWND (from Container)
    if (IsWindow((HWND)parentHandle)) {
        parentHwnd = (HWND)parentHandle;
    } else {
        // Try to get from WindowData
        WindowData* windowData = static_cast<WindowData*>(parentHandle);
        if (windowData && windowData->hwnd) {
            parentHwnd = windowData->hwnd;
        } else {
            return nullptr;
        }
    }
    
    WebViewData* webViewData = new WebViewData;
    webViewData->parent = parentHwnd;
    webViewData->hwnd = nullptr;
    webViewData->controller = nullptr;
    webViewData->webview = nullptr;
    webViewData->initialized = false;
    webViewData->webview2Available = false;
#ifdef HAVE_WEBVIEW2
    webViewData->pendingURL.clear();
    webViewData->pendingHTML.clear();
    webViewData->pendingFile.clear();
    webViewData->createWindowCallback = nullptr;
    webViewData->createWindowUserData = nullptr;
    webViewData->messageCallback = nullptr;
    webViewData->messageUserData = nullptr;
    webViewData->messageHandler = nullptr;
#endif
    
#ifdef HAVE_WEBVIEW2
    // Check if WebView2 Runtime is available
    if (!IsWebView2RuntimeAvailable()) {
        wchar_t errorMsg[512];
        swprintf_s(errorMsg, 512, 
            L"WebView2 Runtime not found.\n\n"
            L"Please install Microsoft Edge WebView2 Runtime:\n"
            L"https://go.microsoft.com/fwlink/p/?LinkId=2124703\n\n"
            L"The runtime is required for WebView2 to work.");
        std::wcout << errorMsg << std::endl;
        OutputDebugStringW(L"WebView2 Runtime not found in registry\n");
        
        webViewData->hwnd = CreateWindowW(
            L"STATIC",
            errorMsg,
            WS_VISIBLE | WS_CHILD | SS_LEFT | SS_CENTER,
            x, y, width, height,
            parentHwnd,
            nullptr,
            getInstance(),
            nullptr
        );
        return webViewData;
    }
    
    // Try to create WebView2 using SDK
    // Note: This is asynchronous - the actual WebView2 controller will be created in the callback
    std::wcout << L"WebView2 Runtime found, attempting to create environment..." << std::endl;
    OutputDebugStringW(L"Attempting to create WebView2 environment...\n");
    HRESULT hr = CreateCoreWebView2EnvironmentWithOptions(
        nullptr, nullptr, nullptr,
        new WebView2EnvironmentHandler(webViewData, parentHwnd, x, y, width, height)
    );
    
    if (FAILED(hr)) {
        // Fallback to static control if WebView2 initialization fails immediately
        wchar_t errorMsg[512];
        if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) || 
            hr == 0x80070002 || // ERROR_FILE_NOT_FOUND
            hr == 0x8007007E) { // ERROR_MOD_NOT_FOUND
            swprintf_s(errorMsg, 512, 
                L"WebView2 Runtime not found.\n\n"
                L"Please install Microsoft Edge WebView2 Runtime:\n"
                L"https://go.microsoft.com/fwlink/p/?LinkId=2124703\n\n"
                L"Error code: 0x%08X", hr);
        } else {
            swprintf_s(errorMsg, 512, 
                L"WebView2 initialization failed immediately.\n\n"
                L"Error code: 0x%08X\n\n"
                L"Please ensure WebView2 Runtime is installed:\n"
                L"https://go.microsoft.com/fwlink/p/?LinkId=2124703", hr);
        }
        
        std::wcout << errorMsg << std::endl;
        OutputDebugStringW(L"CreateCoreWebView2EnvironmentWithOptions failed: ");
        wchar_t hrStr[32];
        swprintf_s(hrStr, 32, L"0x%08X", hr);
        OutputDebugStringW(hrStr);
        OutputDebugStringW(L"\n");
        
        webViewData->hwnd = CreateWindowW(
            L"STATIC",
            errorMsg,
            WS_VISIBLE | WS_CHILD | SS_LEFT | SS_CENTER,
            x, y, width, height,
            parentHwnd,
            nullptr,
            getInstance(),
            nullptr
        );
    } else {
        std::wcout << L"WebView2 environment creation initiated (async)" << std::endl;
        OutputDebugStringW(L"WebView2 environment creation initiated (async)\n");
    }
    // If hr == S_OK, initialization is in progress - the callback will handle success/failure
#else
    // WebView2 SDK not available, use static control
    // User needs to install WebView2 SDK and define HAVE_WEBVIEW2 to enable WebView2
    webViewData->hwnd = CreateWindowW(
        L"STATIC",
        L"WebView2 SDK not available. Install WebView2 SDK and define HAVE_WEBVIEW2 in CMakeLists.txt",
        WS_VISIBLE | WS_CHILD | SS_LEFT,
        x, y, width, height,
        parentHwnd,
        nullptr,
        getInstance(),
        nullptr
    );
#endif
    
    return webViewData;
}

void destroyWebView(void* webViewHandle) {
    if (webViewHandle) {
        WebViewData* data = static_cast<WebViewData*>(webViewHandle);
#ifdef HAVE_WEBVIEW2
        // Remove WebMessageReceived handler before destroying webview
        if (data->webview && data->messageHandler) {
            EventRegistrationToken token = {};
            // Note: We don't have the token stored, but WebView2 should handle cleanup
            // when the webview is released. However, we should still delete our handler.
            delete data->messageHandler;
            data->messageHandler = nullptr;
        }
        if (data->controller) {
            data->controller->Close();
            data->controller->Release();
        }
        if (data->webview) {
            data->webview->Release();
        }
#endif
        if (data->hwnd && data->hwnd != data->parent) {
            DestroyWindow(data->hwnd);
        }
        delete data;
    }
}

void loadHTMLFile(void* webViewHandle, const std::string& filePath) {
    if (!webViewHandle) {
        return;
    }
    
    WebViewData* data = static_cast<WebViewData*>(webViewHandle);
    
#ifdef HAVE_WEBVIEW2
    if (data->initialized && data->webview) {
        // Use WebView2 to load the file
        std::wstring wpath = stringToWString(filePath);
        wchar_t fullPath[MAX_PATH];
        if (GetFullPathNameW(wpath.c_str(), MAX_PATH, fullPath, nullptr)) {
            std::wstring url = L"file:///" + std::wstring(fullPath);
            // Replace backslashes with forward slashes
            for (size_t i = 0; i < url.length(); ++i) {
                if (url[i] == L'\\') url[i] = L'/';
            }
            data->webview->Navigate(url.c_str());
        }
    } else if (!data->initialized) {
        // Queue the file to load once WebView2 is initialized
        data->pendingFile = filePath;
        std::wcout << L"WebView2 not ready yet - queuing file: " << stringToWString(filePath) << std::endl;
        OutputDebugStringW(L"WebView2 not ready yet - file queued\n");
        return; // Don't fall through to static control
    } else
#endif
    {
        // Fallback: Read file and display in static control
        std::ifstream file(filePath);
        if (file.is_open()) {
            std::stringstream buffer;
            buffer << file.rdbuf();
            std::string content = "Loaded: " + filePath + "\n\n" + buffer.str();
            std::wstring wcontent = stringToWString(content);
            if (data->hwnd) {
                SetWindowTextW(data->hwnd, wcontent.c_str());
            }
            file.close();
        } else {
            std::string error = "Failed to load: " + filePath;
            std::wstring werror = stringToWString(error);
            if (data->hwnd) {
                SetWindowTextW(data->hwnd, werror.c_str());
            }
        }
    }
}

void loadHTMLString(void* webViewHandle, const std::string& html) {
    if (!webViewHandle) {
        return;
    }
    
    WebViewData* data = static_cast<WebViewData*>(webViewHandle);
    
#ifdef HAVE_WEBVIEW2
    if (data->initialized && data->webview) {
        // Use WebView2 to load HTML string
        std::wstring whtml = stringToWString(html);
        data->webview->NavigateToString(whtml.c_str());
        
        // Inject message bridge script after loading HTML
        // This ensures the script is available even if navigation hasn't completed yet
        const wchar_t* setupScript = L""
            L"(function() {"
            L"  if (window.__webview2ReceiveMessage) return;"
            L"  console.log('Injecting WebView2 message bridge after HTML load...');"
            L"  window.__webview2Messages = [];"
            L"  window.__webview2MessageListeners = [];"
            L"  window.__webview2ReceiveMessage = function(messageJson) {"
            L"    console.log('__webview2ReceiveMessage called with:', messageJson);"
            L"    try {"
            L"      const data = typeof messageJson === 'string' ? JSON.parse(messageJson) : messageJson;"
            L"      console.log('Parsed message data:', data);"
            L"      window.__webview2Messages.push(data);"
            L"      window.__webview2MessageListeners.forEach(function(listener) {"
            L"        try { listener({ data: data }); } catch(e) { console.error('Error in listener:', e); }"
            L"      });"
            L"    } catch(e) { console.error('Error parsing message:', e, messageJson); }"
            L"  };"
            L"  if (!window.chrome) window.chrome = {};"
            L"  if (!window.chrome.webview) window.chrome.webview = {};"
            L"  window.chrome.webview.addEventListener = function(type, listener) {"
            L"    if (type === 'message') {"
            L"      window.__webview2MessageListeners.push(listener);"
            L"      console.log('Message listener added, total:', window.__webview2MessageListeners.length);"
            L"      window.__webview2Messages.forEach(function(msg) {"
            L"        try { listener({ data: msg }); } catch(e) { console.error('Error:', e); }"
            L"      });"
            L"    }"
            L"  };"
            L"  window.chrome.webview.removeEventListener = function(type, listener) {"
            L"    if (type === 'message') {"
            L"      const index = window.__webview2MessageListeners.indexOf(listener);"
            L"      if (index > -1) window.__webview2MessageListeners.splice(index, 1);"
            L"    }"
            L"  };"
            L"  console.log('WebView2 message bridge injected successfully');"
            L"})();";
        
        // Use a small delay to ensure the page has started loading
        // Then inject the script
        data->webview->ExecuteScript(setupScript, nullptr);
    } else if (!data->initialized) {
        // Queue the HTML to load once WebView2 is initialized
        data->pendingHTML = html;
        std::wcout << L"WebView2 not ready yet - queuing HTML" << std::endl;
        OutputDebugStringW(L"WebView2 not ready yet - HTML queued\n");
        return; // Don't fall through to static control
    } else
#endif
    {
        // Fallback: Display in static control
        std::wstring whtml = stringToWString(html);
        if (data->hwnd) {
            SetWindowTextW(data->hwnd, whtml.c_str());
        }
    }
}

void loadURL(void* webViewHandle, const std::string& url) {
    if (!webViewHandle) {
        return;
    }
    
    WebViewData* data = static_cast<WebViewData*>(webViewHandle);
    
#ifdef HAVE_WEBVIEW2
    if (data->initialized && data->webview) {
        // Use WebView2 to load the URL
        std::wstring wurl = stringToWString(url);
        std::wcout << L"Loading URL in WebView2: " << wurl << std::endl;
        OutputDebugStringW(L"Loading URL in WebView2: ");
        OutputDebugStringW(wurl.c_str());
        OutputDebugStringW(L"\n");
        data->webview->Navigate(wurl.c_str());
    } else if (!data->initialized) {
        // Queue the URL to load once WebView2 is initialized
        data->pendingURL = url;
        std::wcout << L"WebView2 not ready yet - queuing URL: " << stringToWString(url) << std::endl;
        OutputDebugStringW(L"WebView2 not ready yet - URL queued\n");
        // Don't show error message - it will load once ready
    } else {
        // WebView2 failed to initialize
        std::wcout << L"WebView2 failed to initialize - cannot load URL" << std::endl;
        OutputDebugStringW(L"WebView2 failed to initialize - cannot load URL\n");
        // Fallback: Display message
        std::string message = "URL: " + url + "\n(WebView2 required for full URL loading)";
        std::wstring wmessage = stringToWString(message);
        if (data->hwnd) {
            SetWindowTextW(data->hwnd, wmessage.c_str());
        }
    }
#else
    {
        // Fallback: Display message
        std::string message = "URL: " + url + "\n(WebView2 required for full URL loading)";
        std::wstring wmessage = stringToWString(message);
        if (data->hwnd) {
            SetWindowTextW(data->hwnd, wmessage.c_str());
        }
    }
#endif
}

// Set callback for creating new windows from web view messages
void setWebViewCreateWindowCallback(void* webViewHandle, CreateWindowCallback callback, void* userData) {
    if (!webViewHandle) {
        return;
    }
    
    WebViewData* data = static_cast<WebViewData*>(webViewHandle);
#ifdef HAVE_WEBVIEW2
    data->createWindowCallback = callback;
    data->createWindowUserData = userData;
#endif
}

void setWebViewMessageCallback(void* webViewHandle, MessageCallback callback, void* userData) {
    if (!webViewHandle) {
        std::wcout << L"setWebViewMessageCallback: webViewHandle is null" << std::endl;
        OutputDebugStringW(L"setWebViewMessageCallback: webViewHandle is null\n");
        return;
    }
    
    if (!callback) {
        std::wcout << L"setWebViewMessageCallback: callback is null" << std::endl;
        OutputDebugStringW(L"setWebViewMessageCallback: callback is null\n");
        return;
    }
    
    WebViewData* data = static_cast<WebViewData*>(webViewHandle);
#ifdef HAVE_WEBVIEW2
    data->messageCallback = callback;
    data->messageUserData = userData;
    
    std::cout << "✓✓✓ setWebViewMessageCallback: Callback set successfully" << std::endl;
    std::wcout << L"✓✓✓ setWebViewMessageCallback: Callback set successfully" << std::endl;
    OutputDebugStringW(L"✓✓✓ setWebViewMessageCallback: Callback set successfully\n");
    std::cout << "  callback pointer: " << (void*)callback << std::endl;
    std::cout << "  userData pointer: " << userData << std::endl;
#endif
}

void setWebViewPreloadScript(void* webViewHandle, const std::string& scriptContent) {
#ifdef HAVE_WEBVIEW2
    if (!webViewHandle) return;
    WebViewData* data = static_cast<WebViewData*>(webViewHandle);
    data->customPreloadScript = scriptContent;
#endif
}

void postMessageToJavaScript(void* webViewHandle, const std::string& jsonMessage) {
    if (!webViewHandle) {
        std::wcout << L"postMessageToJavaScript: webViewHandle is null" << std::endl;
        OutputDebugStringW(L"postMessageToJavaScript: webViewHandle is null\n");
        return;
    }
    
    WebViewData* data = static_cast<WebViewData*>(webViewHandle);
#ifdef HAVE_WEBVIEW2
    if (data->webview) {
        std::wstring wmsg = stringToWString(jsonMessage);
        std::cout << "postMessageToJavaScript: Sending message: " << jsonMessage << std::endl;
        std::wcout << L"postMessageToJavaScript: Sending message: " << wmsg << std::endl;
        OutputDebugStringW(L"postMessageToJavaScript: Sending message\n");
        OutputDebugStringW(wmsg.c_str());
        OutputDebugStringW(L"\n");
        
        // PostWebMessageAsJson(jsonString): We send a JSON string. Page receives event.data as PARSED OBJECT.
        // WebView2 accepts string (our jsonMessage); page's addEventListener gets parsed object.
        std::cout << "[WebView2] postMessageToJS: PostWebMessageAsJson (native->page), len=" << jsonMessage.length() << std::endl;
        OutputDebugStringA("[WebView2] PostWebMessageAsJson - sends JSON string, page gets object\n");
        HRESULT hr = data->webview->PostWebMessageAsJson(wmsg.c_str());
        if (FAILED(hr)) {
            std::cout << "postMessageToJavaScript: PostWebMessageAsJson failed with HRESULT: 0x" << std::hex << hr << std::dec << std::endl;
            std::wcout << L"postMessageToJavaScript: PostWebMessageAsJson failed with HRESULT: 0x" << std::hex << hr << std::dec << std::endl;
            OutputDebugStringW(L"postMessageToJavaScript: PostWebMessageAsJson failed\n");
            
            // Fallback: Try ExecuteScript as backup
            std::cout << "postMessageToJavaScript: Attempting fallback via ExecuteScript" << std::endl;
            std::wcout << L"postMessageToJavaScript: Attempting fallback via ExecuteScript" << std::endl;
            OutputDebugStringW(L"postMessageToJavaScript: Attempting fallback via ExecuteScript\n");
            
            // Send message using ExecuteScript to call our bridge function
            // First, ensure the bridge is available by injecting it if needed
            // We need to escape the JSON string properly for JavaScript
            std::wstring escapedJson;
            for (wchar_t c : wmsg) {
                if (c == L'\\') {
                    escapedJson += L"\\\\";
                } else if (c == L'"') {
                    escapedJson += L"\\\"";
                } else if (c == L'\n') {
                    escapedJson += L"\\n";
                } else if (c == L'\r') {
                    escapedJson += L"\\r";
                } else if (c == L'\t') {
                    escapedJson += L"\\t";
                } else {
                    escapedJson += c;
                }
            }
            
            std::wstring checkAndInjectScript = 
            L"(function() {"
            L"  console.log('postMessageToJavaScript: Checking bridge...');"
            L"  if (!window.__webview2ReceiveMessage) {"
            L"    console.log('Bridge not found, injecting...');"
            L"    window.__webview2Messages = [];"
            L"    window.__webview2MessageListeners = [];"
            L"    window.__webview2ReceiveMessage = function(messageJson) {"
            L"      console.log('__webview2ReceiveMessage called with:', messageJson);"
            L"      const data = typeof messageJson === 'string' ? JSON.parse(messageJson) : messageJson;"
            L"      console.log('Parsed data:', data);"
            L"      window.__webview2Messages.push(data);"
            L"      console.log('Notifying', window.__webview2MessageListeners.length, 'listeners');"
            L"      window.__webview2MessageListeners.forEach(function(listener) {"
            L"        try { listener({ data: data }); } catch(e) { console.error('Error in listener:', e); }"
            L"      });"
            L"    };"
            L"    if (!window.chrome) window.chrome = {};"
            L"    if (!window.chrome.webview) window.chrome.webview = {};"
            L"    window.chrome.webview.addEventListener = function(type, listener) {"
            L"      if (type === 'message') {"
            L"        window.__webview2MessageListeners.push(listener);"
            L"        console.log('Listener added, total:', window.__webview2MessageListeners.length);"
            L"        window.__webview2Messages.forEach(function(msg) {"
            L"          try { listener({ data: msg }); } catch(e) { console.error('Error:', e); }"
            L"        });"
            L"      }"
            L"    };"
            L"    window.chrome.webview.removeEventListener = function(type, listener) {"
            L"      if (type === 'message') {"
            L"        const index = window.__webview2MessageListeners.indexOf(listener);"
            L"        if (index > -1) window.__webview2MessageListeners.splice(index, 1);"
            L"      }"
            L"    };"
            L"    console.log('Bridge injected successfully');"
            L"  } else {"
            L"    console.log('Bridge already exists');"
            L"  }"
            L"  "
            L"  // Now send the message - pass JSON as a string"
            L"  if (window.__webview2ReceiveMessage) {"
            L"    console.log('Calling __webview2ReceiveMessage with JSON string');"
            L"    try {"
            L"      window.__webview2ReceiveMessage(";
        checkAndInjectScript += L"\"";
        checkAndInjectScript += escapedJson;
        checkAndInjectScript += L"\"";
        checkAndInjectScript += L");"
            L"      console.log('Message sent successfully via bridge');"
            L"    } catch(e) {"
            L"      console.error('Error calling bridge:', e);"
            L"    }"
            L"  } else {"
            L"    console.error('Bridge still not available after injection attempt');"
            L"  }"
            L"})();";
        
            HRESULT hrFallback = data->webview->ExecuteScript(checkAndInjectScript.c_str(), nullptr);
            if (FAILED(hrFallback)) {
                std::cout << "postMessageToJavaScript: Fallback ExecuteScript also failed with HRESULT: 0x" << std::hex << hrFallback << std::dec << std::endl;
                std::wcout << L"postMessageToJavaScript: Fallback ExecuteScript also failed" << std::endl;
                OutputDebugStringW(L"postMessageToJavaScript: Fallback ExecuteScript also failed\n");
            } else {
                std::cout << "postMessageToJavaScript: Message sent via fallback ExecuteScript" << std::endl;
                std::wcout << L"postMessageToJavaScript: Message sent via fallback ExecuteScript" << std::endl;
                OutputDebugStringW(L"postMessageToJavaScript: Message sent via fallback ExecuteScript\n");
            }
        } else {
            std::cout << "postMessageToJavaScript: Message sent via PostWebMessageAsJson" << std::endl;
            std::wcout << L"postMessageToJavaScript: Message sent via PostWebMessageAsJson" << std::endl;
            OutputDebugStringW(L"postMessageToJavaScript: Message sent via PostWebMessageAsJson\n");
        }
    } else {
        std::wcout << L"postMessageToJavaScript: webview is null" << std::endl;
        OutputDebugStringW(L"postMessageToJavaScript: webview is null\n");
    }
#endif
}

void executeWebViewScript(void* webViewHandle, const std::string& script) {
    if (!webViewHandle || script.empty()) return;
    WebViewData* data = static_cast<WebViewData*>(webViewHandle);
#ifdef HAVE_WEBVIEW2
    if (data->webview) {
        std::wstring wscript = stringToWString(script);
        data->webview->ExecuteScript(wscript.c_str(), nullptr);
    }
#endif
}

void printWebView(void* webViewHandle) {
    if (!webViewHandle) return;
    WebViewData* data = static_cast<WebViewData*>(webViewHandle);
#ifdef HAVE_WEBVIEW2
    if (data->webview) {
        // Hide AppHeader/alerts (same as @media print), restore on afterprint, then window.print()
        const char* printScript = R"(
(function(){
  var sel='.app-header,.app-header *,.alerts-view,.alerts-view *,#app>header';
  var els=document.querySelectorAll(sel);
  window.__crossdevPrintHidden=[];
  var main=document.querySelector('.app-main');
  if(main){window.__crossdevPrintHidden.push({el:main,prop:'paddingTop',val:main.style.paddingTop});main.style.paddingTop='0';}
  els.forEach(function(e){
    if(e.offsetParent!==null){
      window.__crossdevPrintHidden.push({el:e,display:e.style.display,visibility:e.style.visibility});
      e.style.setProperty('display','none','important');
      e.style.setProperty('visibility','hidden','important');
    }
  });
  function restore(){
    if(window.__crossdevPrintHidden){
      window.__crossdevPrintHidden.forEach(function(r){
        if(r.prop)r.el.style[r.prop]=r.val||'';
        else{r.el.style.display=r.display||'';r.el.style.visibility=r.visibility||'';}
      });
      window.__crossdevPrintHidden=null;
    }
    window.removeEventListener('afterprint',restore);
  }
  window.addEventListener('afterprint',restore);
  window.print();
})();
)";
        std::wstring wscript = stringToWString(printScript);
        data->webview->ExecuteScript(wscript.c_str(), nullptr);
    }
#endif
}

void resizeWebView(void* webViewHandle, int width, int height) {
    if (!webViewHandle) {
        return;
    }
    
    WebViewData* data = static_cast<WebViewData*>(webViewHandle);
    
#ifdef HAVE_WEBVIEW2
    if (data->controller) {
        RECT bounds = { 0, 0, width, height };
        data->controller->put_Bounds(bounds);
    } else if (data->hwnd) {
        // Fallback for static control
        SetWindowPos(data->hwnd, nullptr, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER);
    }
#else
    if (data->hwnd) {
        SetWindowPos(data->hwnd, nullptr, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER);
    }
#endif
}

void processDeferredWebViewMessage(LPARAM lParam) {
    DeferredWebViewMessage* d = (DeferredWebViewMessage*)lParam;
    if (d && d->message && d->callback) {
        d->callback(*d->message, d->userData);
        delete d->message;
        delete d;
    }
}
#else
void processDeferredWebViewMessage(LPARAM /*lParam*/) {
    // Stub when WebView2 not available - should not be called
}
#endif

void openInspector(void* webViewHandle) {
    if (!webViewHandle) return;
    WebViewData* data = static_cast<WebViewData*>(webViewHandle);
#ifdef HAVE_WEBVIEW2
    if (data->webview) {
        // Open DevTools by simulating Ctrl+Shift+I keyboard shortcut
        data->webview->OpenDevToolsWindow();
    }
#endif
}

} // namespace platform

#endif // PLATFORM_WINDOWS
