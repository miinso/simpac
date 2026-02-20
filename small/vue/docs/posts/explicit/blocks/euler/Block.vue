<script setup>
import BlockEulerMath from './math.md';
</script>

<template>
  <figure class="acm-algorithm ring-0">
    <figcaption class="acm-caption">
      <span class="acm-label">Algorithm 1.</span>
      <span class="acm-title">Explicit Euler Step</span>
    </figcaption>
    <div class="acm-rule"></div>
    <div class="acm-body">
      <BlockEulerMath />
    </div>
    <div class="acm-rule"></div>
  </figure>
</template>

<style scoped>
.acm-algorithm {
  margin: 0;
  padding: 0.75rem;
  background: transparent;
}

.acm-caption {
  display: flex;
  gap: 0.4rem;
  align-items: baseline;
  font-size: 0.9rem;
  font-weight: 600;
  letter-spacing: 0.01em;
}

.acm-label {
  text-transform: uppercase;
  font-size: 0.72rem;
  font-weight: 700;
}

.acm-rule {
  height: 1px;
  background: currentColor;
  opacity: 0.22;
  margin: 0.45rem 0;
}

.acm-body {
  font-size: 0.92rem;
  line-height: 1.45;
}
</style>
