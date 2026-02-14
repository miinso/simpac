<script setup>
import { computed, ref } from 'vue';
import katex from 'katex';

const inlineLatex = String.raw`x_i,\ \Delta t,\ \nabla \cdot v,\ \sum_{i=1}^{n},\ \sqrt{x^2+y^2}`;
const blockLatex = [
  String.raw`\alpha,\ \beta,\ \gamma,\ \lambda,\ \mu,\ \sigma,\ \omega`,
  String.raw`\int_{\Omega} f(x)\,dx,\quad \frac{\partial x}{\partial t},\quad \left\lVert v \right\rVert_2,\quad \mathbb{R}^3`,
];

const inlineHtml = computed(() => {
  try {
    return katex.renderToString(inlineLatex, {
      displayMode: false,
      throwOnError: false,
      strict: 'ignore',
    });
  } catch {
    return inlineLatex;
  }
});

const blockHtml = computed(() => {
  return blockLatex.map((value) => {
    try {
      return katex.renderToString(value, {
        displayMode: true,
        throwOnError: false,
        strict: 'ignore',
      });
    } catch {
      return value;
    }
  });
});

const manualLatex = ref(String.raw`\int_0^{\infty} e^{-t^2} dt = \frac{\sqrt{\pi}}{2}`);

const manualResult = computed(() => {
  try {
    return {
      html: katex.renderToString(manualLatex.value, {
        displayMode: true,
        throwOnError: false,
        strict: 'ignore',
      }),
      error: '',
    };
  } catch (error) {
    return {
      html: '',
      error: String(error?.message || error),
    };
  }
});
</script>

<template>
  <div class="flex flex-col gap-4">
    <section class="ring-2 p-3">
      <p class="text-base font-semibold">KaTeX Symbols</p>
      <div class="mt-2 text-sm" v-html="inlineHtml"></div>
      <div class="mt-3 space-y-2">
        <div v-for="(entry, index) in blockHtml" :key="index" v-html="entry"></div>
      </div>
    </section>

    <section class="ring-2 p-3">
      <p class="text-sm font-semibold">Manual KaTeX (v-html)</p>
      <input
        v-model="manualLatex"
        class="w-full border-b border-slate-200 bg-transparent text-sm focus:outline-none"
        placeholder="LaTeX 입력"
      />
      <div v-if="manualResult.error" class="text-xs text-red-600">{{ manualResult.error }}</div>
      <div v-else class="mt-2" v-html="manualResult.html"></div>
    </section>
  </div>
</template>
