// macOS web view implementation
#import <Cocoa/Cocoa.h>
#import <WebKit/WebKit.h>
#import <objc/runtime.h>
#include "../../../include/platform.h"
#include "../platform_impl.h"
#include <string>
#include <fstream>
#include <sstream>

#ifdef PLATFORM_MACOS

// Callback types
typedef void (*CreateWindowCallback)(const std::string& title, void* userData);
typedef void (*MessageCallback)(const std::string& jsonMessage, void* userData);

// Store callback in WebView's user content controller
@interface WebViewMessageHandler : NSObject <WKScriptMessageHandler>
@property (assign) CreateWindowCallback createWindowCallback;
@property (assign) void* createWindowUserData;
@property (assign) MessageCallback messageCallback;
@property (assign) void* messageUserData;
+ (NSString *)crossdevBridgeScript;
@end

// Custom WKWebView that accepts first mouse click when window is inactive.
// By default, the first click only activates the window; returning YES here
// lets the click also be delivered to the WebView (one-click button activation).
@interface ClickThroughWebView : WKWebView
@end
@implementation ClickThroughWebView
- (BOOL)acceptsFirstMouse:(NSEvent *)event {
    return YES;
}
@end

// Navigation delegate that injects CrossDev bridge when main frame finishes loading.
// Needed because loadURL is async - when setMessageCallback runs, the page may not be loaded yet.
@interface BridgeInjectNavigationDelegate : NSObject <WKNavigationDelegate>
@end
@implementation BridgeInjectNavigationDelegate
- (void)webView:(WKWebView *)webView didFinishNavigation:(WKNavigation *)navigation {
    (void)navigation;
    if (webView && objc_getAssociatedObject(webView, @"messageHandler") != nil) {
        NSString *script = nil;
        id custom = objc_getAssociatedObject(webView, @"customPreloadScript");
        if (custom && [custom isKindOfClass:[NSString class]] && [custom length] > 0) {
            script = (NSString *)custom;
        } else {
            script = [WebViewMessageHandler crossdevBridgeScript];
        }
        if (script) {
            [webView evaluateJavaScript:script completionHandler:nil];
        }
    }
}
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
        "var to=type==='openFileDialog'?120000:30000;"
        "setTimeout(function(){if(_pending.has(rid)){_pending.delete(rid);reject(new Error('Request timeout'));}},to);"
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
        "document.addEventListener('keydown',function(e){"
        "if(!(e.ctrlKey||e.metaKey))return;"
        "var k=e.key;"
        "if(k!=='+'&&k!=='='&&k!=='-'&&k!=='0')return;"
        "e.preventDefault();"
        "var el=document.documentElement;"
        "var z=parseFloat(el.style.zoom||el.getAttribute('data-zoom')||'1');"
        "if(k==='+'||k==='=')z=Math.min(2,z+0.1);"
        "else if(k==='-')z=Math.max(0.5,z-0.1);"
        "else z=1;"
        "el.style.zoom=z;"
        "el.setAttribute('data-zoom',String(z));"
        "},true);"
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
            // Defer openFileDialog - avoid reentrancy with NSOpenPanel. Two run-loop iterations
            // give WebKit time to fully release the script message handler context before we
            // show the modal (prevents disabled/greyed dialog).
            BOOL isOpenFileDialog = ([jsonString rangeOfString:@"\"type\":\"openFileDialog\""].location != NSNotFound ||
                                    [jsonString rangeOfString:@"\"type\": \"openFileDialog\""].location != NSNotFound);
            if (isOpenFileDialog) {
                std::string msgCopy = jsonMsg;
                MessageCallback cb = self.messageCallback;
                void* ud = self.messageUserData;
                dispatch_async(dispatch_get_main_queue(), ^{
                    dispatch_async(dispatch_get_main_queue(), ^{
                        if (cb) cb(msgCopy, ud);
                    });
                });
            } else {
                self.messageCallback(jsonMsg, self.messageUserData);
            }
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

