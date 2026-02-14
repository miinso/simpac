---
title: Config API
---

# Config API

`small/cloth` splits configuration into `props::` (user-tunable inputs) and `state::` (runtime frame state).
This page documents the entity contract and how to read/write it through `remote.js`.

<script setup>
import { onBeforeUnmount, ref } from 'vue';
import { Pane } from 'tweakpane';

const appRef = ref(null);
const paneHost = ref(null);
const status = ref('booting');

let conn = null;
let pane = null;
let statePollTimer = null;
const configurableRefs = new Map();
const stateModels = new Map();

const BOOL_CODEC = {
  toModel(value) {
    return Boolean(value);
  },
  fromModel(value) {
    return Boolean(value);
  },
};

const INT_CODEC = {
  toModel(value) {
    return Math.round(Number(value));
  },
  fromModel(value) {
    return Math.round(Number(value));
  },
};

const FLOAT_CODEC = {
  toModel(value) {
    return Number(value);
  },
  fromModel(value) {
    return Number(value);
  },
};

function makeVectorCodec(keys) {
  return {
    keys,
    toModel(value) {
      const model = {};
      for (const key of keys) model[key] = Number(value?.[key]);
      return model;
    },
    fromModel(modelValue, prevValue) {
      const next = { ...(prevValue || {}) };
      for (const key of keys) next[key] = Number(modelValue?.[key]);
      return next;
    },
  };
}

function toModelValue(entry, value) {
  return entry.schema.codec.toModel(value);
}

function fromModelValue(entry, modelValue, prevValue) {
  return entry.schema.codec.fromModel(modelValue, prevValue);
}

const CONFIG_COMPONENT_SCHEMAS = {
  'flecs.meta.bool': { kind: 'bool', codec: BOOL_CODEC },
  'flecs.meta.i32': { kind: 'int', step: 1, codec: INT_CODEC },
  'flecs.meta.f32': { kind: 'float', step: 0.001, codec: FLOAT_CODEC },
  'flecs.meta.f64': { kind: 'float', step: 0.001, codec: FLOAT_CODEC },
  vec3r: { kind: 'vec3', step: 0.1, codec: makeVectorCodec(['x', 'y', 'z']) },
  vec3f: { kind: 'vec3', step: 0.1, codec: makeVectorCodec(['x', 'y', 'z']) },
  color4f: { kind: 'color4', step: 0.01, min: 0, max: 1, codec: makeVectorCodec(['r', 'g', 'b', 'a']) },
};

function toDotPath(path) {
  return String(path || '').replaceAll('::', '.');
}

function toConfigurablePath(item) {
  if (item?.path) return toDotPath(item.path);
  if (item?.parent && item?.name) return `${toDotPath(item.parent)}.${item.name}`;
  return toDotPath(item?.name || '');
}

function splitPath(path) {
  const p = toDotPath(path);
  const last = p.lastIndexOf('.');
  if (last < 0) return { ns: '', label: p };
  return { ns: p.slice(0, last), label: p.slice(last + 1) };
}

function ensureFolder(root, folderMap, ns) {
  if (!ns) return root;
  let current = root;
  let key = '';
  for (const part of ns.split('.')) {
    key = key ? `${key}.${part}` : part;
    if (!folderMap.has(key)) folderMap.set(key, current.addFolder({ title: part }));
    current = folderMap.get(key);
  }
  return current;
}

function firstComponentEntry(components) {
  const entries = Object.entries(components || {});
  if (!entries.length) return [null, null];
  return entries[0];
}

function deepClone(value) {
  return value == null ? value : JSON.parse(JSON.stringify(value));
}

function getConfigurableRef(path, value) {
  if (!configurableRefs.has(path)) configurableRefs.set(path, ref(deepClone(value)));
  const r = configurableRefs.get(path);
  r.value = deepClone(value);
  return r;
}

function getStateModel(path, value) {
  if (!stateModels.has(path)) stateModels.set(path, { value: deepClone(value) });
  const model = stateModels.get(path);
  model.value = deepClone(value);
  return model;
}

async function queryConfigurableTable() {
  return conn.query('Configurable', {
    full_paths: true,
    table: true,
    values: true,
  });
}

async function queryStateTable() {
  return conn.query('(ChildOf, Scene)', {
    full_paths: true,
    table: true,
    values: true,
  });
}

async function logConfigurable() {
  const reply = await queryConfigurableTable();
  console.log('[query] Configurable table', reply);
}

async function logUnsupportedConfigurable() {
  const reply = await queryConfigurableTable();
  const unsupported = (reply?.results || []).filter((item) => {
    const [component] = firstComponentEntry(item?.components);
    return !CONFIG_COMPONENT_SCHEMAS[component];
  });
  console.log('[query] Configurable unsupported', unsupported);
}

