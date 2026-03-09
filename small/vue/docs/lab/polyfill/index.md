---
title: Touch Test
---

# DOM Polyfill Test

Verifies mouse, touch, gesture, keyboard, and wheel all work through the worker DOM polyfill.

<script setup>
import { ref } from 'vue';
import appSrc from '/polyfill/main.js?url';
const status = ref('booting');
</script>

Status: `{{ status }}`

<Simpac
  :src="appSrc"
  aspect-ratio="4:3"
  @ready="status = 'ready'"
  @error="(msg) => status = msg"
/>


**What to test:**
- Mouse movement (crosshair should follow cursor)
- Left/right click (circle feedback)
- Scroll wheel (wheel values update)
- Touch points (green circles on mobile)
- Gestures: tap, drag, pinch (gesture name shown)
- Keyboard: arrow keys, space, any key (last key shown)