void setWebViewPreloadScript(void* webViewHandle, const std::string& scriptContent) {
    @autoreleasepool {
        if (!webViewHandle) return;
        WKWebView *webView = (__bridge WKWebView*)webViewHandle;
        if (scriptContent.empty()) {
            objc_setAssociatedObject(webView, @"customPreloadScript", nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        } else {
            NSString *script = [NSString stringWithUTF8String:scriptContent.c_str()];
            objc_setAssociatedObject(webView, @"customPreloadScript", script, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        }
    }
}

static NSString* getPreloadScriptForWebView(WKWebView *webView) {
    NSString *custom = objc_getAssociatedObject(webView, @"customPreloadScript");
    if (custom && [custom length] > 0) {
        return custom;
    }
    return [WebViewMessageHandler crossdevBridgeScript];
}

void* createWebView(void* parentHandle, int x, int y, int width, int height) {
    @autoreleasepool {
        if (!parentHandle) {
            return nullptr;
        }
        
        NSView *parentView = nullptr;
        
        // Get parent view - could be NSWindow's contentView or an NSView (from Container)
        if ([(__bridge id)parentHandle isKindOfClass:[NSWindow class]]) {
            NSWindow *window = (__bridge NSWindow*)parentHandle;
            parentView = [window contentView];
        } else if ([(__bridge id)parentHandle isKindOfClass:[NSView class]]) {
            parentView = (__bridge NSView*)parentHandle;
        } else {
            return nullptr;
        }
        
        NSRect webViewRect = NSMakeRect(x, y, width, height);
        
        // Create configuration with developer extras enabled for inspector/console access
        WKWebViewConfiguration *config = [[WKWebViewConfiguration alloc] init];
        // Enable developer extras (inspector/console)
        if ([config.preferences respondsToSelector:@selector(setValue:forKey:)]) {
            [config.preferences setValue:@YES forKey:@"developerExtrasEnabled"];
        }
        
        WKWebView *webView = [[ClickThroughWebView alloc] initWithFrame:webViewRect configuration:config];
        [webView setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
        
        [parentView addSubview:webView];
        
        return (void*)CFBridgingRetain(webView);
    }
}

void destroyWebView(void* webViewHandle) {
    @autoreleasepool {
        if (webViewHandle) {
            WKWebView *webView = (__bridge WKWebView*)webViewHandle;
            [webView removeFromSuperview];
            CFBridgingRelease(webViewHandle);
        }
    }
}

void resizeWebView(void* webViewHandle, int width, int height) {
    @autoreleasepool {
        if (webViewHandle) {
            WKWebView *webView = (__bridge WKWebView*)webViewHandle;
            NSRect newFrame = NSMakeRect(0, 0, width, height);
            [webView setFrame:newFrame];
        }
    }
}

void loadHTMLFile(void* webViewHandle, const std::string& filePath) {
    @autoreleasepool {
        if (!webViewHandle) {
            return;
        }
        
        WKWebView *webView = (__bridge WKWebView*)webViewHandle;
        
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
        
        NSString *htmlString = [NSString stringWithUTF8String:htmlContent.c_str()];
        NSURL *baseURL = [NSURL fileURLWithPath:[NSString stringWithUTF8String:filePath.c_str()]];
        
        [webView loadHTMLString:htmlString baseURL:baseURL];
    }
}

void loadHTMLString(void* webViewHandle, const std::string& html) {
    @autoreleasepool {
        if (!webViewHandle) {
            return;
        }
        
        WKWebView *webView = (__bridge WKWebView*)webViewHandle;
        NSString *htmlString = [NSString stringWithUTF8String:html.c_str()];
        
        // Load HTML first
        [webView loadHTMLString:htmlString baseURL:nil];
        
        // Re-inject bridge after page loads (in case WKUserScript ran before callback was set)
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.1 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
            NSString *script = getPreloadScriptForWebView(webView);
            [webView evaluateJavaScript:script completionHandler:nil];
        });
    }
}

void loadURL(void* webViewHandle, const std::string& url) {
    @autoreleasepool {
        if (!webViewHandle) {
            return;
        }
        
        WKWebView *webView = (__bridge WKWebView*)webViewHandle;
        NSString *urlString = [NSString stringWithUTF8String:url.c_str()];
        NSURL *nsURL = [NSURL URLWithString:urlString];
        if (nsURL) {
            NSURLRequest *request = [NSURLRequest requestWithURL:nsURL];
            [webView loadRequest:request];
        }
    }
}

void setWebViewCreateWindowCallback(void* webViewHandle, void (*callback)(const std::string& title, void* userData), void* userData) {
    @autoreleasepool {
        if (!webViewHandle || !callback) {
            return;
        }
        
        WKWebView *webView = (__bridge WKWebView*)webViewHandle;
        WKUserContentController *userContentController = webView.configuration.userContentController;
        
        // Get or create message handler
        WebViewMessageHandler *handler = objc_getAssociatedObject(webView, @"messageHandler");
        if (!handler) {
            handler = [[WebViewMessageHandler alloc] init];
            objc_setAssociatedObject(webView, @"messageHandler", handler, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
            
            // Add script message handler for "nativeMessage" (generic handler)
            [userContentController addScriptMessageHandler:handler name:@"nativeMessage"];
            
            // Inject JavaScript bridge (custom preload or built-in) in page world
            NSString *script = getPreloadScriptForWebView(webView);
            WKUserScript *userScript;
            if (@available(macOS 11.0, *)) {
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
        
        WKWebView *webView = (__bridge WKWebView*)webViewHandle;
        WKUserContentController *userContentController = webView.configuration.userContentController;
        
        // Get or create message handler
        WebViewMessageHandler *handler = objc_getAssociatedObject(webView, @"messageHandler");
        if (!handler) {
            handler = [[WebViewMessageHandler alloc] init];
            objc_setAssociatedObject(webView, @"messageHandler", handler, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
            
            // Add script message handler for "nativeMessage" (generic handler)
            [userContentController addScriptMessageHandler:handler name:@"nativeMessage"];
            
            // Add navigation delegate to inject bridge when page loads (critical for loadURL - page may load after we set callback)
            BridgeInjectNavigationDelegate *navDelegate = [[BridgeInjectNavigationDelegate alloc] init];
            objc_setAssociatedObject(webView, @"bridgeInjectDelegate", navDelegate, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
            webView.navigationDelegate = navDelegate;
            
            // Inject JavaScript bridge (custom preload or built-in) in page world
            NSString *script = getPreloadScriptForWebView(webView);
            WKUserScript *userScript;
            if (@available(macOS 11.0, *)) {
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

void postMessageToJavaScript(void* webViewHandle, const std::string& jsonMessage) {
    @autoreleasepool {
        if (!webViewHandle) {
            return;
        }
        
        WKWebView *webView = (__bridge WKWebView*)webViewHandle;
        NSString *jsonString = [NSString stringWithUTF8String:jsonMessage.c_str()];
        NSString *jsCode = [NSString stringWithFormat:@"window.postMessage(%@, '*');", jsonString];
        [webView evaluateJavaScript:jsCode completionHandler:nil];
    }
}

void executeWebViewScript(void* webViewHandle, const std::string& script) {
    @autoreleasepool {
        if (!webViewHandle || script.empty()) return;
        WKWebView *webView = (__bridge WKWebView*)webViewHandle;
        NSString *nsScript = [NSString stringWithUTF8String:script.c_str()];
        [webView evaluateJavaScript:nsScript completionHandler:nil];
    }
}

void printWebView(void* webViewHandle) {
    if (!webViewHandle) return;
    WKWebView *webView = (__bridge WKWebView*)webViewHandle;
    // Hide header (same as @media print) via JS, then use native print.
    const char *hideScript = R"(
(function(){
  var sel = '.app-header, .app-header *, .alerts-view, .alerts-view *, #app > header';
  var els = document.querySelectorAll(sel);
  window.__crossdevPrintHidden = [];
  var main = document.querySelector('.app-main');
  if (main) {
    window.__crossdevPrintHidden.push({el: main, prop: 'paddingTop', val: main.style.paddingTop});
    main.style.paddingTop = '0';
  }
  els.forEach(function(e) {
    if (e.offsetParent !== null) {
      window.__crossdevPrintHidden.push({el: e, display: e.style.display, visibility: e.style.visibility});
      e.style.setProperty('display','none','important');
      e.style.setProperty('visibility','hidden','important');
    }
  });
  // Force layout/paint of report content (including data-URL logo) so print captures it
  var report = document.querySelector('.print-report, .print-report-header');
  if (report) { void report.offsetHeight; }
  var imgs = document.querySelectorAll('.letter-head[src^="data:"]');
  imgs.forEach(function(img) { if (img.complete) void img.naturalWidth; });
})();
)";
    executeWebViewScript(webViewHandle, std::string(hideScript));
    // WKWebView print produces blank pages unless view frame is set (macOS quirk).
    // Delay so data-URL images are committed to the layer tree before print capture.
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(250 * NSEC_PER_MSEC)), dispatch_get_main_queue(), ^{
        NSPrintInfo *info = [NSPrintInfo sharedPrintInfo];
        NSPrintOperation *op = [webView printOperationWithPrintInfo:info];
        if (op && op.view) {
            NSRect bounds = webView.bounds;
            CGFloat w = bounds.size.width >= 100 ? bounds.size.width : 612;
            CGFloat h = bounds.size.height >= 100 ? bounds.size.height : 792;
            op.view.frame = NSMakeRect(0, 0, w, h);
        }
        NSWindow *win = webView.window;
        if (op && win) {
            [op runOperationModalForWindow:win delegate:nil didRunSelector:NULL contextInfo:NULL];
        } else if (op) {
            [op runOperation];
        }
        // Restore header after print dialog closes
        const char *restoreScript = R"(
(function(){
  if (window.__crossdevPrintHidden) {
    window.__crossdevPrintHidden.forEach(function(r) {
      if (r.prop) r.el.style[r.prop] = r.val || '';
      else { r.el.style.display = r.display || ''; r.el.style.visibility = r.visibility || ''; }
    });
    window.__crossdevPrintHidden = null;
  }
})();
)";
        executeWebViewScript(webViewHandle, std::string(restoreScript));
    });
}

void openInspector(void* webViewHandle) {
    @autoreleasepool {
        if (!webViewHandle) return;
        WKWebView *webView = (__bridge WKWebView*)webViewHandle;
        // Try to open inspector/console using the private API selector if available
        // On macOS, Cmd+Option+I triggers the inspector if developer extras are enabled
        SEL selector = NSSelectorFromString(@"_setInspectable:");
        if ([webView respondsToSelector:selector]) {
            // Use the private selector to enable inspector (developer tools)
            IMP imp = [webView methodForSelector:selector];
            void (*func)(id, SEL, BOOL) = (void (*)(id, SEL, BOOL))imp;
            func(webView, selector, YES);
        }
        // Also try the keyboard shortcut way: simulate Cmd+Option+I
        NSEvent *keyDown = [NSEvent keyEventWithType:NSEventTypeKeyDown
                                              location:NSZeroPoint
                                         modifierFlags:(NSEventModifierFlagCommand | NSEventModifierFlagOption)
                                             timestamp:[NSDate timeIntervalSinceReferenceDate]
                                          windowNumber:0
                                               context:nil
                                            characters:@"i"
                                           charactersIgnoringModifiers:@"i"
                                             isARepeat:NO
                                               keyCode:0x22]; // 'i' key code
        if (keyDown && webView.window) {
            [webView.window sendEvent:keyDown];
        }
    }
}

} // namespace platform

#endif // PLATFORM_MACOS
