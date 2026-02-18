// Linux web view implementation
#include "../../../include/platform.h"
#include "../platform_impl.h"
#include <string>
#include <gtk/gtk.h>
#include <webkit2/webkit2.h>
#include <fstream>
#include <sstream>
#include <cstring>

#ifdef PLATFORM_LINUX

namespace platform {

struct WindowData {
    Display* display;
    Window window;
    GtkWidget* gtkWindow;
    bool visible;
    std::string title;
};

// Callback type for window creation
typedef void (*CreateWindowCallback)(const std::string& title, void* userData);

struct WebViewData {
    WebKitWebView* webView;
    GtkWidget* container;
    CreateWindowCallback createWindowCallback;
    void* createWindowUserData;
    MessageCallback messageCallback;
    void* messageUserData;
    std::string customPreloadScript;
};

void* createWebView(void* parentHandle, int x, int y, int width, int height) {
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
        if (windowData) {
            // Create GTK window if not exists
            if (!windowData->gtkWindow) {
                windowData->gtkWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
                gtk_window_set_default_size(GTK_WINDOW(windowData->gtkWindow), width + x, height + y);
            }
            parentWidget = windowData->gtkWindow;
        } else {
            return nullptr;
        }
    }
    
    WebViewData* webViewData = new WebViewData;
    webViewData->webView = WEBKIT_WEB_VIEW(webkit_web_view_new());
    webViewData->container = GTK_WIDGET(webViewData->webView);
    webViewData->createWindowCallback = nullptr;
    webViewData->createWindowUserData = nullptr;
    webViewData->messageCallback = nullptr;
    webViewData->messageUserData = nullptr;
    
    gtk_widget_set_size_request(webViewData->container, width, height);
    
    if (GTK_IS_WINDOW(parentWidget)) {
        // Parent is a window - use fixed layout
        GtkWidget* fixed = gtk_fixed_new();
        gtk_container_add(GTK_CONTAINER(parentWidget), fixed);
        gtk_fixed_put(GTK_FIXED(fixed), webViewData->container, x, y);
    } else {
        // Parent is a container/widget - add directly
        gtk_container_add(GTK_CONTAINER(parentWidget), webViewData->container);
    }
    
    gtk_widget_show(webViewData->container);
    
    return webViewData;
}

void destroyWebView(void* webViewHandle) {
    if (webViewHandle) {
        WebViewData* data = static_cast<WebViewData*>(webViewHandle);
        if (data->container) {
            gtk_widget_destroy(data->container);
        }
        delete data;
    }
}

void loadHTMLFile(void* webViewHandle, const std::string& filePath) {
    if (!webViewHandle) {
        return;
    }
    
    WebViewData* data = static_cast<WebViewData*>(webViewHandle);
    
    std::ifstream file(filePath);
    if (file.is_open()) {
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string htmlContent = buffer.str();
        file.close();
        
        webkit_web_view_load_html(data->webView, htmlContent.c_str(), 
                                  ("file://" + filePath).c_str());
    }
}

void loadHTMLString(void* webViewHandle, const std::string& html) {
    if (!webViewHandle) {
        return;
    }
    
    WebViewData* data = static_cast<WebViewData*>(webViewHandle);
    webkit_web_view_load_html(data->webView, html.c_str(), nullptr);
}

void loadURL(void* webViewHandle, const std::string& url) {
    if (!webViewHandle) {
        return;
    }
    
    WebViewData* data = static_cast<WebViewData*>(webViewHandle);
    webkit_web_view_load_uri(data->webView, url.c_str());
}

// Script message handler callback for WebKitGTK
static void onScriptMessageReceived(WebKitUserContentManager* manager, WebKitJavascriptResult* result, gpointer userData) {
    WebViewData* data = static_cast<WebViewData*>(userData);
    if (!data) {
        return;
    }
    
    JSCValue* value = webkit_javascript_result_get_js_value(result);
    if (!value) {
        return;
    }
    
    // Convert to JSON string
    char* jsonStr = jsc_value_to_json_string(value, 0);
    std::string jsonMessage = jsonStr ? jsonStr : "";
    g_free(jsonStr);
    
    // Try new message callback first (for MessageRouter)
    if (data->messageCallback) {
        data->messageCallback(jsonMessage, data->messageUserData);
        g_object_unref(value);
        return;
    }
    
    // Fallback to old createWindow callback for backward compatibility
    if (data->createWindowCallback && jsc_value_is_object(value)) {
        JSCValue* typeValue = jsc_value_object_get_property(value, "type");
        if (typeValue && jsc_value_is_string(typeValue)) {
            gchar* typeStr = jsc_value_to_string(typeValue);
            if (typeStr && strcmp(typeStr, "createWindow") == 0) {
                JSCValue* titleValue = jsc_value_object_get_property(value, "title");
                if (titleValue && jsc_value_is_string(titleValue)) {
                    gchar* titleStr = jsc_value_to_string(titleValue);
                    std::string title = titleStr ? titleStr : "New Window";
                    if (titleStr) g_free(titleStr);
                    
                    data->createWindowCallback(title, data->createWindowUserData);
                }
                if (titleValue) g_object_unref(titleValue);
            }
            if (typeStr) g_free(typeStr);
        }
        if (typeValue) g_object_unref(typeValue);
    }
    
    g_object_unref(value);
}

