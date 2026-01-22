---
title: scripting
---

# Scripting (Flecs)

This page shows how to drive scene config via Flecs script in a worker-hosted WASM app.

<script setup>
import { ref, watch } from 'vue';

const appRef = ref(null);
const status = ref('booting');
let conn = null;

const appSrc = ref('/bazel-bin/small/implicit-euler/webapp/main.js');
const scriptName = ref('SceneScript');
const scriptText = ref('');
const lastError = ref('');

function onReady() {
  status.value = 'ready';
  conn = window.flecs.connect(appRef.value);

  // print the entire world (serialized)
  conn.world((world) => { console.log(world) });

  // get entity with full table info,
  // conn.entity("SceneScript", { table: true })
  // or get component right away
  conn.get(scriptName.value, { component: "flecs.script.Script" }, (resp) => {
    console.log(resp)
    scriptText.value = resp?.code || '';
    lastError.value = resp?.error || '';
  }, (e) => {
    console.error(e);
  });
}

function onError(message) {
  status.value = `error: ${message}`;
}

async function applyScript() {
  if (!conn) return;
  try {
    lastError.value = '';

    const reply = await conn.scriptUpdate(scriptName.value, scriptText.value, 
    { try: true, check_file: true, save_file: false });

    console.log(reply);
    
    const error =
      reply?.error ||
      (Array.isArray(reply?.errors) ? reply.errors.join('\n') : '');
    
    lastError.value = error || '';
  } catch (err) {
    lastError.value = err instanceof Error ? err.message : String(err);
  }
}

function queryWorld() {
    conn.world((world) => { console.log(world) });
}

watch(scriptText, () => {
  if (!conn) return;
  applyScript();
});
</script>

Status: **{{ status }}**

<Simpac
  ref="appRef"
  :src="appSrc"
  :debug="true"
  :cwrap="['flecs_explorer_request']"
  @ready="onReady"
  @error="onError"
/>

<textarea v-model="scriptText" rows="14" cols="80"></textarea>
<div>
  <button @click="applyScript">Apply Script Now</button>
  <button @click="queryWorld">Query World</button>
</div>
<div v-if="lastError">
  Script error: {{ lastError }}
</div>

## Startup scene file

- WASM default is `scenes/default.flecs` (embedded in the build).
- Native default is `small/implicit-euler/scenes/default.flecs` via `graphics::npath`.

## Runtime update via REST

```js
const conn = window.flecs.connect(appRef.value);

const scene = `
Cloth {
  GridCloth: {
    width: 20,
    height: 20,
    spacing: 0.8,
    mass: 1.0,
    k_structural: 8000.0,
    k_shear: 8000.0,
    k_bending: 8.0,
    k_damping: 0.03,
    pin_mode: 0
  }
}
`;

// evaluate script in the running world
await conn.scriptUpdate('SceneScript', scene);
```

If you only want to tweak values, you can patch the component directly:

```js
await conn.set('Cloth', 'GridCloth', { width: 30, height: 30 });
```

## Notes

- `scriptUpdate` ships source code over REST and evaluates immediately.
- `main.cpp` creates a managed script entity named `SceneScript`.
- `scriptUpdate` targets a script entity name, not a file path.
