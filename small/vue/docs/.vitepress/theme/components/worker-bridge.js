/* worker-bridge.js
 * 
 * Reusable implementation of the Simpac Input Bridge for Web Workers.
 * Usage:
 *   import { SimpacWorker } from './worker-bridge.js';
 *   const worker = new SimpacWorker(canvasElement, 'app.wasm', 'app.js', {
 *     exports: ['_leak_hold', '_leak_clear'],
 *     moduleFactoryName: 'createModule', // set to null for non-modular builds
 *     onStats: (stats) => console.log(stats),
 *   });
 *   worker.start();
 *   worker.call('_leak_hold', 1024);
 *   const reply = await worker.callCwrap(
 *     'flecs_explorer_request',
 *     'string',
 *     ['string', 'string', 'string'],
 *     'GET',
 *     '/query?expr=Tag',
 *     ''
 *   );
 */

export const WORKER_BOOTSTRAP_CODE = `
let canvas = null;
const noop = () => {};

// dom polyfill: event listener storage + dispatch for offscreen canvas
let _cachedRect = { left: 0, top: 0, width: 0, height: 0, right: 0, bottom: 0, x: 0, y: 0 };
const _canvasL = {};
const _windowL = {};
function _addL(map, type, handler) {
  if (!map[type]) map[type] = [];
  map[type].push(handler);
}
function _removeL(map, type, handler) {
  if (!map[type]) return;
  map[type] = map[type].filter(h => h !== handler);
}
function _dispatchL(map, type, event) {
  const list = map[type];
  if (!list) return;
  for (let i = 0; i < list.length; i++) list[i](event);
}

const dummyStyle = {
  removeProperty: noop,
  setProperty: noop
};
const dummyElement = {
  style: dummyStyle,
  addEventListener: noop,
  removeEventListener: noop,
  getBoundingClientRect: () => _cachedRect,
  getContext: () => null
};

function getCanvasTarget() {
  return canvas || dummyElement;
}

self.window = self;
self.screen = self.screen || { addEventListener: noop, removeEventListener: noop };
self.window.screen = self.screen;
const documentStub = {
  hidden: false,
  visibilityState: 'visible',
  addEventListener: noop,
  removeEventListener: noop,
  getElementById: (id) => {
    const name = String(id || '');
    if (name === 'canvas') return getCanvasTarget();
    if (name === 'body') return documentStub.body;
    return dummyElement;
  },
  querySelector: (selector) => {
    const name = String(selector || '').toLowerCase();
    if (name === 'canvas' || name === '#canvas') return getCanvasTarget();
    if (name === 'body') return documentStub.body;
    if (name === 'html' || name === 'document') return documentStub.documentElement;
    return dummyElement;
  },
  querySelectorAll: (selector) => {
    const name = String(selector || '').toLowerCase();
    if (name === 'canvas' || name === '#canvas') return [getCanvasTarget()];
    if (name === 'body') return [documentStub.body];
    if (name === 'html' || name === 'document') return [documentStub.documentElement];
    return [];
  },
  getElementsByTagName: (tag) => {
    const name = String(tag || '').toLowerCase();
    if (name === 'canvas') return [getCanvasTarget()];
    if (name === 'body') return [documentStub.body];
    if (name === 'html') return [documentStub.documentElement];
    return [];
  },
  createElement: () => ({
    style: { removeProperty: noop, setProperty: noop },
    getContext: () => null,
    addEventListener: noop,
    removeEventListener: noop
  }),
  body: { style: dummyStyle, addEventListener: noop, removeEventListener: noop },
  documentElement: { style: dummyStyle, addEventListener: noop, removeEventListener: noop }
};
self.document = self.document || documentStub;
self.navigator = self.navigator || { userAgent: 'worker', platform: '' };

// Polyfills
if (!self.window.matchMedia) {
  self.window.matchMedia = () => ({
    matches: false,
    addListener: () => {},
    removeListener: () => {},
    addEventListener: () => {},
    removeEventListener: () => {}
  });
}
// window event polyfill: keyboard events are registered on window by GLFW
self.window.addEventListener = (type, handler) => _addL(_windowL, type, handler);
self.window.removeEventListener = (type, handler) => _removeL(_windowL, type, handler);
self.scrollX = 0;
self.scrollY = 0;

let moduleInstance = null;
let allowedExports = null;
let allowedCwrap = null;
const cwrapCache = new Map();
const send = (type, payload) =>
  postMessage(payload === undefined ? { type } : { type, payload });
const sendError = (type, id, message) => send(type, { id, message });

function setAllowedExports(list) {
  allowedExports = Array.isArray(list) ? new Set(list) : null;
}

function canCall(name) {
  return !allowedExports || allowedExports.has(name);
}

function setAllowedCwrap(list) {
  allowedCwrap = Array.isArray(list) ? new Set(list) : null;
}

function canCwrap(name) {
  return !allowedCwrap || allowedCwrap.has(name);
}

function getCwrap(name, returnType, argTypes) {
  const types = Array.isArray(argTypes) ? argTypes : [];
  const key = name + '|' + (returnType || 'number') + '|' + types.join(',');
  if (cwrapCache.has(key)) {
    return cwrapCache.get(key);
  }
  const fn = moduleInstance.cwrap(name, returnType || 'number', types);
  cwrapCache.set(key, fn);
  return fn;
}

async function loadScript(scriptUrl) {
  try {
    const response = await fetch(scriptUrl, { cache: 'no-store' });
    if (response.ok) {
      const text = await response.text();
      const blob = new Blob([text], { type: 'text/javascript' });
      const blobUrl = URL.createObjectURL(blob);
      importScripts(blobUrl);
      URL.revokeObjectURL(blobUrl);
      return;
    }
  } catch (err) {}
  importScripts(scriptUrl);
}

function getWasmBytes() {
  const wasmMemory = moduleInstance?.wasmMemory?.buffer || moduleInstance?.HEAP8?.buffer;
  return wasmMemory ? wasmMemory.byteLength : 0;
}

function postStats() {
  if (!moduleInstance) {
    send('stats', { module: false });
    return;
  }
  const heldBytes = moduleInstance._leak_held_bytes ? moduleInstance._leak_held_bytes() : 0;
  const heldBlocks = moduleInstance._leak_block_count ? moduleInstance._leak_block_count() : 0;
  send('stats', {
    module: true,
    wasmBytes: getWasmBytes(),
    heldBytes,
    heldBlocks
  });
}

function handleCall(payload) {
  const { id, fn, args } = payload || {};
  if (!moduleInstance) {
    sendError('callError', id, 'module not ready');
    return;
  }
  if (!fn || typeof moduleInstance[fn] !== 'function') {
    sendError('callError', id, 'function missing');
    return;
  }
  if (!canCall(fn)) {
    sendError('callError', id, 'function not allowed');
    return;
  }
  try {
    const result = moduleInstance[fn](...(args || []));
    send('callResult', { id, result });
  } catch (err) {
    sendError('callError', id, String(err));
  }
}

function handleCwrap(payload) {
  const { id, name, returnType, argTypes, args } = payload || {};
  if (!moduleInstance) {
    sendError('cwrapError', id, 'module not ready');
    return;
  }
  if (!name) {
    sendError('cwrapError', id, 'name missing');
    return;
  }
  if (typeof moduleInstance.cwrap !== 'function') {
    sendError('cwrapError', id, 'cwrap not available');
    return;
  }
  if (!canCwrap(name)) {
    sendError('cwrapError', id, 'function not allowed');
    return;
  }
  try {
    const fn = getCwrap(name, returnType, argTypes);
    const result = fn(...(args || []));
    send('cwrapResult', { id, result });
  } catch (err) {
    sendError('cwrapError', id, String(err));
  }
}

function handleFsRead(payload) {
  const { id, path, encoding } = payload || {};
  if (!moduleInstance) {
    sendError('fsReadError', id, 'module not ready');
    return;
  }
  if (!path) {
    sendError('fsReadError', id, 'path missing');
    return;
  }
  if (!moduleInstance.FS || typeof moduleInstance.FS.readFile !== 'function') {
    sendError('fsReadError', id, 'FS.readFile missing');
    return;
  }
  try {
    const result = moduleInstance.FS.readFile(path, { encoding: encoding || 'utf8' });
    send('fsReadResult', { id, result });
  } catch (err) {
    sendError('fsReadError', id, String(err));
  }
}

function handleFsWrite(payload) {
  const { id, path, contents, encoding } = payload || {};
  if (!moduleInstance) {
    sendError('fsWriteError', id, 'module not ready');
    return;
  }
  if (!path) {
    sendError('fsWriteError', id, 'path missing');
    return;
  }
  if (!moduleInstance.FS || typeof moduleInstance.FS.writeFile !== 'function') {
    sendError('fsWriteError', id, 'FS.writeFile missing');
    return;
  }
  try {
    moduleInstance.FS.writeFile(path, contents ?? '', { encoding: encoding || 'utf8' });
    send('fsWriteResult', { id, result: true });
  } catch (err) {
    sendError('fsWriteError', id, String(err));
  }
}

async function init(scriptUrl, wasmUrl, offscreenCanvas, exportsList, cwrapList, webglContextAttributes) {
  canvas = offscreenCanvas || null;
  if (canvas) {
    // OffscreenCanvas has no .id -- emscripten's SetCanvasIdJs does "#" + Module.canvas.id
    // without this, findEventTarget("#undefined") silently fails and touch callbacks never register
    if (!canvas.id) canvas.id = 'canvas';
    if (!canvas.style) canvas.style = { removeProperty: noop, setProperty: noop };
    if (typeof canvas.style.removeProperty !== 'function') canvas.style.removeProperty = noop;
    if (typeof canvas.style.setProperty !== 'function') canvas.style.setProperty = noop;
    // dom polyfill: real event listener storage on canvas
    canvas.addEventListener = (type, handler) => _addL(_canvasL, type, handler);
    canvas.removeEventListener = (type, handler) => _removeL(_canvasL, type, handler);
    canvas.dispatchEvent = (event) => _dispatchL(_canvasL, event.type, event);
    canvas.getBoundingClientRect = () => _cachedRect;
    canvas.focus = noop;
    canvas.setPointerCapture = noop;
    canvas.releasePointerCapture = noop;
    try {
      Object.defineProperty(canvas, 'clientWidth', { get: () => _cachedRect.width, configurable: true });
      Object.defineProperty(canvas, 'clientHeight', { get: () => _cachedRect.height, configurable: true });
    } catch(e) {}
  }
  setAllowedExports(exportsList);
  setAllowedCwrap(cwrapList);
  let factoryUsed = false;
  let readySent = false;
  const signalReady = (instance) => {
    if (readySent) return;
    readySent = true;
    moduleInstance = instance;
    send('ready');
  };

  const config = {
    canvas,
    webglContextAttributes: webglContextAttributes || undefined,
    locateFile: (path) => {
      if (path.endsWith('.wasm') && wasmUrl) return wasmUrl;
      return new URL(path, scriptUrl).toString();
    },
    print: (text) => send('log', { text }),
    printErr: (text) => send('error', { message: text }),
    onRuntimeInitialized: () => {
      if (factoryUsed) return;
      signalReady(self.Module || moduleInstance);
    }
  };
  self.Module = config;

  // Fetch WASM (optional support for single-file builds)
  let wasmBinary = null;
  if (wasmUrl) {
    try {
      const response = await fetch(wasmUrl, { cache: 'no-store' });
      if (response.ok) wasmBinary = await response.arrayBuffer();
      else send('log', { text: 'WASM fetch failed, assuming single-file build' });
    } catch (err) {
      send('log', { text: 'WASM fetch error, assuming single-file build' });
    }
  }
  if (wasmBinary) config.wasmBinary = wasmBinary;

  // Load main script after Module config is set
  try {
    await loadScript(scriptUrl);
  } catch (e) {
    send('error', { message: 'Failed to import script', detail: String(e) });
    return;
  }

  const factoryName = self.__SIMPAC_FACTORY_NAME;
  if (factoryName) {
    const factory = self[factoryName];
    if (typeof factory === 'function') {
      factoryUsed = true;
      try {
        const instance = await factory(config);
        signalReady(instance);
      } catch (err) {
        send('error', { message: 'createModule failed', detail: String(err) });
      }
      return;
    }
    send('log', { text: 'Factory ' + factoryName + ' missing, using Module global' });
  }

  if (!self.Module) {
    send('error', { message: 'Module global missing after load' });
  }
}

onmessage = (event) => {
  const data = event.data || {};
  const { type, payload } = data;

  if (type === 'init') {
    if (payload && Object.prototype.hasOwnProperty.call(payload, 'moduleFactoryName')) {
      self.__SIMPAC_FACTORY_NAME = payload.moduleFactoryName || '';
    } else {
      self.__SIMPAC_FACTORY_NAME = 'createModule';
    }
    init(
      payload.scriptUrl,
      payload.wasmUrl,
      payload.canvas,
      payload.exports,
      payload.cwrap,
      payload.webglContextAttributes
    );
    return;
  }

  if (type === 'stats') {
    postStats();
    return;
  }

  if (type === 'call') {
    handleCall(payload);
    return;
  }

  if (type === 'cwrap') {
    handleCwrap(payload);
    return;
  }

  if (type === 'fsRead') {
    handleFsRead(payload);
    return;
  }

  if (type === 'fsWrite') {
    handleFsWrite(payload);
    return;
  }

  if (type === 'shutdown') {
    if (moduleInstance && moduleInstance._emscripten_shutdown) {
      try {
        moduleInstance._emscripten_shutdown();
      } catch (err) {}
    }
    moduleInstance = null;
    send('shutdown');
    return;
  }

  // dom polyfill: dispatch forwarded events to stored GLFW/emscripten listeners
  if (type === 'input') {
    const { target, data: evData } = payload || {};
    if (!evData) return;
    evData.preventDefault = noop;
    evData.stopPropagation = noop;
    evData.stopImmediatePropagation = noop;
    if (target === 'window') {
      _dispatchL(_windowL, evData.type, evData);
    } else {
      evData.target = canvas;
      _dispatchL(_canvasL, evData.type, evData);
    }
    return;
  }

  if (type === 'size') {
    const { left, top, width, height } = payload || {};
    _cachedRect = {
      left: left || 0, top: top || 0,
      width: width || 0, height: height || 0,
      right: (left || 0) + (width || 0),
      bottom: (top || 0) + (height || 0),
      x: left || 0, y: top || 0
    };
    return;
  }
};
`;

