import { reactive } from 'vue';
import { flecs } from '../components/remote.js';
import { createConfigApi } from './config-api.js';
import { createMathApi, bindSystemToggles } from './math-api.js';
import { createScriptApi } from './script-api.js';
import { createSystemApi } from './system-toggle.js';

export function useEngineBlock(appRef, options = {}) {
  const state = reactive({
    status: 'booting',
    ready: false,
    error: '',
  });

  const math = createMathApi();
  let conn = null;
  let systems = null;
  let config = null;
  let script = null;

  function host() {
    return appRef?.value || appRef;
  }

  function ensureConn() {
    if (!conn) throw new Error('engine not connected');
    return conn;
  }

  function ensureSystems() {
    if (!systems) systems = createSystemApi(ensureConn(), options.systems);
    return systems;
  }

  function ensureConfig() {
    if (!config) config = createConfigApi(ensureConn());
    return config;
  }

  function ensureScript() {
    if (!script) script = createScriptApi(ensureConn());
    return script;
  }

  async function connect() {
    try {
      conn = flecs.connect(host());
      systems = createSystemApi(conn, options.systems);
      config = createConfigApi(conn);
      script = createScriptApi(conn);
      state.ready = true;
      state.status = 'ready';
      state.error = '';
      return conn;
    } catch (error) {
      state.ready = false;
      state.status = 'error';
      state.error = String(error?.message || error);
      throw error;
    }
  }

  function disconnect() {
    conn?.disconnect?.();
    conn = null;
    systems = null;
    config = null;
    script = null;
    state.ready = false;
    state.status = 'disconnected';
  }

  function makeSystemToggleDefs(map = {}) {
    const defs = {};
    for (const [key, entry] of Object.entries(map)) {
      defs[key] = {
        kind: 'toggle',
        label: entry?.label || key,
        default: entry?.default ?? true,
        classes: entry?.classes || [],
        bind: { system: entry?.system || '' },
      };
    }
    return defs;
  }

  function bindMathSystemToggles({ terms, defs, onError } = {}) {
    const systemsProxy = {
      setEnabled: (...args) => ensureSystems().setEnabled(...args),
    };
    const onToggle = bindSystemToggles({
      terms,
      defs,
      systems: systemsProxy,
      onError,
    });
    return math.bind({ terms, defs, onToggle });
  }

  function createMathSystemToggles(map = {}, options = {}) {
    const defs = makeSystemToggleDefs(map);
    const terms = math.createTerms(defs);
    const binding = bindMathSystemToggles({
      terms,
      defs,
      onError: options?.onError,
    });

    async function sync(params = {}) {
      const rows = await ensureSystems().listMap(params);
      for (const [key, def] of Object.entries(defs)) {
        const row = rows.get(def?.bind?.system);
        if (!row || terms?.[key]?.kind !== 'toggle') continue;
        terms[key].active = !!row.enabled;
      }
      binding.sync?.();
      return terms;
    }

    return {
      defs,
      terms,
      attach: binding.attach,
      detach: binding.detach,
      sync,
    };
  }

  return {
    state,
    connect,
    disconnect,
    get conn() {
      return ensureConn();
    },
    systems: {
      list: (params) => ensureSystems().list(params),
      listMap: (params) => ensureSystems().listMap(params),
      setEnabled: (path, enabled) => ensureSystems().setEnabled(path, enabled),
      toggle: (path, enabled) => ensureSystems().toggle(path, enabled),
      defs: makeSystemToggleDefs,
    },
    config: {
      get: (...args) => ensureConfig().get(...args),
      set: (...args) => ensureConfig().set(...args),
      bind: (...args) => ensureConfig().bind(...args),
    },
    script: {
      get: (...args) => ensureScript().get(...args),
      apply: (...args) => ensureScript().apply(...args),
    },
    math: {
      createTerms: (...args) => math.createTerms(...args),
      bind: (...args) => math.bind(...args),
      bindSystemToggles: bindMathSystemToggles,
      systemToggles: createMathSystemToggles,
    },
  };
}
