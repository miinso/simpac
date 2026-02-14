<script setup>
import Simpac from '../../.vitepress/theme/components/Simpac.vue';
import Block2Math from './Block2Math.md';

const title = 'Block2: Force Decomposition';
const wasmSrc = '/bazel-bin/small/cloth/webapp/main.js';
</script>

<template>
  <div class="ring-2 p-3">
    <div class="space-y-3">
      <p class="m-0 text-base font-semibold">{{ title }}</p>
      <div class="aspect-[16/9]">
        <Simpac :src="wasmSrc" aspect-ratio="16:9" />
      </div>
      <div class="space-y-2 text-sm">
        <Block2Math />
      </div>
    </div>
  </div>
</template>