/**
 * Controller class to manage the Worker logic from the Main Thread.
 */
export class SimpacWorker {
    constructor(canvasElement, wasmUrl, scriptUrl, options = {}) {
        this.canvas = canvasElement;
        this.wasmUrl = wasmUrl;
        this.scriptUrl = scriptUrl;
        this.worker = null;
        this.ready = false;
        const hasFactoryName = Object.prototype.hasOwnProperty.call(options, 'moduleFactoryName');
        this.options = {
            enableInput: options.enableInput !== false,
            allowContextMenu: options.allowContextMenu === true,
            webglContextAttributes: options.webglContextAttributes || null,
            exports: options.exports || null,
            cwrap: options.cwrap || null,
            moduleFactoryName: hasFactoryName ? options.moduleFactoryName : 'createModule',
            onReady: options.onReady || null,
            onLog: options.onLog || null,
            onError: options.onError || null,
            onStats: options.onStats || null,
            onShutdown: options.onShutdown || null
        };
        this._listeners = [];
        this._callId = 0;
        this._pendingCalls = new Map();
    }

    _sendRequest(type, payload = {}) {
        if (!this.worker) return Promise.reject(new Error('worker not running'));
        const id = ++this._callId;
        const request = { type, payload: { id, ...payload } };
        this.worker.postMessage(request);
        return new Promise((resolve, reject) => {
            this._pendingCalls.set(id, { resolve, reject });
        });
    }

