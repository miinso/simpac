---
title: filesystem
---

# Filesystem demo

This page loads the filesystem demo wasm build via the Simpac Vue component.

<script setup>
import { ref } from 'vue';

const appRef = ref(null);
const status = ref('booting');
const appSrc = ref('/bazel-bin/small/filesystem/webapp/main.js');
const watchedPath = 'assets/watched.txt';
const watchedText = ref('');
const ioStatus = ref('');

async function loadWatched() {
  if (!appRef.value) return;
  try {
    ioStatus.value = 'loading...';
    watchedText.value = await appRef.value.readFile(watchedPath, 'utf8');
    ioStatus.value = 'loaded';
  } catch (err) {
    ioStatus.value = String(err);
  }
}

let saveTimer = null;

function scheduleSave() {
  if (saveTimer) clearTimeout(saveTimer);
  saveTimer = setTimeout(async () => {
    if (!appRef.value) return;
    try {
      ioStatus.value = 'saving...';
      await appRef.value.writeFile(watchedPath, watchedText.value, 'utf8');
      ioStatus.value = 'saved';
    } catch (err) {
      ioStatus.value = String(err);
    }
  }, 200);
}

async function onReady() {
  status.value = 'ready';
  await loadWatched();
}

function onError(message) {
  status.value = `error: ${message}`;
}
</script>

Status: **{{ status }}**

<Simpac
  ref="appRef"
  :src="appSrc"
  :debug="true"
  :aspectRatio="'4:3'"
  @ready="onReady"
  @error="onError"
/>

## Watched file editor

Status: **{{ ioStatus }}**

<textarea
    v-model="watchedText"
    rows="8"
    cols="60"
    placeholder="Edit assets/watched.txt"
    class="w-full rounded-md border border-slate-300 bg-white p-3 text-sm shadow-sm ring-1 ring-slate-200 focus:border-slate-400 focus:outline-none focus:ring-2 focus:ring-slate-400" @input="scheduleSave"></textarea>
