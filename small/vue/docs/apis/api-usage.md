---
title: API Usage
---

<script setup>
import { onBeforeUnmount, ref } from 'vue';

const appRef = ref(null);
const status = ref('booting');
const systemCount = ref(0);
let conn = null;

async function refreshSystemCount() {
  if (!conn) return;
  const reply = await conn.query('flecs.system.System, !ChildOf($this|up, flecs)', {
    full_paths: true,
    values: false,
  });
  systemCount.value = (reply?.results || []).length;
}

async function onReady() {
  conn = window.flecs.connect(appRef.value);
  status.value = 'ready';
  await refreshSystemCount();
}

function onError(message) {
  status.value = String(message);
}

onBeforeUnmount(() => {
  conn?.disconnect?.();
  conn = null;
});
</script>

# API Usage

Use this page as a quick entry point. For full examples, open each API page.

Status: `{{ status }}`

Systems: `{{ systemCount }}`

<Simpac
  ref="appRef"
  src="/bazel-bin/small/cloth/webapp/main.js"
  aspect-ratio="16:9"
  :cwrap="['flecs_explorer_request']"
  @ready="onReady"
  @error="onError"
/>

<p>
  <button @click="refreshSystemCount">Refresh System Count</button>
</p>

## Jump

- [Config API](/apis/config-api)
- [System API](/apis/system-api)
- [Scripting API](/apis/scripting-api)
- [Toggle API](/apis/toggle-api)
- [Gizmo API](/apis/gizmo-api)
- [FS API](/apis/fs-api)

## Minimal connection pattern

```js
const conn = window.flecs.connect(appRef.value);

const systems = await conn.query('flecs.system.System, !ChildOf($this|up, flecs)', {
  full_paths: true,
  values: false,
});
console.log(systems?.results?.length);
```