    _resolvePending(id, value, isError) {
        const pending = this._pendingCalls.get(id);
        if (!pending) return;
        this._pendingCalls.delete(id);
        if (isError) pending.reject(new Error(value));
        else pending.resolve(value);
    }

    _rejectAllPending(message) {
        for (const pending of this._pendingCalls.values()) {
            pending.reject(new Error(message));
        }
        this._pendingCalls.clear();
    }

    start() {
        if (this.worker) return;

        if (!('transferControlToOffscreen' in HTMLCanvasElement.prototype)) {
            console.error('OffscreenCanvas not supported');
            return;
        }

        const offscreen = this.canvas.transferControlToOffscreen();

        // Create Worker from String Literal
        const blob = new Blob([WORKER_BOOTSTRAP_CODE], { type: 'text/javascript' });
        const blobUrl = URL.createObjectURL(blob);
        this.worker = new Worker(blobUrl);
        URL.revokeObjectURL(blobUrl);

        this.worker.onmessage = (e) => this._handleMessage(e);

        // Send Init
        this.worker.postMessage(
            {
                type: 'init',
                payload: {
                    scriptUrl: this.scriptUrl,
                    wasmUrl: this.wasmUrl,
                    canvas: offscreen,
                    exports: this.options.exports,
                    cwrap: this.options.cwrap,
                    moduleFactoryName: this.options.moduleFactoryName,
                    webglContextAttributes: this.options.webglContextAttributes
                }
            },
            [offscreen]
        );

        if (this.options.enableInput) {
            this._attachInputListeners();
        }
    }

