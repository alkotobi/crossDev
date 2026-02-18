// iOS web view implementation
#import <UIKit/UIKit.h>
#import <WebKit/WebKit.h>
#import <objc/runtime.h>
#import <Foundation/Foundation.h>
#include "../../../include/platform.h"
#include "../platform_impl.h"
#include <string>
#include <fstream>
#include <sstream>

#ifdef PLATFORM_IOS

// Callback types
typedef void (*CreateWindowCallback)(const std::string& title, void* userData);
typedef void (*MessageCallback)(const std::string& jsonMessage, void* userData);

// Store callback in WebView's user content controller
@interface WebViewMessageHandler : NSObject <WKScriptMessageHandler>
@property (assign) CreateWindowCallback createWindowCallback;
@property (assign) void* createWindowUserData;
@property (assign) MessageCallback messageCallback;
@property (assign) void* messageUserData;
@end

@implementation WebViewMessageHandler
+ (NSString *)crossdevBridgeScript {
    return @"(function(){"
        "var _pending=new Map();"
        "var _eventListeners={};"
        "function _ab2b64(ab){var u8=new Uint8Array(ab);var bin='';for(var i=0;i<u8.length;i++)bin+=String.fromCharCode(u8[i]);return btoa(bin);}"
        "function _b642ab(s){var bin=atob(s);var u8=new Uint8Array(bin.length);for(var i=0;i<bin.length;i++)u8[i]=bin.charCodeAt(i);return u8.buffer;}"
        "function _toWire(obj){"
        "if(obj===null||typeof obj!=='object')return obj;"
        "if(obj instanceof ArrayBuffer){return {__base64:_ab2b64(obj)};}"
        "if(ArrayBuffer.isView(obj)){return {__base64:_ab2b64(obj.buffer.slice(obj.byteOffset,obj.byteOffset+obj.byteLength))};}"
        "if(Array.isArray(obj)){return obj.map(_toWire);}"
        "var out={};for(var k in obj)if(obj.hasOwnProperty(k))out[k]=_toWire(obj[k]);return out;"
        "}"
        "window.addEventListener('message',function(e){"
        "var d=e.data;"
        "if(!d)return;"
        "if(d.type==='crossdev:event'){"
        "var name=d.name,payload=d.payload||{};"
        "var list=_eventListeners[name];"
        "if(list)list.forEach(function(fn){try{fn(payload)}catch(err){console.error(err)}});"
        "return;"
        "}"
        "if(d.requestId){"
        "var h=_pending.get(d.requestId);"
        "if(h){_pending.delete(d.requestId);"
        "var res=d.result;"
        "if(h.binary&&res&&typeof res.data==='string'){res=Object.assign({},res);res.data=_b642ab(res.data);}"
        "d.error?h.reject(new Error(d.error)):h.resolve(res);"
        "}"
        "}"
        "});"
        "function _post(msg){"
        "if(window.webkit&&window.webkit.messageHandlers&&window.webkit.messageHandlers.nativeMessage){"
        "window.webkit.messageHandlers.nativeMessage.postMessage(msg);"
        "}"
        "}"
        "var CrossDev={"
        "invoke:function(type,payload,opts){"
        "var opt=opts||{};"
        "return new Promise(function(resolve,reject){"
        "var rid=Date.now()+'-'+Math.random();"
        "_pending.set(rid,{resolve:resolve,reject:reject,binary:!!opt.binaryResponse});"
        "setTimeout(function(){if(_pending.has(rid)){_pending.delete(rid);reject(new Error('Request timeout'));}},10000);"
        "_post({type:type,payload:_toWire(payload||{}),requestId:rid});"
        "});"
        "},"
        "events:{"
        "on:function(name,fn){"
        "if(!_eventListeners[name])_eventListeners[name]=[];"
        "_eventListeners[name].push(fn);"
        "return function(){var i=_eventListeners[name].indexOf(fn);if(i>=0)_eventListeners[name].splice(i,1);};"
        "}"
        "}"
        "};"
        "Object.freeze(CrossDev.events);"
        "Object.freeze(CrossDev);"
        "Object.defineProperty(window,'CrossDev',{value:CrossDev,configurable:false,writable:false});"
        "window.chrome=window.chrome||{};"
        "window.chrome.webview=window.chrome.webview||{};"
        "window.chrome.webview.postMessage=function(m){var msg=typeof m==='string'?JSON.parse(m):m;_post(msg);};"
        "})();";
}
- (void)userContentController:(WKUserContentController *)userContentController
      didReceiveScriptMessage:(WKScriptMessage *)message {
    // Convert message to JSON string
    NSError *error = nil;
    NSData *jsonData = [NSJSONSerialization dataWithJSONObject:message.body
                                                       options:0
                                                         error:&error];
    if (jsonData && !error) {
        NSString *jsonString = [[NSString alloc] initWithData:jsonData encoding:NSUTF8StringEncoding];
        std::string jsonMsg = [jsonString UTF8String];
        
        // Try new message callback first (for MessageRouter)
        if (self.messageCallback) {
            self.messageCallback(jsonMsg, self.messageUserData);
            return;
        }
        
        // Fallback to old createWindow callback for backward compatibility
        if ([message.name isEqualToString:@"createWindow"] && self.createWindowCallback) {
            NSDictionary *body = message.body;
            NSString *title = body[@"title"] ?: @"New Window";
            std::string titleStr = [title UTF8String];
            self.createWindowCallback(titleStr, self.createWindowUserData);
        }
    }
}
@end

