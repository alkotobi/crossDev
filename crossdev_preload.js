/**
 * CrossDev Preload Script Template
 *
 * Use this as a custom preload: set htmlLoading.preloadPath to "crossdev_preload.js"
 * in options.json. Must expose CrossDev.invoke() and CrossDev.events for MessageRouter.
 *
 * Bridge is non-overridable: Object.defineProperty + Object.freeze so the page cannot
 * replace or delete window.CrossDev.
 *
 * Binary support: Pass ArrayBuffer/Uint8Array in payload - auto base64. Use
 * invoke(type, payload, { binaryResponse: true }) to decode result.data to ArrayBuffer.
 *
 * Platform support:
 * - WebKit (macOS/iOS): window.webkit.messageHandlers.nativeMessage
 * - WebView2 (Windows): window.chrome.webview.postMessage + addEventListener('message')
 */
;(function () {
  var _pending = new Map()
  var _eventListeners = {}
  var _nativeWebView2Post = null

  function _ab2b64(ab) {
    var u8 = ab instanceof Uint8Array ? ab : new Uint8Array(ab)
    var bin = ''
    for (var i = 0; i < u8.length; i++) bin += String.fromCharCode(u8[i])
    return btoa(bin)
  }
  function _b642ab(s) {
    var bin = atob(s)
    var u8 = new Uint8Array(bin.length)
    for (var i = 0; i < bin.length; i++) u8[i] = bin.charCodeAt(i)
    return u8.buffer
  }
  function _toWire(obj) {
    if (obj === null || typeof obj !== 'object') return obj
    if (obj instanceof ArrayBuffer) return { __base64: _ab2b64(obj) }
    if (ArrayBuffer.isView && ArrayBuffer.isView(obj))
      return {
        __base64: _ab2b64(obj.buffer.slice(obj.byteOffset, obj.byteOffset + obj.byteLength)),
      }
    if (Array.isArray(obj)) return obj.map(_toWire)
    var out = {}
    for (var k in obj) if (Object.prototype.hasOwnProperty.call(obj, k)) out[k] = _toWire(obj[k])
    return out
  }

  function _handleMessage(d) {
    if (!d) return
    if (d.type === 'crossdev:event') {
      var name = d.name,
        payload = d.payload || {}
      var list = _eventListeners[name]
      if (list)
        list.forEach(function (fn) {
          try {
            fn(payload)
          } catch (err) {
            console.error(err)
          }
        })
      return
    }
    if (d.requestId) {
      var h = _pending.get(d.requestId)
      if (h) {
        _pending.delete(d.requestId)
        var res = d.result
        if (h.binary && res && typeof res.data === 'string') {
          res = Object.assign({}, res)
          res.data = _b642ab(res.data)
        }
        console.log(
          '[CrossDev] Response received for requestId:',
          d.requestId,
          'error:',
          d.error,
          'result:',
          d.error ? null : res && typeof res === 'object' ? JSON.stringify(res).slice(0, 100) : res,
        )
        d.error
          ? h.reject(
              new Error(typeof d.error === 'string' ? d.error || 'Unknown error' : String(d.error)),
            )
          : h.resolve(res)
      }
    }
  }

  window.addEventListener('message', function (e) {
    var d = e.data
    if (d && typeof d === 'string')
      try {
        d = JSON.parse(d)
      } catch (_) {}
    _handleMessage(d)
  })

  function _ensureWebView2Listener() {
    if (_nativeWebView2Post) return true
    var wv = window.chrome && window.chrome.webview
    if (!wv || !wv.postMessage || typeof wv.postMessage !== 'function') return false
    _nativeWebView2Post = wv.postMessage
    wv.addEventListener('message', function (evt) {
      var d = evt.data
      if (d && typeof d === 'string')
        try {
          d = JSON.parse(d)
        } catch (_) {}
      _handleMessage(d)
    })
    return true
  }

  function _post(msg) {
    if (
      window.webkit &&
      window.webkit.messageHandlers &&
      window.webkit.messageHandlers.nativeMessage
    ) {
      window.webkit.messageHandlers.nativeMessage.postMessage(msg)
    } else if (window.chrome && window.chrome.webview) {
      _ensureWebView2Listener()
      var post = _nativeWebView2Post
      if (post)
        post.call(window.chrome.webview, typeof msg === 'string' ? msg : JSON.stringify(msg))
    }
  }

  var CrossDev = {
    invoke: function (type, payload, opts) {
      var opt = opts || {}
      return new Promise(function (resolve, reject) {
        var rid = Date.now() + '-' + Math.random()
        _pending.set(rid, { resolve: resolve, reject: reject, binary: !!opt.binaryResponse })
        console.log(
          '[CrossDev] invoke:',
          type,
          'requestId:',
          rid,
          'payload:',
          JSON.stringify(payload || {}).slice(0, 120),
        )
        // File dialogs can take a while - use 120s for openFileDialog, 30s for others
        var timeoutMs = type === 'openFileDialog' ? 120000 : 30000
        setTimeout(function () {
          if (_pending.has(rid)) {
            _pending.delete(rid)
            console.error('[CrossDev] Request timeout for requestId:', rid, 'type:', type)
            reject(new Error('Request timeout'))
          }
        }, timeoutMs)
        var msg = { type: type, payload: _toWire(payload || {}), requestId: rid }
        console.log('[CrossDev] Sending message to native:', JSON.stringify(msg).slice(0, 150))
        _post(msg)
      })
    },
    events: {
      on: function (name, fn) {
        if (!_eventListeners[name]) _eventListeners[name] = []
        _eventListeners[name].push(fn)
        return function () {
          var i = _eventListeners[name].indexOf(fn)
          if (i >= 0) _eventListeners[name].splice(i, 1)
        }
      },
    },
  }
  Object.freeze(CrossDev.events)
  Object.freeze(CrossDev)
  try {
    Object.defineProperty(window, 'CrossDev', {
      value: CrossDev,
      configurable: false,
      writable: false,
    })
  } catch (_) {
    window.CrossDev = CrossDev
  }

  function _setupWebView2() {
    if (!window.chrome) window.chrome = {}
    if (!window.chrome.webview) return false
    var wv = window.chrome.webview
    if (wv.postMessage && typeof wv.postMessage === 'function') {
      _nativeWebView2Post = wv.postMessage
      wv.addEventListener('message', function (evt) {
        var d = evt.data
        if (d && typeof d === 'string')
          try {
            d = JSON.parse(d)
          } catch (_) {}
        _handleMessage(d)
      })
      wv.postMessage = function (m) {
        var msg =
          typeof m === 'string'
            ? (function () {
                try {
                  return JSON.parse(m)
                } catch (_) {
                  return m
                }
              })()
            : m
        _post(msg)
      }
      return true
    }
    return false
  }
  if (!_setupWebView2()) {
    var _poll = setInterval(function () {
      if (_setupWebView2()) clearInterval(_poll)
    }, 50)
    setTimeout(function () {
      clearInterval(_poll)
    }, 30000)
  }

  // Chrome-style zoom shortcuts: Ctrl/Cmd + Plus/Minus/0
  document.addEventListener(
    'keydown',
    function (e) {
      if (!(e.ctrlKey || e.metaKey)) return
      var k = e.key
      if (k !== '+' && k !== '=' && k !== '-' && k !== '0') return
      e.preventDefault()
      var el = document.documentElement
      var z = parseFloat(el.style.zoom || el.getAttribute('data-zoom') || '1')
      if (k === '+' || k === '=') z = Math.min(2, z + 0.1)
      else if (k === '-') z = Math.max(0.5, z - 0.1)
      else z = 1
      el.style.zoom = z
      el.setAttribute('data-zoom', String(z))
    },
    true,
  )
})()
