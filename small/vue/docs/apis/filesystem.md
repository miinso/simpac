---
title: Filesystem
---

# Filesystem

Edit a watched file in the WASM filesystem.

<script setup>
import { ref } from 'vue';

const appRef = ref(null);
const watchedText = ref('');
const watchedPath = 'assets/watched.txt';

async function loadWatched() {
  if (!appRef.value) return;
  watchedText.value = await appRef.value.readFile(watchedPath, 'utf8');
}

async function saveWatched() {
  if (!appRef.value) return;
  await appRef.value.writeFile(watchedPath, watchedText.value, 'utf8');
}
</script>

<Simpac
  ref="appRef"
  src="/bazel-bin/small/filesystem/webapp/main.js"
  :debug="true"
  :aspectRatio="'4:3'"
  @ready="loadWatched"
  @error="console.error($event)"
/>

## Watched file

<textarea
    v-model="watchedText"
    rows="8"
    cols="60"
    placeholder="Edit assets/watched.txt"
    class="w-full rounded-md border border-slate-300 bg-white p-3 text-sm shadow-sm ring-1 ring-slate-200 focus:border-slate-400 focus:outline-none focus:ring-2 focus:ring-slate-400" @input="saveWatched"></textarea>
