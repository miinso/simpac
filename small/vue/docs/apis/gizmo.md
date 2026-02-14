---
title: Gizmo
---

# Gizmo

Tune camera and target via Flecs.

<script setup>
import { onBeforeUnmount, ref, watch } from 'vue';

const appRef = ref(null);
const fovY = ref(56);
const cameraEntity = ref('DefaultCamera');
const camPos = ref({ x: 0, y: 0, z: 0 });
const targetPos = ref({ x: 0, y: 0, z: 0 });
let conn = null;
// block echo writes while applying remote snapshot
let syncLock = false;
let statePollTimer = null;
const stopSync = [];

function stopTwoWaySync() {
  for (const stop of stopSync) stop();
  stopSync.length = 0;
}

function startTwoWaySync() {
  stopTwoWaySync();
  // local slider change -> remote write
  stopSync.push(
    watch(fovY, (value) => {
      if (syncLock || !conn) return;
      conn.set(cameraEntity.value, 'graphics.Camera', { fovy: value }).catch(console.error);
    })
  );
  stopSync.push(
    watch(
      () => [camPos.value.x, camPos.value.y, camPos.value.z],
      () => {
        if (syncLock || !conn) return;
        conn.set(cameraEntity.value, 'graphics.Position', { ...camPos.value }).catch(console.error);
      }
    )
  );
  stopSync.push(
    watch(
      () => [targetPos.value.x, targetPos.value.y, targetPos.value.z],
      () => {
        if (syncLock || !conn) return;
        conn.set(cameraEntity.value, 'graphics.Camera', { target: { ...targetPos.value } }).catch(console.error);
      }
    )
  );
}

function stopPolling() {
  if (!statePollTimer) return;
  clearInterval(statePollTimer);
  statePollTimer = null;
}

function startPolling() {
  stopPolling();
  // remote state -> local slider refresh
  statePollTimer = setInterval(() => {
    pullCameraState().catch(console.error);
  }, 16);
}

function applyRemote(camera, position) {
  syncLock = true;
  if (typeof camera?.fovy === 'number') fovY.value = Number(camera.fovy);
  camPos.value = {
    x: Number(position?.x),
    y: Number(position?.y),
    z: Number(position?.z),
  };
  targetPos.value = {
    x: Number(camera?.target?.x),
    y: Number(camera?.target?.y),
    z: Number(camera?.target?.z),
  };
  syncLock = false;
}

async function pullCameraState() {
  if (!conn) return;
  // find the active camera if its entity name changes
  const resp = await conn.query('graphics.ActiveCamera, graphics.Camera, graphics.Position', {
    table: true,
    values: true
  });
  const entity = resp?.results?.[0];
  const camera = entity?.components?.['graphics.Camera'];
  const position = entity?.components?.['graphics.Position'];
  if (!camera || !position) return;
  if (entity?.name) cameraEntity.value = entity.name;
  applyRemote(camera, position);
}

async function onReady() {
  // connect -> initial pull -> enable sync loop
  conn = window.flecs.connect(appRef.value);
  await pullCameraState();
  startTwoWaySync();
  startPolling();
}

onBeforeUnmount(() => {
  // stop timers and watchers
  stopPolling();
  stopTwoWaySync();
});
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
  <input type="range" min="0" max="120" step="0.1" v-model.number="fovY" />
</label>

<label>
  Camera position:
  <br>
  <input type="range" min="-50" max="50" step="0.1" v-model.number="camPos.x" />
  X {{ camPos.x.toFixed(2) }}
  <br>
  <input type="range" min="-50" max="50" step="0.1" v-model.number="camPos.y" />
  Y {{ camPos.y.toFixed(2) }}
  <br>
  <input type="range" min="-50" max="50" step="0.1" v-model.number="camPos.z" />
  Z {{ camPos.z.toFixed(2) }}
</label>

<label>
  Target position:
  <br>
  <input type="range" min="-10" max="10" step="0.1" v-model.number="targetPos.x" />
  X {{ targetPos.x.toFixed(2) }}
  <br>
  <input type="range" min="-10" max="10" step="0.1" v-model.number="targetPos.y" />
  Y {{ targetPos.y.toFixed(2) }}
  <br>
  <input type="range" min="-10" max="10" step="0.1" v-model.number="targetPos.z" />
  Z {{ targetPos.z.toFixed(2) }}
</label>