    _handleMessage(e) {
        const { type, payload } = e.data || {};
        if (type === 'ready') {
            this.ready = true;
            if (this.options.onReady) this.options.onReady();
            else console.log('SimpacWorker: Ready');
        } else if (type === 'log') {
            if (this.options.onLog) this.options.onLog(payload.text);
            else console.log('WASM:', payload.text);
        } else if (type === 'error') {
            if (this.options.onError) this.options.onError(payload.message, payload.detail || '');
            else console.error('SimpacWorker Error:', payload.message, payload.detail || '');
        } else if (type === 'stats') {
            if (this.options.onStats) this.options.onStats(payload);
        } else if (type === 'callResult' || type === 'cwrapResult') {
            this._resolvePending(payload.id, payload.result, false);
        } else if (type === 'callError' || type === 'cwrapError') {
            this._resolvePending(payload.id, payload.message, true);
        } else if (type === 'fsReadResult' || type === 'fsWriteResult') {
            this._resolvePending(payload.id, payload.result, false);
        } else if (type === 'fsReadError' || type === 'fsWriteError') {
            this._resolvePending(payload.id, payload.message, true);
        } else if (type === 'cursor') {
            const { visible } = payload;
            this.canvas.style.cursor = visible ? 'default' : 'none';
        } else if (type === 'shutdown') {
            this.ready = false;
            if (this.options.onShutdown) this.options.onShutdown();
        }
    }

