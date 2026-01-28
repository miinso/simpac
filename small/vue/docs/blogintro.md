---
title: Blog Intro
---

# Blog Intro

Find scene parameters for the intro page.

<script setup>
import { ref } from 'vue';

const appRef = ref(null);
const scriptText = ref('');
const scriptName = 'SceneScript';
let conn = null;

function onReady() {
  conn = window.flecs.connect(appRef.value);

  // print the entire world (serialized)
  conn.world((world) => { console.log(world) });

  conn.get(scriptName, { component: "flecs.script.Script" }, (resp) => {
    console.log(resp)
    scriptText.value = resp?.code || '';
  }, (e) => {
    console.error(e);
  });
}

async function applyScript() {
  if (!conn) return;
  const reply = await conn.scriptUpdate(
    scriptName,
    scriptText.value,
    { try: true, check_file: true, save_file: false }
  );
  console.log(reply);
}
</script>

<Simpac
  ref="appRef"
  src="/bazel-bin/small/implicit-euler/webapp/main.js"
  :aspectRatio="'32:9'"
  :debug="true"
  :cwrap="['flecs_explorer_request']"
  @ready="onReady"
  @error="console.error($event)"
/>

<textarea v-model="scriptText" rows="14" cols="80"></textarea>
<div>
  <button @click="applyScript">Apply Script</button>
</div>

