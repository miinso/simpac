---
title: Solar System
---

# Solar System

<script setup>
import { ref } from 'vue';

const appRef = ref(null);
const fovY = ref(60);
let conn = null;

async function onReady() {
  conn = window.flecs.connect(appRef.value);

  // print world as a whole
  conn.world((world) => { console.log(world)})

  const reply = await conn.get('MainCamera', { component: 'graphics.Camera', values: true });
  const value = reply?.value || reply?.Camera || reply;
  if (typeof value?.fovy === 'number') fovY.value = value.fovy;
}

function onInput() {
  if (!conn) return;
  conn.set('MainCamera', 'graphics.Camera', { fovy: fovY.value }).catch(console.error);
}
</script>

<Simpac
  ref="appRef"
  src="/bazel-bin/small/explorer/explorer_demo.js"
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
