---
title: gizmo
---

<script setup>
import { ref, onMounted } from 'vue';

const appRef = ref(null);
const fovY = ref(56);
const cameraEntity = ref('');
const camPos = ref({ x: 0, y: 0, z: 0 });
const targetPos = ref({ x: 0, y: 0, z: 0 });
const status = ref('booting');
// const flecsReady = ref(false);
let conn = null;

const appSrc = ref('/bazel-bin/small/gizmo/webapp/main.js');

onMounted(() => {
//   flecsReady.value = typeof window !== 'undefined' && !!window.flecs;
});

async function onReady() {
  status.value = 'ready';

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

  console.log(resp)

//   conn.query('graphics.ActiveCamera, graphics.Camera, graphics.Position', {
//     table: true,
//     values: true
//   }, (resp) => {console.log(resp)}, (e) => {console.error(e)});

  const entity = resp?.results?.[0];
  if (!entity) throw new Error('Active camera not found');
  const camera = entity?.components?.['graphics.Camera'];
  const position = entity?.components?.['graphics.Position'];
  if (!camera || !position) throw new Error('Missing camera components');

  cameraEntity.value = entity?.name || 'DefaultCamera';
  if (typeof camera.fovy === 'number') fovY.value = camera.fovy;
  camPos.value = position;
  console.log(camPos.value)

  targetPos.value = camera.target;
}

function onError(message) {
  status.value = `error: ${message}`;
}

// method 1. define event
function onInput() {
  if (!appRef.value || !conn) return;
  const target = cameraEntity.value || 'DefaultCamera';
  conn
    .set(target, 'graphics.Camera', { fovy: fovY.value })
    .then((resp) => console.log('fovy update', resp))
    .catch((err) => console.error('fovy update failed', err));
}

function setCameraPos() {
  if (!appRef.value || !conn) return;
  const target = cameraEntity.value || 'DefaultCamera';
  conn
    .set(target, 'graphics.Position', { ...camPos.value })
    .then((resp) => console.log('camera position update', resp))
    .catch((err) => console.error('camera position update failed', err));
}

function setTargetPos() {
  if (!appRef.value || !conn) return;
  const target = cameraEntity.value || 'DefaultCamera';
  conn
    .set(target, 'graphics.Camera', { target: { ...targetPos.value } })
    .then((resp) => console.log('camera target update', resp))
    .catch((err) => console.error('camera target update failed', err));
}

// i see an exciting opportunity here..
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
