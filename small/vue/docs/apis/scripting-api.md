---
title: Scripting API
---

<script setup>
import { onBeforeUnmount, ref } from 'vue';

const appRef = ref(null);
const status = ref('booting');
const scriptName = ref('SceneScript');
const code = ref('');
let conn = null;

function readScriptCode(reply) {
  const script = reply?.value ?? reply?.['flecs.script.Script'] ?? reply?.Script ?? reply;
  return script?.code || '';
}

async function loadScript() {
  if (!conn) return;
  try {
    const reply = await conn.get(scriptName.value, {
      component: 'flecs.script.Script',
      values: true,
    });
    code.value = readScriptCode(reply);
    status.value = `loaded: ${scriptName.value}`;
  } catch (error) {
    status.value = String(error?.message || error);
  }
}

async function applyScript() {
  if (!conn) return;
  try {
    const reply = await conn.scriptUpdate(scriptName.value, String(code.value || ''), {
      try: true,
      check_file: true,
      save_file: false,
    });
    if (reply?.error) throw new Error(reply.error);
    status.value = `applied: ${scriptName.value}`;
  } catch (error) {
    status.value = String(error?.message || error);
  }
}

async function onReady() {
  conn = window.flecs.connect(appRef.value);
  status.value = 'ready';
  await loadScript();
}

function onError(message) {
  status.value = String(message);
}

onBeforeUnmount(() => {
  conn?.disconnect?.();
  conn = null;
});
</script>

# Scripting API

Status: `{{ status }}`

<Simpac
  ref="appRef"
  src="/bazel-bin/small/cloth/webapp/main.js"
  aspect-ratio="16:9"
  :cwrap="['flecs_explorer_request']"
  @ready="onReady"
  @error="onError"
/>

<p>
  <label>
    Script entity:
    <input v-model="scriptName" />
  </label>
  <button @click="loadScript">Reload</button>
  <button @click="applyScript">Apply</button>
</p>

<textarea v-model="code" rows="14" style="width: 100%;"></textarea>

## Notes

- `scriptUpdate` targets a script entity name, not a file path.
- If you only need value tweaks, `conn.set(...)` is simpler than replacing the whole script.
