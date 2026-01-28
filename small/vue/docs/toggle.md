---
title: Toggle Lab
outline: false
---

# Toggle Lab

Click the math symbols to toggle terms. Parameters use sliders below.

<script setup>
import { onBeforeUnmount, onMounted } from 'vue';
import { bindInteractiveMath, createTerms } from './interactive-math.js';

const termDefs = {
  gravity: {
    label: 'gravity',
    kind: 'toggle',
    default: true,
    classes: ['text-violet-500'],
  },
  spring: {
    label: 'spring',
    kind: 'toggle',
    default: true,
    classes: ['text-emerald-600'],
  },
};

const paramDefs = {
  dt: {
    label: 'dt',
    kind: 'range',
    default: 0.016,
    min: 0.001,
    max: 0.05,
    step: 0.001,
    classes: ['text-sky-500'],
  },
  k: {
    label: 'k',
    kind: 'range',
    default: 100,
    min: 0,
    max: 1000,
    step: 1,
    classes: ['text-amber-600'],
  },
};

const defs = { ...termDefs, ...paramDefs };
const terms = createTerms(defs);

const { attach, detach } = bindInteractiveMath({
  terms,
  defs,
});

onMounted(attach);
onBeforeUnmount(detach);
</script>

Inline math: $\t{spring}{f_{spring}} + \t{gravity}{f_{gravity}}$.

$$
\begin{aligned}
x_{t+1} &= x_t + \p{dt}{\Delta t} v_t \\
v_{t+1} &= v_t + \p{dt}{\Delta t} M^{-1} ( \t{spring}{f_{spring}} + \t{gravity}{f_{gravity}} )
\end{aligned}
$$

$$
f_{spring} = -\p{k}{k}(x - x_0)
$$

<div class="mt-6 grid max-w-lg gap-4">
  <div class="grid gap-3">
    <label v-for="(def, key) in paramDefs" :key="key" class="grid gap-1 text-sm">
      <span class="font-medium text-slate-700">
        {{ def.label }}: {{ terms[key].value }}
      </span>
      <input
        type="range"
        class="w-full"
        :min="def.min"
        :max="def.max"
        :step="def.step"
        v-model.number="terms[key].value"
      />
    </label>
  </div>
</div>
