---
title: Gizmo API
---

<script setup>
import { onBeforeUnmount, reactive, ref } from 'vue';
import { useEngineBlock } from '../.vitepress/theme/lib/index.js';

const appRef = ref(null);
const status = ref('booting');
const engine = useEngineBlock(appRef);

const position = reactive({ x: 0, y: 3, z: 10 });
const target = reactive({ x: 0, y: 1.5, z: 0 });
const fovy = ref(60);
const projection = ref(0);

async function loadCamera() {
  try {
    const pos = await engine.config.get('DefaultCamera', 'graphics.Position');
    Object.assign(position, pos || {});

    const cam = await engine.config.get('DefaultCamera', 'graphics.Camera');
    Object.assign(target, cam?.target || {});
    if (Number.isFinite(cam?.fovy)) fovy.value = cam.fovy;
    if (Number.isFinite(cam?.projection)) projection.value = cam.projection;

    status.value = 'loaded camera';
  } catch (error) {
    status.value = String(error?.message || error);
  }
}

async function applyCamera() {
  if (!engine.state.ready) return;
  try {
    await engine.config.set('DefaultCamera', 'graphics.Position', {
      x: Number(position.x),
      y: Number(position.y),
      z: Number(position.z),
    });

    await engine.config.set('DefaultCamera', 'graphics.Camera', {
      target: {
        x: Number(target.x),
        y: Number(target.y),
        z: Number(target.z),
      },
      fovy: Number(fovy.value),
      projection: Number(projection.value),
    });

    status.value = 'applied camera';
  } catch (error) {
    status.value = String(error?.message || error);
  }
}

async function onReady() {
  await engine.connect();
  status.value = 'ready';
  await loadCamera();
}

function onError(message) {
  status.value = String(message);
}

onBeforeUnmount(() => {
  engine.disconnect();
});
</script>

# Gizmo API

Status: `{{ status }}`

<Simpac
  ref="appRef"
  src="/bazel-bin/small/cloth/webapp/main.js"
  aspect-ratio="16:9"
  :cwrap="['flecs_explorer_request']"
  @ready="onReady"
  @error="onError"
/>

<p>
  <button @click="loadCamera">Reload Camera</button>
</p>

<p>
  <label>x <input type="number" step="0.1" v-model.number="position.x" @input="applyCamera" /></label>
  <label>y <input type="number" step="0.1" v-model.number="position.y" @input="applyCamera" /></label>
  <label>z <input type="number" step="0.1" v-model.number="position.z" @input="applyCamera" /></label>
</p>

<p>
  <label>target.x <input type="number" step="0.1" v-model.number="target.x" @input="applyCamera" /></label>
  <label>target.y <input type="number" step="0.1" v-model.number="target.y" @input="applyCamera" /></label>
  <label>target.z <input type="number" step="0.1" v-model.number="target.z" @input="applyCamera" /></label>
</p>

<p>
  <label>fovy <input type="number" step="0.1" v-model.number="fovy" @input="applyCamera" /></label>
  <label>projection <input type="number" min="0" max="1" step="1" v-model.number="projection" @input="applyCamera" /></label>
</p>

## Notes

- Active camera selection uses the pair `(graphics.ActiveCamera, CameraEntity)` on `graphics.ActiveCameraSource`.
- In docs examples, `DefaultCamera` is the common target to edit.