async function logSceneStateEntities() {
  const reply = await queryStateTable();
  console.log('[query] State table', reply);
}

function bindWritableScalar(folder, entry) {
  const r = getConfigurableRef(entry.path, entry.value);
  const model = { value: toModelValue(entry, r.value) };
  if (entry.schema.kind === 'bool') {
    folder.addBinding(model, 'value', { label: entry.label }).on('change', async () => {
      r.value = fromModelValue(entry, model.value, r.value);
      await conn.set(entry.path, entry.component, r.value);
    });
    return;
  }
  folder.addBinding(model, 'value', {
    label: entry.label,
    step: entry.schema.step,
  }).on('change', async () => {
    r.value = fromModelValue(entry, model.value, r.value);
    model.value = toModelValue(entry, r.value);
    await conn.set(entry.path, entry.component, r.value);
  });
}

function bindWritableVector(folder, entry) {
  const r = getConfigurableRef(entry.path, entry.value);
  const model = { value: toModelValue(entry, r.value) };
  const options = { label: entry.label };
  for (const key of entry.schema.codec.keys) {
    options[key] = { step: entry.schema.step, min: entry.schema.min, max: entry.schema.max };
  }
  folder.addBinding(model, 'value', options).on('change', async () => {
    r.value = fromModelValue(entry, model.value, r.value);
    await conn.set(entry.path, entry.component, r.value);
  });
}

function bindWritableColor4(folder, entry) {
  const r = getConfigurableRef(entry.path, entry.value);
  const model = { value: toModelValue(entry, r.value) };
  folder.addBinding(model, 'value', {
    label: entry.label,
    color: { type: 'float', alpha: true },
  }).on('change', async () => {
    r.value = fromModelValue(entry, model.value, r.value);
    await conn.set(entry.path, entry.component, r.value);
  });
}

function bindConfigurableControl(folder, entry) {
  if (entry.schema.kind === 'bool' || entry.schema.kind === 'int' || entry.schema.kind === 'float') {
    bindWritableScalar(folder, entry);
    return;
  }
  if (entry.schema.kind === 'vec3') {
    bindWritableVector(folder, entry);
    return;
  }
  if (entry.schema.kind === 'vec4') {
    bindWritableVector(folder, entry);
    return;
  }
  if (entry.schema.kind === 'color4') {
    bindWritableColor4(folder, entry);
  }
}

function bindReadonlyScalar(folder, entry) {
  const model = getStateModel(entry.path, toModelValue(entry, entry.value));
  const options = {
    label: entry.label,
    readonly: true,
  };
  if (entry.schema.kind !== 'bool') options.step = entry.schema.step;
  folder.addBinding(model, 'value', options);
}

function bindReadonlyVector(folder, entry) {
  const model = getStateModel(entry.path, toModelValue(entry, entry.value));
  const options = {
    label: entry.label,
    readonly: true,
  };
  for (const key of entry.schema.codec.keys) {
    options[key] = { step: entry.schema.step, min: entry.schema.min, max: entry.schema.max };
  }
  folder.addBinding(model, 'value', options);
}

function bindStateControl(folder, entry) {
  if (entry.schema.kind === 'bool' || entry.schema.kind === 'int' || entry.schema.kind === 'float') {
    bindReadonlyScalar(folder, entry);
    return;
  }
  if (entry.schema.kind === 'vec3') {
    bindReadonlyVector(folder, entry);
    return;
  }
  if (entry.schema.kind === 'vec4') {
    bindReadonlyVector(folder, entry);
    return;
  }
  if (entry.schema.kind === 'color4') {
    bindReadonlyVector(folder, entry);
  }
}

function toEntry(item) {
  const path = toConfigurablePath(item);
  if (!path) return null;
  const [component, value] = firstComponentEntry(item?.components);
  if (!component || value === undefined || value === null) return null;
  const schema = CONFIG_COMPONENT_SCHEMAS[component];
  if (!schema) return null;
  const { ns, label } = splitPath(path);
  return { path, ns, label, component, value, schema };
}

function sortByPath(entries) {
  return [...entries].sort((a, b) => {
    if (a.ns === b.ns) return a.label.localeCompare(b.label);
    return a.ns.localeCompare(b.ns);
  });
}

async function buildConfigurablePane(rootFolder) {
  configurableRefs.clear();
  const reply = await queryConfigurableTable();
  const tableResults = reply?.results || [];
  const entries = sortByPath(
    tableResults
      .map((item) => toEntry(item))
      .filter(Boolean)
  );
  const folderMap = new Map();
  for (const entry of entries) {
    const folder = ensureFolder(rootFolder, folderMap, entry.ns);
    bindConfigurableControl(folder, entry);
  }
}

