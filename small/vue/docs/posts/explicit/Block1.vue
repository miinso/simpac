<script setup>
import { onBeforeUnmount, onMounted, ref } from 'vue';
import Simpac from '../../.vitepress/theme/components/Simpac.vue';
import Block1Math from './Block1Math.md';
import block1Script from './block1script.flecs?raw';
import { useEngineBlock } from '../../.vitepress/theme/lib/index.js';

const title = 'Block1: Explicit Step';
const wasmSrc = '/bazel-bin/small/cloth/webapp/main.js';
const appRef = ref(null);
const engine = useEngineBlock(appRef);

const toggles = engine.math.systemToggles({
  b1_spring: {
    label: 'f_spring',
    system: 'Implicit Euler.Collect Spring Gradient',
    default: true,
    classes: ['text-emerald-600'],
  },
  b1_gravity: {
    label: 'f_gravity',
    system: 'Implicit Euler.Collect External Force',
    default: true,
    classes: ['text-violet-600'],
  },
}, {
  onError: (error, key) => console.error(`[block1] toggle ${key} failed`, error),
});

const terms = toggles.terms;

async function onReady() {
  await engine.connect();
  await engine.script.apply('Block1Config', block1Script);
  await engine.config.set('Config.graphics.backgroundColor', 'color4f', { r: 0, g: 0, b: 0, a: 0 });
  await engine.config.set('Config.graphics.showFps', 'flecs.meta.bool', 0);
  await engine.config.set('Config.graphics.debugOptions.showGrid', 'flecs.meta.bool', 0);
  await engine.systems.setEnabled('graphics.DrawTimingInfo', false).catch(() => {});
  await engine.systems.setEnabled('graphics.DrawSolveHistory', false).catch(() => {});
  await engine.systems.setEnabled('graphics.DragParticlesSpring', true).catch(() => {});
  await toggles.sync();
}

onMounted(toggles.attach);
onBeforeUnmount(() => {
  toggles.detach();
  engine.disconnect();
});
</script>

<template>
  <div class="ring-2 p-3">
    <div class="grid gap-4 md:grid-cols-[3fr,2fr]">
      <div class="aspect-[9/16]">
        <Simpac
          ref="appRef"
          :src="wasmSrc"
          aspect-ratio="9:16"
          @ready="onReady"
          @error="console.error($event)"
        />
      </div>
      <div class="space-y-2">
        <p class="m-0 text-base font-semibold">{{ title }}</p>
        <div class="space-y-2 text-sm">
          <Block1Math />
        </div>
      </div>
    </div>
  </div>
</template>
