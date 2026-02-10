---
title: Scripting
---

# Scripting

Drive scene config via Flecs script in a worker-hosted WASM app.

<script setup>
import { computed, ref } from 'vue';

const app_ref = ref(null);
const script_text = ref('');
const script_name = 'SceneScript';
const systems_parent = ref('sim');
const systems = ref([]);
const systems_query = computed(() => {
  if (systems_parent.value) {
    return `(ChildOf, ${systems_parent.value}), flecs.system.System, ?Disabled`;
  }
  return 'flecs.system.System, !(ChildOf, *), ?Disabled';
});
const disabled_tag = 'flecs.core.Disabled';
let conn = null;

function onReady() {
  conn = window.flecs.connect(app_ref.value);

  // print the entire world (serialized)
  conn.world((world) => { console.log(world) });

  // get entity with full table info,
  // conn.entity("SceneScript", { table: true })
  // or get component right away
  conn.get(script_name, { component: "flecs.script.Script" }, (resp) => {
    console.log(resp)
    script_text.value = resp?.code || '';
  }, (e) => {
    console.error(e);
  });

  load_systems();
}

async function apply_script() {
  const reply = await conn.scriptUpdate(
    script_name,
    script_text.value,
    { try: true, check_file: true, save_file: false }
  );
  console.log(reply);
}

async function world_query() {
  const reply = await conn.world();
  console.log(reply);
}

async function load_systems() {
  const reply = await conn.query(systems_query.value, {
    full_paths: true,
    fields: true,
    values: false
  });
  systems.value = reply.results.map((item) => ({
    parent: item.parent,
    name: item.name,
    path: item.parent ? `${item.parent}.${item.name}` : item.name,
    enabled: item.fields?.is_set?.[2] === false
  }));
}

async function toggle_system(entry) {
  const enabled = !entry.enabled;
  const path = entry.path.replace(/ /g, '%20');
  const reply = enabled
    ? await conn.remove(path, disabled_tag)
    : await conn.add(path, disabled_tag);
  console.log(reply);
  entry.enabled = enabled;
}

async function query_user_made_systems() {
  const reply = await conn.query('flecs.system.System, !ChildOf($this|up, flecs), ?Disabled', {
    full_paths: true
  });
  console.log(reply);
}
</script>

<Simpac
  ref="app_ref"
  src="/bazel-bin/small/cloth/webapp/main.js"
  :debug="true"
  :cwrap="['flecs_explorer_request']"
  @ready="onReady"
  @error="console.error($event)"
/>

<textarea v-model="script_text" rows="14" cols="80" @input="apply_script"></textarea>
<div>
  <button @click="apply_script">Apply Script</button>
  <button @click="world_query">Get World</button>
  <button @click="load_systems">Refresh systems</button>
  <button @click="query_user_made_systems">Query user systems</button>
</div>

<div>
  <label>
    Parent
    <input v-model="systems_parent" />
  </label>
  <strong>Systems under {{ systems_parent }}</strong>
  <div v-for="sys in systems" :key="sys.path">
    <label>
      <input type="checkbox" :checked="sys.enabled" @change="toggle_system(sys)" />
      {{ sys.path }}
    </label>
  </div>
</div>

## Startup scene

- WASM default: `scenes/default.flecs` (embedded in the build).
- Native default: `small/cloth/scenes/default.flecs` via `graphics::npath`.

## Runtime update

```js
const conn = window.flecs.connect(app_ref.value);

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
