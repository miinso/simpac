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
const dummyRect = { left: 0, top: 0, width: 0, height: 0 };
const dummyStyle = {
  removeProperty: noop,
  setProperty: noop
};
const dummyElement = {
  style: dummyStyle,
  addEventListener: noop,
  removeEventListener: noop,
  getBoundingClientRect: () => dummyRect,
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
if (!self.window.addEventListener) {
  self.window.addEventListener = () => {};
  self.window.removeEventListener = () => {};
}

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

async function init(scriptUrl, wasmUrl, offscreenCanvas, exportsList, cwrapList, webglContextAttributes) {
  canvas = offscreenCanvas || null;
  if (canvas) {
    if (!canvas.style) canvas.style = { removeProperty: noop, setProperty: noop };
    if (typeof canvas.style.removeProperty !== 'function') canvas.style.removeProperty = noop;
    if (typeof canvas.style.setProperty !== 'function') canvas.style.setProperty = noop;
    if (typeof canvas.addEventListener !== 'function') canvas.addEventListener = noop;
    if (typeof canvas.removeEventListener !== 'function') canvas.removeEventListener = noop;
    if (typeof canvas.getBoundingClientRect !== 'function') {
      canvas.getBoundingClientRect = () => dummyRect;
    }
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

function callEngine(name, ...args) {
  const fn = moduleInstance && moduleInstance[name];
  if (typeof fn === 'function') fn(...args);
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

  if (!moduleInstance) return;

  // Input Bridge
  switch (type) {
    case 'mousemove':
      callEngine('_engine_mouse_move', data.x, data.y, data.dx, data.dy);
      break;
    case 'mousedown':
      callEngine('_engine_mouse_down', data.button, data.x, data.y);
      break;
    case 'mouseup':
      callEngine('_engine_mouse_up', data.button, data.x, data.y);
      break;
    case 'wheel':
      callEngine('_engine_mouse_wheel', data.dx, data.dy);
      break;
    case 'mouseenter':
      callEngine('_engine_mouse_enter', 1);
      break;
    case 'mouseleave':
      callEngine('_engine_mouse_enter', 0);
      break;
    case 'keydown':
      callEngine('_engine_key_down', data.key);
      break;
    case 'keyup':
      callEngine('_engine_key_up', data.key);
      break;
    case 'touch':
      const list = Array.isArray(payload) ? payload : [];
      if (moduleInstance && typeof moduleInstance._engine_set_touch_count === 'function') {
        moduleInstance._engine_set_touch_count(list.length);
        for (let i = 0; i < list.length; i++) {
          moduleInstance._engine_set_touch(i, list[i].x, list[i].y);
        }
      }
      break;
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
            entry.target.removeEventListener(entry.type, entry.handler, entry.options);
        }
        this._listeners = [];
    }

    _attachInputListeners() {
        const c = this.canvas;

        const getPos = (e) => {
            const r = c.getBoundingClientRect();
            return { x: e.clientX - r.left, y: e.clientY - r.top };
        };

        this._addListener(c, 'mousemove', e => {
            const p = getPos(e);
            if (!this.worker) return;
            this.worker.postMessage({ type: 'mousemove', x: p.x, y: p.y, dx: e.movementX, dy: e.movementY });
        });

        this._addListener(c, 'mousedown', e => {
            const p = getPos(e); // Mouse updates pos too
            if (!this.worker) return;
            this.worker.postMessage({ type: 'mousedown', x: p.x, y: p.y, button: e.button });
        });

        this._addListener(c, 'mouseup', e => {
            const p = getPos(e);
            if (!this.worker) return;
            this.worker.postMessage({ type: 'mouseup', x: p.x, y: p.y, button: e.button });
        });

        this._addListener(c, 'wheel', e => {
            e.preventDefault();
            if (!this.worker) return;
            this.worker.postMessage({ type: 'wheel', dx: e.deltaX, dy: e.deltaY });
        }, { passive: false });

        this._addListener(c, 'mouseenter', () => {
            if (!this.worker) return;
            this.worker.postMessage({ type: 'mouseenter' });
        });
        this._addListener(c, 'mouseleave', () => {
            if (!this.worker) return;
            this.worker.postMessage({ type: 'mouseleave' });
        });

        // Context menu prevent (opt-in)
        if (!this.options.allowContextMenu) {
            this._addListener(c, 'contextmenu', e => e.preventDefault());
        }

        // Keyboard
        c.tabIndex = 0;
        c.style.outline = 'none';
        this._addListener(c, 'keydown', e => {
            if (['ArrowUp', 'ArrowDown', 'ArrowLeft', 'ArrowRight', ' '].includes(e.key)) e.preventDefault();
            if (!this.worker) return;
            this.worker.postMessage({ type: 'keydown', key: e.keyCode });
        });
        this._addListener(c, 'keyup', e => {
            if (!this.worker) return;
            this.worker.postMessage({ type: 'keyup', key: e.keyCode });
        });

        // Touch
        const handleTouch = (e) => {
            e.preventDefault();
            const r = c.getBoundingClientRect();
            const touches = [];
            for (let i = 0; i < e.touches.length; i++) {
                const t = e.touches[i];
                touches.push({ x: t.clientX - r.left, y: t.clientY - r.top });
            }
            this.worker.postMessage({ type: 'touch', payload: touches });
        };
        this._addListener(c, 'touchstart', handleTouch, { passive: false });
        this._addListener(c, 'touchmove', handleTouch, { passive: false });
        this._addListener(c, 'touchend', handleTouch, { passive: false });
        this._addListener(c, 'touchcancel', handleTouch, { passive: false });
    }
}