namespace platform {

void* createWebView(void* parentHandle, int x, int y, int width, int height) {
    @autoreleasepool {
        if (!parentHandle) {
            return nullptr;
        }
        
        UIView *parentView = nullptr;
        
        // Get parent view - could be UIWindow's root view or a UIView (from Container)
        if ([(__bridge id)parentHandle isKindOfClass:[UIWindow class]]) {
            UIWindow *window = (__bridge UIWindow*)parentHandle;
            UIViewController* viewController = window.rootViewController;
            if (viewController) {
                parentView = viewController.view;
            } else {
                return nullptr;
            }
        } else if ([(__bridge id)parentHandle isKindOfClass:[UIView class]]) {
            parentView = (__bridge UIView*)parentHandle;
        } else {
            return nullptr;
        }
        
        CGRect webViewFrame = CGRectMake(x, y, width, height);
        WKWebView* webView = [[WKWebView alloc] initWithFrame:webViewFrame];
        webView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
        
        [parentView addSubview:webView];
        
        return (void*)CFBridgingRetain(webView);
    }
}

void destroyWebView(void* webViewHandle) {
    @autoreleasepool {
        if (webViewHandle) {
            WKWebView* webView = (__bridge_transfer WKWebView*)webViewHandle;
            [webView removeFromSuperview];
        }
    }
}

void loadHTMLFile(void* webViewHandle, const std::string& filePath) {
    @autoreleasepool {
        if (!webViewHandle) {
            return;
        }
        
        WKWebView* webView = (__bridge WKWebView*)webViewHandle;
        
        // Read file content
        std::ifstream file(filePath);
        if (!file.is_open()) {
            NSLog(@"Failed to open file: %s", filePath.c_str());
            return;
        }
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string htmlContent = buffer.str();
        file.close();
        
        NSString* htmlString = [NSString stringWithUTF8String:htmlContent.c_str()];
        NSURL* baseURL = [NSURL fileURLWithPath:[NSString stringWithUTF8String:filePath.c_str()]];
        
        [webView loadHTMLString:htmlString baseURL:baseURL];
    }
}

void loadHTMLString(void* webViewHandle, const std::string& html) {
    @autoreleasepool {
        if (!webViewHandle) return;
        WKWebView* webView = (__bridge WKWebView*)webViewHandle;
        NSString* htmlString = [NSString stringWithUTF8String:html.c_str()];
        [webView loadHTMLString:htmlString baseURL:nil];
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.1 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
            WebViewMessageHandler* h = objc_getAssociatedObject(webView, (__bridge const void*)@"messageHandler");
            if (h) {
                NSString* script = getPreloadScriptForWebView(webView);
                [webView evaluateJavaScript:script completionHandler:nil];
            }
        });
    }
}

void loadURL(void* webViewHandle, const std::string& url) {
    @autoreleasepool {
        if (!webViewHandle) {
            return;
        }
        
        WKWebView* webView = (__bridge WKWebView*)webViewHandle;
        NSString* urlString = [NSString stringWithUTF8String:url.c_str()];
        NSURL* nsURL = [NSURL URLWithString:urlString];
        if (nsURL) {
            NSURLRequest* request = [NSURLRequest requestWithURL:nsURL];
            [webView loadRequest:request];
        }
    }
}

static NSString* getPreloadScriptForWebView(WKWebView* webView) {
    NSString* custom = objc_getAssociatedObject(webView, (__bridge const void*)@"customPreloadScript");
    if (custom && [custom length] > 0) {
        return custom;
    }
    return [WebViewMessageHandler crossdevBridgeScript];
}

