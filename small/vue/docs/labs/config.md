---
title: Config Lab
outline: false
---

# Config Lab

Raw entity access/control for `small/cloth`.

<script setup>
import { ref } from 'vue';

const appRef = ref(null);
const status = ref('booting');
const fovy = ref(60);
const position = ref({ x: 0, y: 5, z: 10 });
let conn = null;

async function loadCamera() {
  if (!conn) return;

  const camReply = await conn.get('MainCamera', {
    component: 'graphics.Camera',
    values: true,
  });
  const cam = camReply?.value || camReply?.['graphics.Camera'] || camReply?.Camera || camReply;
  if (typeof cam?.fovy === 'number') {
    fovy.value = cam.fovy;
  }

  const posReply = await conn.get('MainCamera', {
    component: 'graphics.Position',
    values: true,
  });
  const pos = posReply?.value || posReply?.['graphics.Position'] || posReply?.Position || posReply;
  if (typeof pos?.x === 'number' && typeof pos?.y === 'number' && typeof pos?.z === 'number') {
    position.value = { x: pos.x, y: pos.y, z: pos.z };
  }
}

async function setFovy() {
  if (!conn) return;
  const reply = await conn.set('MainCamera', 'graphics.Camera', { fovy: fovy.value });
  console.log('[set] MainCamera graphics.Camera', { fovy: fovy.value }, reply);
}

async function setPosition() {
  if (!conn) return;
  const value = {
    x: position.value.x,
    y: position.value.y,
    z: position.value.z,
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
  console.log('[query] camera', reply);
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
  await loadCamera();
}

function onError(message) {
  status.value = message;
  console.error(message);
}
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

<div>
  <label>MainCamera fovy: {{ fovy.toFixed(1) }}</label>
  <input type="range" min="10" max="120" step="0.1" v-model.number="fovy" @input="setFovy" />
</div>

<div>
  <label>MainCamera position</label>
  <input type="number" step="0.1" v-model.number="position.x" @change="setPosition" />
  <input type="number" step="0.1" v-model.number="position.y" @change="setPosition" />
  <input type="number" step="0.1" v-model.number="position.z" @change="setPosition" />
</div>

<div>
  <button @click="logCameraQuery">Log Camera Query</button>
  <button @click="logCameraEntity">Log MainCamera Entity</button>
</div>
