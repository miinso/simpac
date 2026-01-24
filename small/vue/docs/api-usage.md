---
title: Usage
---

<script setup>
import { ref } from 'vue';

const appRef = ref(null);
const fovY = ref(60);
const status = ref('booting');
let conn = null;

const appSrc = ref('/bazel-bin/small/implicit-euler/webapp/main.js');
const wasmUrl = ref('');

function onReady() {
  status.value = 'ready';
  conn = window.flecs.connect(appRef.value);
  conn.get('MainCamera', { component: 'graphics.Camera', values: true }).then((reply) => {
    const value = reply?.value || reply?.Camera || reply;
    if (value && typeof value.fovy === 'number') fovY.value = value.fovy;
  });
}

function onError(message) {
  status.value = `error: ${message}`;
}

function onInput() {
  if (!appRef.value || !conn) return;
  conn.set('MainCamera', 'graphics.Camera', { fovy: fovY.value }).catch(console.error);
}
</script>

# Usage

Drop your WASM build into `docs/.vitepress/public/worker/` as `main.js` (and `main.wasm` if split).
The Flecs client is globally injected as `window.flecs`.

Status: **{{ status }}**

<Simpac
  ref="appRef"
  :src="appSrc"
  :wasm-url="wasmUrl"
  :debug="true"
  :cwrap="['flecs_explorer_request']"
  @ready="onReady"
  @error="onError"
/>

<label>
  Camera fovy: {{ fovY.toFixed(2) }}
  <br>
  <input type="range" min="0" max="120" step="0.1" v-model.number="fovY" @input="onInput" />
</label>

Camera position lives in `graphics.Position` and serializes as `{ x, y, z }`:

```js
conn.set('MainCamera', 'graphics.Position', { x: 5, y: 5, z: 5 });
```

## Remote API surface

```js:line-numbers
// global helper
const { flecs } = window;
const conn = flecs.connect(appRef.value); // or flecs.connect('localhost')
flecs.escapePath('Parent.Child');
flecs.trimQuery('Position, Velocity');

// core requests
conn.request('/world', {}, (reply) => {});
conn.entity('MainCamera', { values: true }, (reply) => {});
conn.query('Position,Velocity', { table: true }, (reply) => {});
conn.queryName('MyQuery', { table: true }, (reply) => {});
conn.get('MainCamera', { component: 'graphics.Camera' }, (reply) => {});
conn.set('MainCamera', 'graphics.Camera', { fovy: 75 });
conn.set('MainCamera', 'graphics.Position', { x: 5, y: 5, z: 5 });
conn.add('MainCamera', 'graphics.ActiveCamera');
conn.remove('MainCamera', 'graphics.ActiveCamera');
conn.enable('MainCamera', 'graphics.ActiveCamera');
conn.disable('MainCamera', 'graphics.ActiveCamera');
conn.create('NewEntity');
conn.delete('NewEntity');
conn.world((reply) => {});
conn.scriptUpdate('main', 'Tag {}', {}, (reply) => {});
conn.action('reset', (reply) => {});
```

| Line | Description |
| --- | --- |
| 2-3 | Grab the global client and connect to the worker-hosted app. |
| 4-5 | Utility helpers for path/query normalization. |
| 8 | Raw REST request shortcut. |
| 9 | Fetch entity info. |
| 10 | Query entities by expression. |
| 11 | Query entities by named query. |
| 12 | Read a component (optionally with params). |
| 13 | Set/patch a component value. |
| 14-15 | Add/remove a component tag. |
| 16-17 | Enable/disable a component. |
| 18-19 | Create/delete entities. |
| 20 | Fetch world stats. |
| 21 | Update script content. |
| 22 | Invoke a server action. |