async function buildStatePane(rootFolder) {
  stateModels.clear();
  const reply = await queryStateTable();
  const tableResults = reply?.results || [];
  const entries = sortByPath(
    tableResults
      .map((item) => toEntry(item))
      .filter(Boolean)
  );
  const folderMap = new Map();
  for (const entry of entries) {
    const folder = ensureFolder(rootFolder, folderMap, entry.ns);
    bindStateControl(folder, entry);
  }
}

async function refreshStateValues() {
  const reply = await queryStateTable();
  const tableResults = reply?.results || [];
  for (const item of tableResults) {
    const entry = toEntry(item);
    if (!entry) continue;
    const model = stateModels.get(entry.path);
    if (!model) continue;
    model.value = toModelValue(entry, entry.value);
  }
  pane?.refresh();
}

function stopStatePolling() {
  if (!statePollTimer) return;
  clearInterval(statePollTimer);
  statePollTimer = null;
}

function startStatePolling() {
  stopStatePolling();
  statePollTimer = setInterval(() => {
    if (!conn) return;
    refreshStateValues().catch(console.error);
  }, 16);
}

async function initPane() {
  if (typeof window === 'undefined' || !Pane || !paneHost.value || !conn) return;
  pane?.dispose();
  pane = new Pane({ container: paneHost.value, title: 'Config Controls' });

  pane.addButton({ title: 'Reload Pane' }).on('click', async () => {
    await initPane();
  });
  pane.addButton({ title: 'Log Configurable Table' }).on('click', async () => {
    await logConfigurable();
  });
  pane.addButton({ title: 'Log Unsupported Types' }).on('click', async () => {
    await logUnsupportedConfigurable();
  });

  const autoFolder = pane.addFolder({ title: 'Configurable' });
  await buildConfigurablePane(autoFolder);

  const stateFolder = pane.addFolder({ title: 'State' });
  stateFolder.addButton({ title: 'Refresh State Values' }).on('click', async () => {
    await refreshStateValues();
  });
  stateFolder.addButton({ title: 'Log State Table' }).on('click', async () => {
    await logSceneStateEntities();
  });
  await buildStatePane(stateFolder);
}

async function onReady() {
  conn = window.flecs.connect(appRef.value);
  status.value = 'ready';
  await initPane();
  await refreshStateValues();
  startStatePolling();
}

function onError(message) {
  status.value = message;
  console.error(message);
  stopStatePolling();
}

onBeforeUnmount(() => {
  stopStatePolling();
  pane?.dispose();
  pane = null;
});
</script>

## Live Probe (Remote API -> `small/cloth`)

Status: `{{ status }}`

<Simpac
  ref="appRef"
  src="/bazel-bin/small/cloth/webapp/main.js"
  :debug="true"
  :cwrap="['flecs_explorer_request']"
  @ready="onReady"
  @error="onError"
/>

<div ref="paneHost"></div>

## Source Contract (`small/cloth`)

| Domain | Entity path | Value type | Configurable | Main writers | Main readers |
| --- | --- | --- | --- | --- | --- |
| `props::` | `Config.Scene.dt` | `Real` | yes | seed + remote set | scene time step, physics systems, render text |
| `props::` | `Config.Scene.gravity` | `vec3r` | yes | seed + remote set | external-force accumulation |
| `props::` | `Config.Scene.paused` | `bool` | yes | seed + remote set | `Scene.UpdateSimTime` gate |
| `props::` (implicit app) | `Config.Solver.cg_max_iter` | `int` | yes | `main_implicit.cpp` + remote set | implicit solver |
| `props::` (implicit app) | `Config.Solver.cg_tolerance` | `Real` | yes | `main_implicit.cpp` + remote set | implicit solver |
| `state::` | `Scene.wall_time` | `Real` | no | `Scene.UpdateWallTime` | render text |
| `state::` | `Scene.sim_time` | `Real` | no | `Scene.UpdateSimTime` | render text |
| `state::` | `Scene.frame_count` | `int` | no | `Scene.UpdateSimTime` | render text + implicit stats |
| `state::` | `Scene.dirty` | `bool` | no | spawn/edit flows | reindex/solver prep/clear |

## Coupling Rules

1. User-facing knobs go under `Config.*` and attach `Configurable`.
2. Frame/runtime bookkeeping goes under `Scene.*` and is written by systems.
3. Bootstrap flow: register components -> `props::seed`/`state::seed` -> install systems.
4. Remote write path is entity-by-entity (`conn.set`) and immediately reflected by next frame.

## Raw Patterns

```js
const conn = window.flecs.connect(appRef.value);

// discover user-facing knobs
await conn.query('Configurable', { full_paths: true, table: true, values: true });

// read one value (component name discovered at runtime)
const entityReply = await conn.entity('Config.Scene.dt', { values: true });
const [component, value] = Object.entries(entityReply?.components || {})[0] || [];

// write one value
await conn.set('Config.Scene.dt', component, 1 / 120);

// state readback
await conn.entity('Scene.frame_count', { values: true });
```
