---
title: repl
---

# repl


<script setup>
import { nextTick, ref, watch } from 'vue';
import { useStore } from '@vue/repl';

const appRef = ref(null);
const scriptText = ref('');
const scriptName = 'SceneScript';
let conn = null;

// repl
const store = useStore();
const initialValue = '// Loading script...'
let suppressApply = false;

watch(
  () => store.activeFile?.code,
  (code) => {
    if (code == null) return;
    scriptText.value = code;
    // if (suppressApply) {
    //   suppressApply = false;
    //   return;
    // }
    if (!conn) return;
    applyScript(code);
  }
);

function onReady() {
  conn = window.flecs.connect(appRef.value);

  // print the entire world (serialized)
  conn.world((world) => { console.log(world) });

  // get entity with full table info,
  // conn.entity("SceneScript", { table: true })
  // or get component right away
  conn.get(scriptName, { component: "flecs.script.Script" }, async (resp) => {
    console.log(resp)
    scriptText.value = resp?.code || '';
    // suppressApply = true;
    store.activeFile.code = scriptText.value;
    await nextTick();
  }, (e) => {
    console.error(e);
  });
}

async function applyScript(code = store.activeFile?.code ?? scriptText.value) {
  if (!conn) return;
  scriptText.value = code;
  const reply = await conn.scriptUpdate(
    scriptName,
    code,
    { try: true, check_file: true, save_file: false }
  );
  console.log(reply);
}

async function worldQuery() {
  if (!conn) return;
  const reply = await conn.world();
  console.log(reply);
}
</script>

<Simpac
  ref="appRef"
  src="/bazel-bin/small/implicit-euler/webapp/main.js"
  :aspectRatio="'4:3'"
  :debug="true"
  :cwrap="['flecs_explorer_request']"
  @ready="onReady"
  @error="console.error($event)"
/>

<Repl class="h-32 rounded-md"
    :store="store"
    :initialValue="initialValue"
    filename="App.vue"
    :autoResize="false"/>

<div>
  <button @click="applyScript">Apply Script</button>
</div>