void setWebViewCreateWindowCallback(void* webViewHandle, void (*callback)(const std::string& title, void* userData), void* userData) {
    if (!webViewHandle || !callback) {
        return;
    }
    
    WebViewData* data = static_cast<WebViewData*>(webViewHandle);
    data->createWindowCallback = callback;
    data->createWindowUserData = userData;
    
    // Get user content manager
    WebKitUserContentManager* manager = webkit_web_view_get_user_content_manager(data->webView);
    
    // Register script message handler
    webkit_user_content_manager_register_script_message_handler(manager, "createWindow");
    
    // Connect to script message received signal
    g_signal_connect(manager, "script-message-received::createWindow",
                     G_CALLBACK(onScriptMessageReceived), data);
    
    // Inject JavaScript bridge for window.chrome.webview.postMessage
    const char* script = R"(
        window.chrome = window.chrome || {};
        window.chrome.webview = window.chrome.webview || {};
        window.chrome.webview.postMessage = function(message) {
            if (typeof message === 'object' && message.type === 'createWindow') {
                window.webkit.messageHandlers.createWindow.postMessage(message);
            }
        };
    )";
    
    // Inject script that will be executed when page loads
    WebKitUserScript* userScript = webkit_user_script_new(script, 
        WEBKIT_USER_CONTENT_INJECT_TOP_FRAME, 
        WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_START, 
        nullptr, nullptr);
    webkit_user_content_manager_add_script(manager, userScript);
    g_object_unref(userScript);
}

