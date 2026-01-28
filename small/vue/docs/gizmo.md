---
title: Gizmo
---

# Gizmo

Tune camera and target via Flecs.

<script setup>
import { ref } from 'vue';

const appRef = ref(null);
const fovY = ref(56);
const cameraEntity = ref('DefaultCamera');
const camPos = ref({ x: 0, y: 0, z: 0 });
const targetPos = ref({ x: 0, y: 0, z: 0 });
// const flecsReady = ref(false);
let conn = null;

// onMounted(() => {
//   flecsReady.value = typeof window !== 'undefined' && !!window.flecs;
// });

async function onReady() {
  conn = window.flecs.connect(appRef.value);

  // print the entire world (serialized)
  conn.world((world) => { console.log(world) });

  // we get entity by their name (the path), and access their component info
  // conn.get('DefaultCamera', { component: 'graphics.Camera', values: true }).then((reply) => {
  //   const value = reply?.value || reply?.Camera || reply;
  //   if (value && typeof value.fovy === 'number') fovY.value = value.fovy;
  // });

  // or..
  // we can query entity by some tag, and use that.
  // in case we don't have the exact name for the camera entity,
  // we can query the thing with tag matching.
  const resp = await conn.query('graphics.ActiveCamera, graphics.Camera, graphics.Position', {
    table: true,
    values: true
  });

  //   conn.query('graphics.ActiveCamera, graphics.Camera, graphics.Position', {
  //     table: true,
  //     values: true
  //   }, (resp) => {console.log(resp)}, (e) => {console.error(e)});

  const entity = resp?.results?.[0];
  const camera = entity?.components?.['graphics.Camera'];
  const position = entity?.components?.['graphics.Position'];
  if (!camera || !position) return;

  if (entity?.name) cameraEntity.value = entity.name;
  if (typeof camera.fovy === 'number') fovY.value = camera.fovy;
  camPos.value = position;

  if (camera.target) targetPos.value = camera.target;
}

// method 1. define event
function onInput() {
  if (!conn) return;
  conn.set(cameraEntity.value, 'graphics.Camera', { fovy: fovY.value }).catch(console.error);
}

function setCameraPos() {
  if (!conn) return;
  conn.set(cameraEntity.value, 'graphics.Position', { ...camPos.value }).catch(console.error);
}

function setTargetPos() {
  if (!conn) return;
  conn.set(cameraEntity.value, 'graphics.Camera', { target: { ...targetPos.value } }).catch(console.error);
}

// i see an exciting opportunity here..
</script>

<Simpac
  ref="appRef"
  src="/bazel-bin/small/gizmo/webapp/main.js"
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

<label>
  Camera position:
  <br>
  <input type="range" min="-50" max="50" step="0.1" v-model.number="camPos.x" @input="setCameraPos" />
  X {{ camPos.x.toFixed(2) }}
  <br>
  <input type="range" min="-50" max="50" step="0.1" v-model.number="camPos.y" @input="setCameraPos" />
  Y {{ camPos.y.toFixed(2) }}
  <br>
  <input type="range" min="-50" max="50" step="0.1" v-model.number="camPos.z" @input="setCameraPos" />
  Z {{ camPos.z.toFixed(2) }}
</label>

<label>
  Target position:
  <br>
  <input type="range" min="-10" max="10" step="0.1" v-model.number="targetPos.x" @input="setTargetPos" />
  X {{ targetPos.x.toFixed(2) }}
  <br>
  <input type="range" min="-10" max="10" step="0.1" v-model.number="targetPos.y" @input="setTargetPos" />
  Y {{ targetPos.y.toFixed(2) }}
  <br>
  <input type="range" min="-10" max="10" step="0.1" v-model.number="targetPos.z" @input="setTargetPos" />
  Z {{ targetPos.z.toFixed(2) }}
</label>
