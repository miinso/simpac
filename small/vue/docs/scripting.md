---
title: Scripting
---

# Scripting

Drive scene config via Flecs script in a worker-hosted WASM app.

<script setup>
import { ref } from 'vue';

const appRef = ref(null);
const scriptText = ref('');
const scriptName = 'SceneScript';
let conn = null;

function onReady() {
  conn = window.flecs.connect(appRef.value);

  // print the entire world (serialized)
  conn.world((world) => { console.log(world) });

  // get entity with full table info,
  // conn.entity("SceneScript", { table: true })
  // or get component right away
  conn.get(scriptName, { component: "flecs.script.Script" }, (resp) => {
    console.log(resp)
    scriptText.value = resp?.code || '';
  }, (e) => {
    console.error(e);
  });
}

async function applyScript() {
  if (!conn) return;
  const reply = await conn.scriptUpdate(
    scriptName,
    scriptText.value,
    { try: true, check_file: true, save_file: false }
  );
  console.log(reply);
}

async function worldQuery() {
  if (!conn) return;
  const reply = await conn.world();
  console.log(reply);
}
</script>

<Simpac
  ref="appRef"
  src="/bazel-bin/small/implicit-euler/webapp/main.js"
  :debug="true"
  :cwrap="['flecs_explorer_request']"
  @ready="onReady"
  @error="console.error($event)"
/>

<textarea v-model="scriptText" rows="14" cols="80" @input="applyScript"></textarea>
<div>
  <button @click="applyScript">Apply Script</button>
  <button @click="worldQuery">Get World</button>
</div>

## Startup scene

- WASM default: `scenes/default.flecs` (embedded in the build).
- Native default: `small/implicit-euler/scenes/default.flecs` via `graphics::npath`.

## Runtime update

```js
const conn = window.flecs.connect(appRef.value);

const scene = `
Cloth {
  Position: { x: -10, y: 10, z: 0 }
  GridCloth: {
    width: 20,
    height: 20,
    spacing: 0.8
  }
}
`;

// evaluate script in the running world
await conn.scriptUpdate('SceneScript', scene);
```

If you only want to tweak values, you can patch the component directly:

```js
await conn.set('Cloth', 'GridCloth', { width: 30, height: 30 });
await conn.set('Cloth', 'Position', { x: -10, y: 10, z: 0 });
```

## Notes

- `scriptUpdate` targets a script entity name, not a file path.
- For scalar components in scripts (e.g., `Mass`), use a positional initializer like `Mass: { 1.0 }`.
