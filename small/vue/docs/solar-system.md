---
title: explorer
---

<script setup>
import { ref } from 'vue';

const appRef = ref(null);
const fovY = ref(60);
const status = ref('booting');
let conn = null;

const appSrc = ref('/bazel-bin/small/explorer/explorer_demo.js');

function onReady() {
  status.value = 'ready';
  conn = window.flecs.connect(appRef.value);

  // print world as a whole
  conn.world((world) => { console.log(world)})

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

<Simpac
  ref="appRef"
  :src="appSrc"
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
