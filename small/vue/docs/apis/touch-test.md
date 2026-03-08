---
title: Touch Test
---

<script setup>
import { ref } from 'vue';
const status = ref('booting');
</script>

# Touch Test

Status: `{{ status }}`

<Simpac
  src="/touch/main.js"
  aspect-ratio="4:3"
  @ready="status = 'ready'"
  @error="(msg) => status = msg"
/>
