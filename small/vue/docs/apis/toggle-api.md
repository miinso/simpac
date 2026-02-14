---
title: Toggle API
---

<script setup>
import { onBeforeUnmount, reactive, ref } from 'vue';
import { useEngineBlock } from '../.vitepress/theme/lib/index.js';

const appRef = ref(null);
const status = ref('booting');
const engine = useEngineBlock(appRef);

const systems = {
  particles: 'graphics.DrawParticlesGPU',
  springs: 'graphics.DrawSpringsGPU',
  drag: 'graphics.DragParticlesSpring',
};

const flags = reactive({
  particles: false,
  springs: false,
  drag: false,
});

async function syncToggles() {
  try {
    const rows = await engine.systems.listMap();
    flags.particles = !!rows.get(systems.particles)?.enabled;
    flags.springs = !!rows.get(systems.springs)?.enabled;
    flags.drag = !!rows.get(systems.drag)?.enabled;
    status.value = 'synced';
  } catch (error) {
    status.value = String(error?.message || error);
  }
}

async function setEnabled(key) {
  const path = systems[key];
  if (!path) return;
  try {
    await engine.systems.setEnabled(path, !!flags[key]);
    status.value = `${path}: ${flags[key] ? 'enabled' : 'disabled'}`;
  } catch (error) {
    status.value = String(error?.message || error);
  }
}

async function onReady() {
  await engine.connect();
  status.value = 'ready';
  await syncToggles();
}

function onError(message) {
  status.value = String(message);
}

onBeforeUnmount(() => {
  engine.disconnect();
});
</script>

# Toggle API

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
  <button @click="syncToggles">Refresh</button>
</p>

<p>
  <label>
    <input type="checkbox" v-model="flags.particles" @change="setEnabled('particles')" />
    graphics.DrawParticlesGPU
  </label>
  <br>
  <label>
    <input type="checkbox" v-model="flags.springs" @change="setEnabled('springs')" />
    graphics.DrawSpringsGPU
  </label>
  <br>
  <label>
    <input type="checkbox" v-model="flags.drag" @change="setEnabled('drag')" />
    graphics.DragParticlesSpring
  </label>
</p>

## Notes

- If you need a toggle UI, use `useEngineBlock(...).math.systemToggles(...)`.
- If system paths contain spaces, encode the path before calling remote endpoints.
