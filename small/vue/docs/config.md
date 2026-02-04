---
title: Config
---

# Config

Auto-bind configurable fields and runtime properties.

<script setup>
import { defineComponent, h, nextTick, ref, watch } from 'vue';
import { useStore } from '@vue/repl';

const app_ref = ref(null);
const config_tree = ref([]);
const component_groups = ref([]);
const runtime_groups = ref([]);
let conn = null;

// roots to query under
const component_roots = ['Scene', 'Thing1'];
const runtime_roots = ['Runtime.Scene', 'Runtime.Thing1'];
const script_name = 'SceneScript';
const vec_axes = ['x', 'y', 'z'];

const store = useStore();
const initial_value = '// Loading script...';

// vec3 detector for field rendering
function is_vec3(value) {
  return value?.x !== undefined && value?.y !== undefined && value?.z !== undefined;
}

// tiny input renderer to avoid template bloat
const FieldInput = defineComponent({
  props: {
    entry: { type: Object, required: true },
    onUpdate: { type: Function, required: true },
  },
  setup(props) {
    return () => {
      const entry = props.entry;
      if (typeof entry.value === 'boolean') {
        return h('input', {
          class: 'input-checkbox',
          type: 'checkbox',
          checked: entry.value,
          onChange: (event) => {
            entry.value = event.target.checked;
            props.onUpdate(entry);
          },
        });
      }
      if (typeof entry.value === 'number') {
        return h('input', {
          class: 'input-number input-number-narrow',
          type: 'number',
          value: entry.value,
          onChange: (event) => {
            entry.value = Number(event.target.value);
            props.onUpdate(entry);
          },
        });
      }
      if (is_vec3(entry.value)) {
        return h(
          'div',
          { class: 'vec-row' },
          vec_axes.map((axis) =>
            h('input', {
              class: 'input-number input-number-vec',
              type: 'number',
              value: entry.value[axis],
              onChange: (event) => {
                entry.value[axis] = Number(event.target.value);
                props.onUpdate(entry);
              },
            })
          )
        );
      }
      return null;
    };
  },
});

// auto-apply script when repl changes
watch(
  () => store.activeFile?.code,
  (code) => {
    if (code == null) return;
    if (!conn) return;
    apply_script(code);
  }
);

// connect + refresh panels
// connect + refresh panels
function on_ready() {
  conn = window.flecs.connect(app_ref.value);
  load_script();
  load_config_tree();
  load_component_groups();
  load_runtime_groups();
}

// load script text into repl
async function load_script() {
  conn.get(script_name, { component: 'flecs.script.Script' }, async (resp) => {
    store.activeFile.code = resp?.code || '';
    await nextTick();
  }, (e) => {
    console.error(e);
  });
}

// push repl text into flecs script
async function apply_script(code = store.activeFile?.code ?? '') {
  const reply = await conn.scriptUpdate(
    script_name,
    code,
    { try: true, check_file: true, save_file: false }
  );
  console.log(reply);
  load_component_groups();
  load_runtime_groups();
}

// pull Configurable tree listing
async function load_config_tree() {
  const reply = await conn.query('Configurable', { full_paths: true });
  console.log(reply);
  const groups = {};
  for (const item of reply.results ?? []) {
    const parent = item.parent || '(root)';
    (groups[parent] ||= []).push(item);
  }
  config_tree.value = Object.entries(groups).map(([parent, items]) => ({
    parent,
    items
  }));
}

// pull component-backed groups + values
async function load_component_groups() {
  const groups = await Promise.all(component_roots.map(async (root) => {
    const members = await conn.query(`Configurable, (ChildOf, ${root})`, { full_paths: true });
    const reply = await conn.get(root, { component: root, values: true });
    const values = reply?.value || reply?.[root] || reply;
    return {
      root,
      fields: (members.results ?? []).map((item) => ({
        root,
        name: item.name,
        path: `${item.parent}.${item.name}`,
        value: values?.[item.name]
      }))
    };
  }));
  component_groups.value = groups;
}

// write component-backed field
async function update_component_field(entry) {
  const reply = await conn.set(entry.root, entry.root, { [entry.name]: entry.value });
  console.log(reply);
}

// pull runtime prop groups + values
async function load_runtime_groups() {
  const lists = await Promise.all(runtime_roots.map((root) =>
    conn.query(`Configurable, (ChildOf, ${root})`, { full_paths: true })
  ));
  console.log(lists);

  runtime_groups.value = await Promise.all(runtime_roots.map(async (root, index) => {
    const items = lists[index].results ?? [];
    const props = await Promise.all(items.map(async (item) => {
      const path = item.parent ? `${item.parent}.${item.name}` : item.name;
      const reply = await conn.entity(path, { values: true });
      const [component, value] = Object.entries(reply?.components ?? {})[0] || [];
      return {
        root,
        name: item.name,
        path,
        component,
        value
      };
    }));
    return { root, props };
  }));
}

