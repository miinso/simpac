---
title: Tweak Lab
outline: false
---

# Tweak Lab

Tweakpane-driven `vec3` control for `MainCamera` position.

<script setup>
import { Pane } from 'tweakpane';
import { onBeforeUnmount, onMounted, ref } from 'vue';

const appRef = ref(null);
const paneHost = ref(null);
const status = ref('booting');
const position = ref({ x: 0, y: 5, z: 10 });
const paneVec3 = { x: 0, y: 5, z: 10 };
let conn = null;
let pane = null;

function syncPaneVec3() {
  paneVec3.x = position.value.x;
  paneVec3.y = position.value.y;
  paneVec3.z = position.value.z;
  pane?.refresh();
}

async function loadCameraPosition() {
  if (!conn) return;
  const posReply = await conn.get('MainCamera', {
    component: 'graphics.Position',
    values: true,
  });
  const pos = posReply?.value || posReply?.['graphics.Position'] || posReply?.Position || posReply;
  if (typeof pos?.x === 'number' && typeof pos?.y === 'number' && typeof pos?.z === 'number') {
    position.value = { x: pos.x, y: pos.y, z: pos.z };
    syncPaneVec3();
  }
}

async function setCameraPosition() {
  if (!conn) return;
  const value = {
    x: position.value.x,
    y: position.value.y,
    z: position.value.z,
  };
  const reply = await conn.set('MainCamera', 'graphics.Position', value);
  console.log('[set] MainCamera graphics.Position', value, reply);
}

async function initPane() {
  if (typeof window === 'undefined' || !Pane || !paneHost.value || pane) return;

  pane = new Pane({
    container: paneHost.value,
    title: 'MainCamera Position',
  });

  const folder = pane.addFolder({ title: 'vec3' });
  const opts = { min: -50, max: 50, step: 0.1 };
  const onChange = async () => {
    position.value = { x: paneVec3.x, y: paneVec3.y, z: paneVec3.z };
    await setCameraPosition();
  };

  folder.addBinding(paneVec3, 'x', opts).on('change', onChange);
  folder.addBinding(paneVec3, 'y', opts).on('change', onChange);
  folder.addBinding(paneVec3, 'z', opts).on('change', onChange);
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
  await loadCameraPosition();
}

function onError(message) {
  status.value = message;
  console.error(message);
}

onMounted(() => {
  initPane().catch(console.error);
});

onBeforeUnmount(() => {
  pane?.dispose();
  pane = null;
});
</script>

Status: `{{ status }}`

<Simpac
  ref="appRef"
  src="/bazel-bin/small/cloth/webapp/main.js"
  :debug="true"
  :cwrap="['flecs_explorer_request']"
  @ready="onReady"
  @error="onError"
/>

<div ref="paneHost"></div>

<div>
  <button @click="loadCameraPosition">Reload Position</button>
  <button @click="logCameraEntity">Log MainCamera Entity</button>
</div>
