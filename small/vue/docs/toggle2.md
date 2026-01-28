---
title: Toggle Lab 2
outline: false
---

# Toggle Lab 2

Click the symbols to toggle draw systems via Flecs REST.

<script setup>
import { onBeforeUnmount, onMounted, ref } from 'vue';
import { bindInteractiveMath, createTerms } from './interactive-math.js';

const appRef = ref(null);
let conn = null;

const termDefs = {
  draw_particles: {
    label: 'draw particles',
    kind: 'toggle',
    default: true,
    classes: ['text-sky-500'],
    bind: { system: 'graphics.DrawParticlesGPU' },
  },
  draw_springs: {
    label: 'draw springs',
    kind: 'toggle',
    default: true,
    classes: ['text-emerald-500'],
    bind: { system: 'graphics.DrawSpringsGPU' },
  },
};

const terms = createTerms(termDefs);

async function setSystemEnabled(systemName, enabled) {
  if (!conn || !systemName) return;
  let request = conn.disable;
  if (enabled) request = conn.enable;
  const resp = await request(systemName);
  console.log(resp);
}

function onReady() {
  conn = window.flecs.connect(appRef.value);
  for (const term of Object.values(terms)) {
    setSystemEnabled(term.bind?.system, term.active).catch((err) => {
      console.error(err);
    });
  }
}

const { attach, detach } = bindInteractiveMath({
  terms,
  defs: termDefs,
  onToggle: (key, term) => {
    setSystemEnabled(term.bind?.system, term.active).catch((err) => {
      console.error(err);
    });
  },
});

onMounted(attach);
onBeforeUnmount(detach);
</script>

Inline math:
$\t{draw_particles}{\mathrm{DrawParticlesGPU}} + \t{draw_springs}{\mathrm{DrawSpringsGPU}}$.

$$
\t{draw_particles}{\mathrm{DrawParticlesGPU}}(x, v, m)
$$

$$
\t{draw_springs}{\mathrm{DrawSpringsGPU}}(x, v, m)
$$

<Simpac
  ref="appRef"
  src="/bazel-bin/small/implicit-euler/webapp/main.js"
  :debug="true"
  :cwrap="['flecs_explorer_request']"
  @ready="onReady"
  @error="console.error($event)"
/>