// write runtime prop value
async function update_runtime_prop(entry) {
  const path = entry.path.replace(/ /g, '%20');
  const reply = await conn.set(path, entry.component, entry.value);
  console.log(reply);
}
</script>

<Repl class="h-40 rounded-md"
  :store="store"
  :initialValue="initial_value"
  filename="config.flecs"
  :autoResize="false"
/>

<Simpac
  ref="app_ref"
  src="/bazel-bin/small/config/webapp/main.js"
  :debug="true"
  :cwrap="['flecs_explorer_request']"
  @ready="on_ready"
  @error="console.error($event)"
/>

<div class="mt-6 flex flex-wrap gap-3">
  <button class="btn" @click="apply_script">Apply script</button>
  <button class="btn" @click="load_config_tree">Reload config tree</button>
  <button class="btn" @click="load_component_groups">Reload component fields</button>
  <button class="btn" @click="load_runtime_groups">Reload runtime props</button>
</div>

<div class="mt-8 flex flex-col gap-6 lg:flex-row lg:items-start">
  <section class="panel">
    <strong class="panel-title">Configurable tree</strong>
    <div class="space-y-2">
      <div v-for="group in config_tree" :key="group.parent" class="panel-tree">
        <div class="panel-group-title">{{ group.parent }}</div>
        <div class="tree-items">
          <div v-for="item in group.items" :key="`${group.parent}.${item.name}`" class="pl-2">
            - {{ group.parent }}.{{ item.name }}
          </div>
        </div>
      </div>
    </div>
  </section>

  <section class="panel">
    <strong class="panel-title">Component fields</strong>
    <div class="space-y-4">
      <div v-for="group in component_groups" :key="group.root" class="panel-group">
        <div class="panel-group-title">{{ group.root }}</div>
        <div class="space-y-3">
          <div v-for="field in group.fields" :key="field.path" class="space-y-1">
            <label class="field-label">{{ field.path }}</label>
            <div class="field-row">
              <FieldInput :entry="field" :onUpdate="update_component_field" />
            </div>
          </div>
        </div>
      </div>
    </div>
  </section>

  <section class="panel">
    <strong class="panel-title">Runtime properties</strong>
    <div class="space-y-4">
      <div v-for="group in runtime_groups" :key="group.root" class="panel-group">
        <div class="panel-group-title">{{ group.root }}</div>
        <div class="space-y-3">
          <div v-for="prop in group.props" :key="prop.path" class="space-y-1">
            <label class="field-label">{{ prop.path }}</label>
            <div class="field-row">
              <FieldInput :entry="prop" :onUpdate="update_runtime_prop" />
            </div>
          </div>
        </div>
      </div>
    </div>
  </section>
</div>

<style scoped>
.btn {
  @apply px-3 py-2 text-sm font-medium text-white bg-slate-800 rounded shadow hover:bg-slate-700 transition;
}
.panel {
  @apply flex-1 min-w-[260px] lg:basis-1/3 space-y-3 rounded-xl border border-slate-200/80 bg-white/70 p-4 shadow-sm dark:border-slate-700/80 dark:bg-slate-900/60;
}
.panel-title {
  @apply text-base font-semibold text-slate-900 dark:text-slate-100;
}
.panel-tree {
  @apply rounded-md bg-slate-50/60 px-3 py-2 dark:bg-slate-800/60;
}
.panel-group {
  @apply space-y-3 rounded-lg border border-slate-100/80 bg-slate-50/50 p-3 dark:border-slate-700/50 dark:bg-slate-900/40;
}
.panel-group-title {
  @apply text-sm font-semibold text-slate-700 dark:text-slate-200;
}
.tree-items {
  @apply mt-1 space-y-1 text-sm text-slate-600 dark:text-slate-300;
}
.field-label {
  @apply block text-sm font-medium text-slate-800 dark:text-slate-200;
}
.field-row {
  @apply flex flex-wrap items-center gap-2 w-full;
}
:deep(.input-checkbox) {
  @apply h-5 w-5 rounded border border-slate-300 bg-white ring-1 ring-slate-200 focus:outline-none focus:ring-2 focus:ring-indigo-500 focus:ring-offset-1 dark:ring-slate-700;
}
:deep(.input-number) {
  @apply rounded border border-slate-300 bg-white px-2 py-1 text-sm text-slate-900 ring-1 ring-slate-200 focus:border-indigo-500 focus:outline-none focus:ring-2 focus:ring-indigo-500 focus:ring-offset-1 dark:ring-slate-700;
}
:deep(.input-number-narrow) {
  @apply w-28;
}
:deep(.input-number-vec) {
  @apply w-full min-w-0;
}
.vec-row {
  @apply grid w-full grid-cols-3 gap-2;
}
</style>
