<script setup>
import { onBeforeUnmount, onMounted, ref } from 'vue';
import IntroMath from './math.md';
import sceneCode from './scene.flecs?raw';
import { useEngineBlock } from '../../../../.vitepress/theme/lib/index.js';

const title = 'Intro 3: Pinned + Spring';
const wasmSrc = '/bazel-bin/small/cloth/webapp/main.js';
const appRef = ref(null);
const engine = useEngineBlock(appRef);

const toggles = engine.math.systemToggles({
  i3_gravity: {
    label: 'f_gravity',
    system: 'Implicit Euler.Collect External Force',
    default: false,
    classes: ['text-violet-600'],
  },
}, {
  onError: (error) => console.error('[intro3] toggle failed', error),
});

async function reloadScene() {
  await engine.script.apply('Intro3Scene', sceneCode);
}

async function onReady() {
  await engine.connect();
  await reloadScene();
  await engine.config.set('Config.graphics.showFps', 'flecs.meta.bool', 0);
  await engine.config.set('Config.graphics.debugOptions.showGrid', 'flecs.meta.bool', 0);
  await engine.systems.setEnabled('graphics.DrawTimingInfo', false).catch(() => {});
  await engine.systems.setEnabled('graphics.DrawSolveHistory', false).catch(() => {});
  await engine.systems.setEnabled('Implicit Euler.Collect External Force', false).catch(() => {});
  await toggles.sync();
}

function onSceneDblClick() {
  reloadScene().catch((error) => console.error('[intro3] scene reload failed', error));
}

onMounted(toggles.attach);
onBeforeUnmount(() => {
  toggles.detach();
  engine.disconnect();
});
</script>

<template>
  <div class="ring-2 p-3">
    <p class="m-0 mb-3 text-base font-semibold">{{ title }}</p>
    <div class="flex flex-col gap-3 lg:flex-row lg:items-start">
      <div class="shrink-0 lg:basis-[20%]">
        <div class="mx-auto w-full max-w-[220px]" @dblclick="onSceneDblClick">
          <Simpac
            ref="appRef"
            :src="wasmSrc"
            aspect-ratio="3:4"
            @ready="onReady"
            @error="console.error($event)"
          />
        </div>
      </div>
      <div class="min-w-0 flex flex-col gap-3 lg:basis-[80%]">
        <div class="ring-2 p-3 text-sm">
          <IntroMath />
        </div>
        <div class="ring-2 p-3 text-sm leading-6">
          <p class="m-0">Turn on <code>f_gravity</code> to start motion.</p>
          <p class="m-0 mt-2">This time the left particle is pinned and a spring connects the pair, so the response is constrained.</p>
        </div>
      </div>
    </div>
  </div>
</template>