    requestStats() {
        if (!this.worker) return;
        this.worker.postMessage({ type: 'stats' });
    }

    call(fn, ...args) {
        return this._sendRequest('call', { fn, args });
    }

    callCwrap(name, returnType, argTypes, ...args) {
        return this._sendRequest('cwrap', { name, returnType, argTypes, args });
    }

    fsRead(path, encoding = 'utf8') {
        return this._sendRequest('fsRead', { path, encoding });
    }

    fsWrite(path, contents, encoding = 'utf8') {
        return this._sendRequest('fsWrite', { path, contents, encoding });
    }

    shutdown() {
        if (!this.worker) return;
        this.worker.postMessage({ type: 'shutdown' });
    }

    terminate() {
        if (!this.worker) return;
        this.worker.terminate();
        this.worker = null;
        this.ready = false;
        this._removeListeners();
        this._rejectAllPending('worker terminated');
    }

    _addListener(target, type, handler, options) {
        target.addEventListener(type, handler, options);
        this._listeners.push({ target, type, handler, options });
    }

    _removeListeners() {
        for (const entry of this._listeners) {
            if (!entry.target) { entry.handler(); continue; }
            entry.target.removeEventListener(entry.type, entry.handler, entry.options);
        }
        this._listeners = [];
    }

    _attachInputListeners() {
        const c = this.canvas;

        // send canvas rect so worker polyfill can serve getBoundingClientRect
        const sendSize = () => {
            if (!this.worker) return;
            const rect = c.getBoundingClientRect();
            this.worker.postMessage({ type: 'size', payload: {
                left: rect.left, top: rect.top,
                width: c.clientWidth, height: c.clientHeight
            }});
        };
        sendSize();

        // helpers: serialize raw DOM event properties and forward to worker
        const sendCanvas = (e, props) => {
            if (!this.worker) return;
            const data = { type: e.type };
            for (const p of props) data[p] = e[p];
            this.worker.postMessage({ type: 'input', payload: { target: 'canvas', data } });
        };
        const sendWindow = (e, props) => {
            if (!this.worker) return;
            const data = { type: e.type };
            for (const p of props) data[p] = e[p];
            this.worker.postMessage({ type: 'input', payload: { target: 'window', data } });
        };

        // mouse -> canvas (GLFW registers mouse handlers on canvas)
        const mouseProps = [
            'pageX', 'pageY', 'clientX', 'clientY', 'screenX', 'screenY',
            'movementX', 'movementY', 'button', 'buttons', 'offsetX', 'offsetY'
        ];
        for (const evt of ['mousemove', 'mousedown', 'mouseup', 'mouseenter', 'mouseleave', 'click']) {
            this._addListener(c, evt, e => sendCanvas(e, mouseProps));
        }

        // wheel -> canvas
        const wheelProps = [
            'deltaX', 'deltaY', 'deltaMode',
            'wheelDelta', 'wheelDeltaX', 'wheelDeltaY',
            'pageX', 'pageY', 'clientX', 'clientY'
        ];
        this._addListener(c, 'wheel', e => {
            e.preventDefault();
            sendCanvas(e, wheelProps);
        }, { passive: false });

        // touch -> canvas (GLFW + emscripten register touch handlers on canvas)
        const serializeTouches = (tl) => {
            if (!tl) return [];
            const arr = [];
            for (let i = 0; i < tl.length; i++) {
                const t = tl[i];
                arr.push({
                    identifier: t.identifier,
                    pageX: t.pageX, pageY: t.pageY,
                    clientX: t.clientX, clientY: t.clientY,
                    screenX: t.screenX, screenY: t.screenY
                });
            }
            return arr;
        };
        for (const evt of ['touchstart', 'touchmove', 'touchend', 'touchcancel']) {
            this._addListener(c, evt, e => {
                e.preventDefault();
                if (!this.worker) return;
                const mods = { ctrlKey: e.ctrlKey, shiftKey: e.shiftKey, altKey: e.altKey, metaKey: e.metaKey };
                const active = serializeTouches(e.touches);
                const changed = serializeTouches(e.changedTouches);
                const sendTouch = (touches, changedTouches, targetTouches) => {
                    this.worker.postMessage({ type: 'input', payload: { target: 'canvas', data: {
                        type: e.type, touches, changedTouches, targetTouches, ...mods
                    }}});
                };
                // raylib's touchend handler has `break` after removing one isChanged
                // touch, so multi-finger lift in one event leaves stale pointCount.
                // split into individual events so each has exactly one changed touch.
                if ((e.type === 'touchend' || e.type === 'touchcancel') && changed.length > 1) {
                    for (let k = 0; k < changed.length; k++) {
                        const remaining = active.concat(changed.slice(k + 1));
                        sendTouch(remaining, [changed[k]], remaining);
                    }
                } else {
                    sendTouch(active, changed, serializeTouches(e.targetTouches));
                }
            }, { passive: false });
        }

        // keyboard -> window (GLFW registers keyboard handlers on window)
        const keyProps = [
            'keyCode', 'key', 'code', 'charCode', 'which',
            'ctrlKey', 'metaKey', 'shiftKey', 'altKey', 'repeat'
        ];
        c.tabIndex = 0;
        c.style.outline = 'none';
        for (const evt of ['keydown', 'keyup', 'keypress']) {
            this._addListener(c, evt, e => {
                if (['ArrowUp', 'ArrowDown', 'ArrowLeft', 'ArrowRight', ' '].includes(e.key)) {
                    e.preventDefault();
                }
                sendWindow(e, keyProps);
            });
        }

        // context menu
        if (!this.options.allowContextMenu) {
            this._addListener(c, 'contextmenu', e => e.preventDefault());
        }

        // focus/blur -> canvas
        this._addListener(c, 'focus', e => sendCanvas(e, []));
        this._addListener(c, 'blur', e => sendCanvas(e, []));

        // resize -> update cached rect in worker
        // ResizeObserver is most reliable: fires after layout settles (orientation change, CSS resize, etc.)
        if (typeof ResizeObserver !== 'undefined') {
            const ro = new ResizeObserver(() => sendSize());
            ro.observe(c);
            this._listeners.push({ target: null, type: 'resizeobserver', handler: () => ro.disconnect() });
        }
        this._addListener(window, 'resize', sendSize);
        this._addListener(window, 'scroll', sendSize, { passive: true });
    }
}
