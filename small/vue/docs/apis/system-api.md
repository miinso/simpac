---
title: System API
---

# System API

Load systems from the running world, toggle each system, and monitor runtime in a separate stats tab.

<script setup>
import { onBeforeUnmount, ref } from 'vue';
import { Pane } from 'tweakpane';

const appRef = ref(null);
const paneHost = ref(null);
const status = ref('booting');
const systemCount = ref(0);

let conn = null;
let pane = null;
let statsTimer = null;

const disabledTag = 'flecs.core.Disabled';
const systemsQuery = 'flecs.system.System, !ChildOf($this|up, flecs), ?Disabled';
const pipelinesQuery = '[none] flecs.pipeline.Pipeline';
const statsPeriod = '1s';
const statsPollMs = 100;
const systemModels = new Map();
let statsPipelineName = 'all';

function splitPath(path) {
  const p = String(path || '');
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

function formatSystemTime(seconds) {
  if (!Number.isFinite(seconds) || seconds <= 0) return '0us';
  if (seconds >= 1e-3) return `${(seconds * 1000).toFixed(2)}ms`;
  if (seconds >= 1e-6) return `${(seconds * 1000000).toFixed(1)}us`;
  return `${(seconds * 1000000000).toFixed(1)}ns`;
}

function parseSystemTimes(statsReply) {
  const times = new Map();
  const leafTimes = new Map();
  if (!Array.isArray(statsReply)) return { times, leafTimes };
  for (const item of statsReply) {
    const path = item?.name;
    const samples = item?.time_spent?.avg;
    if (!path || !Array.isArray(samples) || samples.length === 0) continue;
    let sum = 0;
    let nzSum = 0;
    let nzCount = 0;
    for (const value of samples) {
      const v = Number(value) || 0;
      sum += v;
      if (v > 0) {
        nzSum += v;
        nzCount += 1;
      }
    }
    const avgAll = sum / samples.length;
    const avgNonZero = nzCount > 0 ? nzSum / nzCount : 0;
    const display = avgNonZero > 0 ? avgNonZero : avgAll;
    times.set(path, display);
    const leaf = String(path).split('.').pop();
    if (!leafTimes.has(leaf)) leafTimes.set(leaf, []);
    leafTimes.get(leaf).push(display);
  }
  return { times, leafTimes };
}

function timeFromParsed(parsed, path) {
  const direct = parsed.times.get(path);
  if (direct !== undefined) return direct;
  const leaf = String(path).split('.').pop();
  const list = parsed.leafTimes.get(leaf);
  if (list?.length === 1) return list[0];
  return 0;
}

async function queryPipelineStats() {
  let stats = await conn.request('stats/pipeline', {
    name: statsPipelineName,
    period: statsPeriod,
  });
  if (stats?.error && statsPipelineName !== 'all') {
    statsPipelineName = 'all';
    stats = await conn.request('stats/pipeline', {
      name: statsPipelineName,
      period: statsPeriod,
    });
  }
  if (stats?.error) {
    status.value = String(stats.error);
    return null;
  }
  if (!Array.isArray(stats)) {
    status.value = 'stats/pipeline returned non-array';
    return null;
  }
  return stats;
}

async function resolveStatsPipelineName() {
  if (!conn) return;
  const reply = await conn.query(pipelinesQuery, {
    full_paths: true,
    values: false,
  });
  const names = (reply?.results || [])
    .map((item) => (item.parent ? `${item.parent}.${item.name}` : item.name))
    .filter(Boolean);
  const nonFlecs = names.find((name) => !name.startsWith('flecs.'));
  statsPipelineName = nonFlecs || names[0] || 'all';
}

async function refreshSystemTimes() {
  if (!conn || systemModels.size === 0) return;
  const stats = await queryPipelineStats();
  if (!stats) return;
  const parsed = parseSystemTimes(stats);
  for (const [path, model] of systemModels) {
    model.time = formatSystemTime(timeFromParsed(parsed, path));
  }
}

function startStatsPolling() {
  if (statsTimer) clearInterval(statsTimer);
  statsTimer = setInterval(() => {
    refreshSystemTimes().catch((error) => {
      status.value = String(error?.message || error);
    });
  }, statsPollMs);
}

function stopStatsPolling() {
  if (!statsTimer) return;
  clearInterval(statsTimer);
  statsTimer = null;
}

async function querySystems() {
  const reply = await conn.query(systemsQuery, {
    full_paths: true,
    fields: true,
    values: false,
  });
  const rows = (reply?.results || [])
    .map((item) => ({
      parent: item.parent || '',
      name: item.name || '',
      path: item.parent ? `${item.parent}.${item.name}` : item.name,
      disabled: item?.fields?.is_set?.[2] === true,
    }))
    .map((item) => ({
      ...item,
      enabled: !item.disabled,
    }))
    .sort((a, b) => a.path.localeCompare(b.path));
  systemCount.value = rows.length;
  return rows;
}

async function logSystemsQuery() {
  if (!conn) return;
  const reply = await conn.query(systemsQuery, {
    full_paths: true,
    fields: true,
    values: false,
    table: true,
  });
  console.log('[query] systems', reply);
}

async function logPipelineStats() {
  if (!conn) return;
  const stats = await queryPipelineStats();
  console.log('[stats] pipeline target', statsPipelineName);
  console.log('[stats] pipeline count', Array.isArray(stats) ? stats.length : 0);
  if (!Array.isArray(stats)) return;
  console.log('[stats] sample names', stats.slice(0, 20).map((x) => x?.name));
}

async function setSystemEnabled(path, enabled) {
  if (!conn || !path) return;
  const escaped = path.replace(/ /g, '%20');
  if (enabled) {
    await conn.remove(escaped, disabledTag);
  } else {
    await conn.add(escaped, disabledTag);
  }
}

async function loadPane() {
  if (!conn || !paneHost.value || typeof window === 'undefined' || !Pane) return;

  const systems = await querySystems();
  systemModels.clear();
  pane?.dispose();
  pane = new Pane({
    container: paneHost.value,
    title: 'Systems',
  });
  const tabs = pane.addTab({
    pages: [
      { title: 'Toggles' },
      { title: 'Stats' },
    ],
  });

  pane.addButton({ title: 'Reload Systems' }).on('click', async () => {
    await loadPane();
  });
  pane.addButton({ title: 'Log Systems Query' }).on('click', async () => {
    await logSystemsQuery();
  });
  pane.addButton({ title: 'Log Pipeline Stats' }).on('click', async () => {
    await logPipelineStats();
  });

  const toggleFolders = new Map();
  const statsFolders = new Map();
  for (const system of systems) {
    const { ns, label } = splitPath(system.path);
    const model = { enabled: system.enabled, time: '0us' };
    systemModels.set(system.path, model);
    const toggleFolder = ensureFolder(tabs.pages[0], toggleFolders, ns);
    const binding = toggleFolder.addBinding(model, 'enabled', { label });
    binding.on('change', async () => {
      await setSystemEnabled(system.path, Boolean(model.enabled));
    });
    const statsFolder = ensureFolder(tabs.pages[1], statsFolders, ns);
    statsFolder.addBinding(model, 'time', {
      label,
      readonly: true,
      interval: statsPollMs,
    });
  }
  await refreshSystemTimes();
  startStatsPolling();
}

async function onReady() {
  conn = window.flecs.connect(appRef.value);
  await resolveStatsPipelineName();
  status.value = 'ready';
  await loadPane();
}

function onError(message) {
  status.value = message;
  console.error(message);
}

onBeforeUnmount(() => {
  stopStatsPolling();
  pane?.dispose();
  pane = null;
});
</script>

Status: `{{ status }}`

Systems loaded: `{{ systemCount }}`

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

| Group | Install path | What gets registered |
| --- | --- | --- |
| Scene core systems | `small/cloth/systems.h` | `Scene.UpdateWallTime`, `Scene.UpdateSimTime`, `Scene.ReindexParticles`, `Scene.ClearDirty` |
| Render systems | `small/cloth/systems/render/module.h` | draw/upload systems and interaction systems such as `graphics.PickParticles`, `graphics.DragParticlesKinematic`, `graphics.DragParticlesSpring` |
| Graphics pipeline systems | `modules/graphics/systems.h` | `graphics.begin`, `graphics.render3d`, `graphics.render2d`, `graphics.end`, camera sync/update |
| Algorithm pipelines | `small/cloth/main_explicit.cpp`, `small/cloth/main_implicit.cpp` | parent systems (`Explicit Euler`, `Implicit Euler`) and scoped sub-systems |

## Query Contract

The page loads systems with:

```js
const systemsQuery = 'flecs.system.System, !ChildOf($this|up, flecs), ?Disabled';
```

This means:

1. include entities with `flecs.system.System`
2. exclude Flecs built-in hierarchy (`!ChildOf(..., flecs)`)
3. include optional disabled state (`?Disabled`)

Enabled state is computed from query fields:

```js
const disabled = item?.fields?.is_set?.[2] === true;
const enabled = !disabled;
```

## Toggle Semantics

System toggling is explicit tag add/remove on `flecs.core.Disabled`:

1. enable: `conn.remove(path, 'flecs.core.Disabled')`
2. disable: `conn.add(path, 'flecs.core.Disabled')`

This keeps behavior stable even when systems are not grouped under a custom parent.

## Raw Patterns

```js
const conn = window.flecs.connect(appRef.value);

const reply = await conn.query(
  'flecs.system.System, !ChildOf($this|up, flecs), ?Disabled',
  { full_paths: true, fields: true, values: false, table: true }
);

for (const item of reply?.results || []) {
  const path = item.parent ? `${item.parent}.${item.name}` : item.name;
  const enabled = item?.fields?.is_set?.[2] === false;
  await (enabled ? conn.add(path, 'flecs.core.Disabled')
                 : conn.remove(path, 'flecs.core.Disabled'));
}
```
