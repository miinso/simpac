---
title: Vue REPL Fork
---

<script setup>
import { ref } from 'vue'
import { Repl } from '@vue/repl'
import '@vue/repl/style.css'

const replRef = ref(null)
const initialValue = `<template>\n  <div>Hello from the fork</div>\n</template>\n`
</script>

# Vue REPL Fork

Use the forked `@vue/repl` package directly in this VitePress docs site.

## Install from GitHub

```bash
npm install github:miinso/repl#plain
```

If you install from GitHub, the fork should include `dist/` in the repo (or add a `prepare` script to build on install).

## Live demo

<ClientOnly>
  <div style="height: 360px; border: 1px solid #ddd;">
    <Repl
      ref="replRef"
      :initialValue="initialValue"
      filename="App.vue"
      :autoResize="false"
    />
  </div>
</ClientOnly>

## Usage in a VitePress post

```vue
<script setup>
import { ref } from 'vue'
import { Repl } from '@vue/repl'
import '@vue/repl/style.css'

const replRef = ref(null)
const initialValue = `<template>\n  <div>Hello from the fork</div>\n</template>\n`
</script>

<ClientOnly>
  <div style="height: 360px; border: 1px solid #ddd;">
    <Repl
      ref="replRef"
      :initialValue="initialValue"
      filename="App.vue"
      :autoResize="false"
    />
  </div>
</ClientOnly>
```

## Notes

- This uses `ClientOnly` to avoid SSR errors while keeping a static import.
- If you want to read the current contents, bind a ref and call `getValue()`.