void setWebViewCreateWindowCallback(void* webViewHandle, void (*callback)(const std::string& title, void* userData), void* userData) {
    @autoreleasepool {
        if (!webViewHandle || !callback) {
            return;
        }
        
        WKWebView* webView = (__bridge WKWebView*)webViewHandle;
        WKUserContentController* userContentController = webView.configuration.userContentController;
        
        // Get or create message handler and inject bridge
        WebViewMessageHandler* handler = objc_getAssociatedObject(webView, (__bridge const void*)@"messageHandler");
        if (!handler) {
            handler = [[WebViewMessageHandler alloc] init];
            objc_setAssociatedObject(webView, (__bridge const void*)@"messageHandler", handler, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
            
            [userContentController addScriptMessageHandler:handler name:@"nativeMessage"];
            NSString* script = getPreloadScriptForWebView(webView);
            WKUserScript* userScript;
            if (@available(iOS 14.0, *)) {
                userScript = [[WKUserScript alloc] initWithSource:script injectionTime:WKUserScriptInjectionTimeAtDocumentStart forMainFrameOnly:YES inContentWorld:WKContentWorld.pageWorld];
            } else {
                userScript = [[WKUserScript alloc] initWithSource:script injectionTime:WKUserScriptInjectionTimeAtDocumentStart forMainFrameOnly:YES];
            }
            [userContentController addUserScript:userScript];
            [webView evaluateJavaScript:script completionHandler:nil];
        }
        
        handler.createWindowCallback = callback;
        handler.createWindowUserData = userData;
    }
}

void setWebViewMessageCallback(void* webViewHandle, void (*callback)(const std::string& jsonMessage, void* userData), void* userData) {
    @autoreleasepool {
        if (!webViewHandle || !callback) {
            return;
        }
        
        WKWebView* webView = (__bridge WKWebView*)webViewHandle;
        WKUserContentController* userContentController = webView.configuration.userContentController;
        
        // Get or create message handler and inject bridge
        WebViewMessageHandler* handler = objc_getAssociatedObject(webView, (__bridge const void*)@"messageHandler");
        if (!handler) {
            handler = [[WebViewMessageHandler alloc] init];
            objc_setAssociatedObject(webView, (__bridge const void*)@"messageHandler", handler, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
            
            [userContentController addScriptMessageHandler:handler name:@"nativeMessage"];
            NSString* script = getPreloadScriptForWebView(webView);
            WKUserScript* userScript;
            if (@available(iOS 14.0, *)) {
                userScript = [[WKUserScript alloc] initWithSource:script injectionTime:WKUserScriptInjectionTimeAtDocumentStart forMainFrameOnly:YES inContentWorld:WKContentWorld.pageWorld];
            } else {
                userScript = [[WKUserScript alloc] initWithSource:script injectionTime:WKUserScriptInjectionTimeAtDocumentStart forMainFrameOnly:YES];
            }
            [userContentController addUserScript:userScript];
            [webView evaluateJavaScript:script completionHandler:nil];
        }
        
        handler.messageCallback = callback;
        handler.messageUserData = userData;
    }
}

void setWebViewPreloadScript(void* webViewHandle, const std::string& scriptContent) {
    @autoreleasepool {
        if (!webViewHandle) return;
        WKWebView* webView = (__bridge WKWebView*)webViewHandle;
        if (scriptContent.empty()) {
            objc_setAssociatedObject(webView, (__bridge const void*)@"customPreloadScript", nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        } else {
            NSString* script = [NSString stringWithUTF8String:scriptContent.c_str()];
            objc_setAssociatedObject(webView, (__bridge const void*)@"customPreloadScript", script, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        }
    }
}

void postMessageToJavaScript(void* webViewHandle, const std::string& jsonMessage) {
    @autoreleasepool {
        if (!webViewHandle) return;
        WKWebView* webView = (__bridge WKWebView*)webViewHandle;
        NSString* jsonString = [NSString stringWithUTF8String:jsonMessage.c_str()];
        NSString* jsCode = [NSString stringWithFormat:@"window.postMessage(%@, '*');", jsonString];
        [webView evaluateJavaScript:jsCode completionHandler:^(id result, NSError* error) {
            if (error) NSLog(@"Error posting message to JavaScript: %@", error);
        }];
    }
}

void executeWebViewScript(void* webViewHandle, const std::string& script) {
    @autoreleasepool {
        if (!webViewHandle || script.empty()) return;
        WKWebView* webView = (__bridge WKWebView*)webViewHandle;
        NSString* nsScript = [NSString stringWithUTF8String:script.c_str()];
        [webView evaluateJavaScript:nsScript completionHandler:nil];
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
    executeWebViewScript(webViewHandle, std::string(script));
}

void resizeWebView(void* webViewHandle, int width, int height) {
    @autoreleasepool {
        if (webViewHandle) {
            WKWebView* webView = (__bridge WKWebView*)webViewHandle;
            CGRect newFrame = CGRectMake(0, 0, width, height);
            webView.frame = newFrame;
        }
    }
}

} // namespace platform

#endif // PLATFORM_IOS
