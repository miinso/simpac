---
title: Blocks MK1
outline: false
---

# Blocks MK1

Single-page experiment with `Simpac + Repl + Remote API + interactive math`.

<script setup>
import { getCurrentInstance, nextTick, onBeforeUnmount, onMounted, ref, watch } from 'vue';
import { Repl as ReplEditor, useStore } from '@vue/repl';

const { createTerms, bindInteractiveMath } = getCurrentInstance().appContext.config.globalProperties.$imath;

const appRef = ref(null);
const status = ref('booting');
const cameraFovy = ref(60);
const cameraPos = ref({ x: 0, y: 5, z: 10 });

const scriptName = 'SceneScript';
const scriptText = ref('// Loading script...');
let conn = null;

const store = useStore();
const initialValue = '// Loading script...';

watch(
  () => store.activeFile?.code,
  (code) => {
    if (code == null) return;
    scriptText.value = code;
  }
);

const termDefs = {
  drag_kinematic: {
    kind: 'toggle',
    default: true,
    classes: ['text-sky-500'],
    bind: { system: 'graphics.DragParticlesKinematic' },
  },
  drag_spring: {
    kind: 'toggle',
    default: false,
    classes: ['text-emerald-500'],
    bind: { system: 'graphics.DragParticlesSpring' },
  },
};

const terms = createTerms(termDefs);

async function setSystemEnabled(systemName, enabled) {
  if (!conn || !systemName) return;
  const request = enabled ? conn.enable : conn.disable;
  try {
    await request(systemName);
  } catch (error) {
    console.warn(error);
  }
}

async function refreshSystems() {
  if (!conn) return;
  const reply = await conn.query('flecs.system.System, !ChildOf($this|up, flecs), ?Disabled', {
    full_paths: true,
    values: true,
    // limit: 12,
  });
  console.log('[query] systems', reply);
}

async function inspectEntity(path = scriptName) {
  if (!conn || !path) return;
  const entity = await conn.entity(path, { values: true, full_paths: true });
  console.log('[entity]', path, entity);
}

async function loadScript() {
  if (!conn) return;
  try {
    const reply = await conn.get(scriptName, { component: 'flecs.script.Script' });
    const code = reply?.code || '';
    scriptText.value = code;
    if (store.activeFile) {
      store.activeFile.code = code;
      await nextTick();
    }
  } catch {
    scriptText.value = '// SceneScript was not found yet.';
    if (store.activeFile) {
      store.activeFile.code = scriptText.value;
      await nextTick();
    }
  }
}

async function applyScript(code = store.activeFile?.code ?? scriptText.value) {
  if (!conn) return;
  scriptText.value = code;
  const reply = await conn.scriptUpdate(
    scriptName,
    scriptText.value,
    { try: true, check_file: true, save_file: false }
  );
  console.log('[scriptUpdate]', scriptName, reply);
  await inspectEntity(scriptName);
  return reply;
}

async function loadCameraConfig() {
  if (!conn) return;

  const camReply = await conn.get('MainCamera', {
    component: 'graphics.Camera',
    values: true,
  });
  const cam = camReply?.value || camReply?.['graphics.Camera'] || camReply?.Camera || camReply;
  if (typeof cam?.fovy === 'number') {
    cameraFovy.value = cam.fovy;
  }

  const posReply = await conn.get('MainCamera', {
    component: 'graphics.Position',
    values: true,
  });
  const pos = posReply?.value || posReply?.['graphics.Position'] || posReply?.Position || posReply;
  if (typeof pos?.x === 'number' && typeof pos?.y === 'number' && typeof pos?.z === 'number') {
    cameraPos.value = { x: pos.x, y: pos.y, z: pos.z };
  }
}

async function setCameraFovy() {
  if (!conn) return;
  const reply = await conn.set('MainCamera', 'graphics.Camera', { fovy: cameraFovy.value });
  console.log('[set] MainCamera graphics.Camera', { fovy: cameraFovy.value }, reply);
}

async function setCameraPosition() {
  if (!conn) return;
  const value = {
    x: cameraPos.value.x,
    y: cameraPos.value.y,
    z: cameraPos.value.z,
  };
  const reply = await conn.set('MainCamera', 'graphics.Position', value);
  console.log('[set] MainCamera graphics.Position', value, reply);
}

async function logCameraQuery() {
  if (!conn) return;
  const reply = await conn.query('graphics.ActiveCamera, graphics.Camera, graphics.Position', {
    values: true,
    full_paths: true,
  });
  console.log('[query] MainCamera', reply);
}

async function logCameraEntity() {
  if (!conn) return;
  const reply = await conn.entity('MainCamera', {
    values: true,
    full_paths: true,
  });
  console.log('[entity] MainCamera', reply);
}

async function onReady() {
  conn = window.flecs.connect(appRef.value);
  status.value = 'ready';

  await loadScript();
  await loadCameraConfig();
  await refreshSystems();
  await inspectEntity(scriptName);

  for (const term of Object.values(terms)) {
    await setSystemEnabled(term.bind?.system, term.active);
  }
}

function onError(message) {
  status.value = message;
  console.error(message);
}

const { attach, detach, sync } = bindInteractiveMath({
  terms,
  defs: termDefs,
  onToggle: async (_key, term) => {
    await setSystemEnabled(term.bind?.system, term.active);
    await refreshSystems();
  },
});

onMounted(() => {
  attach();
  sync();
});

onBeforeUnmount(() => {
  detach();
});
</script>

Status: `{{ status }}`

Inline math:
$\t{drag_kinematic}{\mathrm{DragParticlesKinematic}} + \t{drag_spring}{\mathrm{DragParticlesSpring}}$

<Simpac
  ref="appRef"
  src="/bazel-bin/small/cloth/webapp/main.js"
  :aspectRatio="'16:9'"
  :debug="true"
  :cwrap="['flecs_explorer_request']"
  @ready="onReady"
  @error="onError"
/>

<ReplEditor
  class="h-40 rounded-md"
  :store="store"
  :initialValue="initialValue"
  filename="App.vue"
  :autoResize="false"
/>

<div class="mt-3 flex gap-2">
  <button @click="applyScript">Apply Script</button>
  <button @click="refreshSystems">Log Systems Query</button>
  <button @click="inspectEntity(scriptName)">Log SceneScript Entity</button>
</div>

## Raw Config Controls

<div>
  <label>MainCamera fovy: {{ cameraFovy.toFixed(1) }}</label>
  <input
    type="range"
    min="10"
    max="120"
    step="0.1"
    v-model.number="cameraFovy"
    @input="setCameraFovy"
  />
</div>

<div>
  <label>MainCamera position</label>
  <input type="number" step="0.1" v-model.number="cameraPos.x" @change="setCameraPosition" />
  <input type="number" step="0.1" v-model.number="cameraPos.y" @change="setCameraPosition" />
  <input type="number" step="0.1" v-model.number="cameraPos.z" @change="setCameraPosition" />
</div>

<div class="mt-2 flex gap-2">
  <button @click="logCameraQuery">Log Camera Query</button>
  <button @click="logCameraEntity">Log MainCamera Entity</button>
</div>
