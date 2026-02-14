---
title: FS API
---

<script setup>
import { ref } from 'vue';

const appRef = ref(null);
const status = ref('booting');
const filePath = ref('assets/watched.txt');
const text = ref('');

async function loadFile() {
  if (!appRef.value) return;
  try {
    text.value = await appRef.value.readFile(filePath.value, 'utf8');
    status.value = `loaded: ${filePath.value}`;
  } catch (error) {
    status.value = String(error?.message || error);
  }
}

async function saveFile() {
  if (!appRef.value) return;
  try {
    await appRef.value.writeFile(filePath.value, text.value, 'utf8');
    status.value = `saved: ${filePath.value}`;
  } catch (error) {
    status.value = String(error?.message || error);
  }
}

async function onReady() {
  status.value = 'ready';
  await loadFile();
}

function onError(message) {
  status.value = String(message);
}
</script>

# FS API

Status: `{{ status }}`

<Simpac
  ref="appRef"
  src="/bazel-bin/small/filesystem/webapp/main.js"
  aspect-ratio="16:9"
  @ready="onReady"
  @error="onError"
/>

<p>
  <label>
    Path:
    <input v-model="filePath" @input="loadFile" />
  </label>
</p>

<textarea v-model="text" rows="12" style="width: 100%;" @input="saveFile"></textarea>

## Surface

- `readFile(path, encoding?)`
- `writeFile(path, data, encoding?)`
- This is a runtime/dev flow. Persistence depends on your runtime policy.