void setWebViewMessageCallback(void* webViewHandle, void (*callback)(const std::string& jsonMessage, void* userData), void* userData) {
    if (!webViewHandle || !callback) {
        return;
    }
    
    WebViewData* data = static_cast<WebViewData*>(webViewHandle);
    if (!data || !data->webView) {
        return;
    }
    
    data->messageCallback = callback;
    data->messageUserData = userData;
    
    // Register message handler if not already registered
    WebKitUserContentManager* manager = webkit_web_view_get_user_content_manager(data->webView);
    if (!g_signal_has_handler_pending(manager, g_signal_lookup("script-message-received::nativeMessage", 
                                                               WEBKIT_TYPE_USER_CONTENT_MANAGER), 0, false)) {
        webkit_user_content_manager_register_script_message_handler(manager, "nativeMessage");
        g_signal_connect(manager, "script-message-received::nativeMessage",
                         G_CALLBACK(onScriptMessageReceived), data);
        
        const char* script = nullptr;
        if (!data->customPreloadScript.empty()) {
            script = data->customPreloadScript.c_str();
        } else {
        // Inject CrossDev bridge (invoke, events, binary) - same as macOS
        const char* script = R"(
            (function(){
                var _pending=new Map();
                var _eventListeners={};
                function _ab2b64(ab){var u8=new Uint8Array(ab);var bin='';for(var i=0;i<u8.length;i++)bin+=String.fromCharCode(u8[i]);return btoa(bin);}
                function _b642ab(s){var bin=atob(s);var u8=new Uint8Array(bin.length);for(var i=0;i<bin.length;i++)u8[i]=bin.charCodeAt(i);return u8.buffer;}
                function _toWire(obj){
                    if(obj===null||typeof obj!=='object')return obj;
                    if(obj instanceof ArrayBuffer){return {__base64:_ab2b64(obj)};}
                    if(ArrayBuffer.isView&&ArrayBuffer.isView(obj)){return {__base64:_ab2b64(obj.buffer.slice(obj.byteOffset,obj.byteOffset+obj.byteLength))};}
                    if(Array.isArray(obj))return obj.map(_toWire);
                    var out={};for(var k in obj)if(obj.hasOwnProperty(k))out[k]=_toWire(obj[k]);return out;
                }
                window.addEventListener('message',function(e){
                    var d=e.data;
                    if(!d)return;
                    if(d.type==='crossdev:event'){
                        var name=d.name,payload=d.payload||{};
                        var list=_eventListeners[name];
                        if(list)list.forEach(function(fn){try{fn(payload)}catch(err){console.error(err)}});
                        return;
                    }
                    if(d.requestId){
                        var h=_pending.get(d.requestId);
                        if(h){_pending.delete(d.requestId);
                            var res=d.result;
                            if(h.binary&&res&&typeof res.data==='string'){res=Object.assign({},res);res.data=_b642ab(res.data);}
                            d.error?h.reject(new Error(d.error)):h.resolve(res);
                        }
                    }
                });
                function _post(msg){
                    if(window.webkit&&window.webkit.messageHandlers&&window.webkit.messageHandlers.nativeMessage){
                        window.webkit.messageHandlers.nativeMessage.postMessage(msg);
                    }
                }
                var CrossDev={
                    invoke:function(type,payload,opts){
                        var opt=opts||{};
                        return new Promise(function(resolve,reject){
                            var rid=Date.now()+'-'+Math.random();
                            _pending.set(rid,{resolve:resolve,reject:reject,binary:!!opt.binaryResponse});
                            setTimeout(function(){if(_pending.has(rid)){_pending.delete(rid);reject(new Error('Request timeout'));}},10000);
                            _post({type:type,payload:_toWire(payload||{}),requestId:rid});
                        });
                    },
                    events:{
                        on:function(name,fn){
                            if(!_eventListeners[name])_eventListeners[name]=[];
                            _eventListeners[name].push(fn);
                            return function(){var i=_eventListeners[name].indexOf(fn);if(i>=0)_eventListeners[name].splice(i,1);};
                        }
                    }
                };
                Object.freeze(CrossDev.events);
                Object.freeze(CrossDev);
                try{Object.defineProperty(window,'CrossDev',{value:CrossDev,configurable:false,writable:false});}catch(_){window.CrossDev=CrossDev;}
                window.chrome=window.chrome||{};
                window.chrome.webview=window.chrome.webview||{};
                window.chrome.webview.postMessage=function(m){var msg=typeof m==='string'?JSON.parse(m):m;_post(msg);};
            })();
        )";
        }
        
        WebKitUserScript* userScript = webkit_user_script_new(script, 
            WEBKIT_USER_CONTENT_INJECT_TOP_FRAME, 
            WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_START, 
            nullptr, nullptr);
        webkit_user_content_manager_add_script(manager, userScript);
        g_object_unref(userScript);
    }
}

void setWebViewPreloadScript(void* webViewHandle, const std::string& scriptContent) {
    if (!webViewHandle) return;
    WebViewData* data = static_cast<WebViewData*>(webViewHandle);
    data->customPreloadScript = scriptContent;
}

void postMessageToJavaScript(void* webViewHandle, const std::string& jsonMessage) {
    if (!webViewHandle) {
        return;
    }
    
    WebViewData* data = static_cast<WebViewData*>(webViewHandle);
    if (!data || !data->webView) {
        return;
    }
    
    // Execute JavaScript to post message
    std::string script = "window.postMessage(" + jsonMessage + ", '*');";
    webkit_web_view_run_javascript(data->webView, script.c_str(), nullptr, nullptr, nullptr);
}

void executeWebViewScript(void* webViewHandle, const std::string& script) {
    if (!webViewHandle || script.empty()) return;
    WebViewData* data = static_cast<WebViewData*>(webViewHandle);
    if (data && data->webView) {
        webkit_web_view_run_javascript(data->webView, script.c_str(), nullptr, nullptr, nullptr);
    }
}

void printWebView(void* webViewHandle) {
    if (!webViewHandle) return;
    // Hide AppHeader/alerts (same as @media print), restore on afterprint, then window.print()
    const char* script = R"(
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
    executeWebViewScript(webViewHandle, script);
}

void resizeWebView(void* webViewHandle, int width, int height) {
    if (!webViewHandle) {
        return;
    }
    
    WebViewData* data = static_cast<WebViewData*>(webViewHandle);
    if (data && data->webView) {
        gtk_widget_set_size_request(data->webView, width, height);
    }
}

} // namespace platform

#endif // PLATFORM_LINUX
