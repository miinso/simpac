---
title: Usage
---

<script setup>
import { ref } from 'vue';

const appRef = ref(null);
const fovY = ref(60);
let conn = null;

function onReady() {
  conn = window.flecs.connect(appRef.value);
  conn.get('MainCamera', { component: 'graphics.Camera', values: true }).then((reply) => {
    const value = reply?.value || reply?.Camera || reply;
    if (value && typeof value.fovy === 'number') fovY.value = value.fovy;
  });
}

function onInput() {
  if (!conn) return;
  conn.set('MainCamera', 'graphics.Camera', { fovy: fovY.value }).catch(console.error);
}
</script>

# Usage

Drop your WASM build into `docs/.vitepress/public/worker/` as `main.js` (and `main.wasm` if split).
The Flecs client is globally injected as `window.flecs`.

<Simpac
  ref="appRef"
  src="/bazel-bin/small/cloth/webapp/main.js"
  :debug="true"
  :cwrap="['flecs_explorer_request']"
  @ready="onReady"
  @error="console.error($event)"
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

```js
// global helper
const { flecs } = window;
const conn = flecs.connect(appRef.value); // or flecs.connect('localhost')

flecs.escapePath('Parent.Child');
flecs.trimQuery('Position, Velocity');

// core requests
await conn.query('Position, Velocity', { values: true });
await conn.get('MainCamera', 'graphics.Camera');
await conn.set('MainCamera', 'graphics.Camera', { fovy: 75 });
await conn.scriptUpdate('main', 'Tag {}');
```
