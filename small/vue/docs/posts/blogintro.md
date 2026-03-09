---
title: Blog Intro
---

# Blog Int2ro


<script setup>
import { nextTick, ref, watch } from 'vue';
import { Repl as ReplEditor, useStore } from '@vue/repl';
import { useEngineBlock } from '@simpac/lib';

const appRef = ref(null);
const scriptText = ref('');
const scriptName = 'SceneScript';
const engine = useEngineBlock(appRef);
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

async function onReady() {
  await engine.connect();
  conn = engine.state.conn;

  // enable kinematic dragging
  await engine.systems.setEnabled('graphics.PickParticles', true).catch(() => {});
  await engine.systems.setEnabled('graphics.DragParticlesKinematic', true).catch(() => {});

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
  src="/bazel-bin/small/cloth/webapp/main.js"
  :aspectRatio="'32:9'"
  :debug="true"
  :cwrap="['flecs_explorer_request']"
  @ready="onReady"
  @error="console.error($event)"
/>

바로 만져볼 수 있는 인터랙티브 데모 시리즈를 운영합니다.

애니메이션 공부를 하면서, 교과서 또는 논문을 통해 새로운 알고리즘을 접할때, 보통 우리는 수식으로 간결하게 표현된 알고리즘, 혹은 중간중간 실린 삽화를 통해 만나게 되죠.

여러분은 어떠셨나요, 저는 매 순간이 좌절이었습니다. 왜냐하면 저는 한시라도 빨리 그러한 알고리즘을 실행해보고 직접 체험해보고 싶었거든요.

문제는, 새로 만난 알고리즘을 실제 작동하는 프로그램으로 만들어내기까지, 그렇게 많은 수고가 들 거라고 예상하지 못했다는 점입니다..

근사한 언어로 컴파일된 프로그램일 수도 있고 매틀랩 프로토타입일 수도 있죠. 특별한 솔버를 준비해야하는 경우도 있고, 근사한 가속기를 장착해야만 돌아가는 경우도 있었습니다.

(초보자의 시선에서) 멋진 알고리즘들은, 책이나 pdf 문서에 정지 화상으로 남은채로, 때로는 어딘가 십년 전 비메오/유튜브에 동영상으로 박제된 채로, "너가 나를 실행해보겠다고?" 약올리는 것 같아 야속했습니다.

아주 가끔 바로 실행할 수 있는 데모나, 소스코드를 손에 넣는 행운은, 저자의 집요한 의지, 또는 그들이 아주 특별한 배려를 배풀어 주었을때만 가능한 일이었죠.

그래픽스 공부를 하면서 만난 보석같은 자료들을 어디에서든 실행 가능한 형태로 만들어 정리합니다.

I’m running a series of hands-on, immediately playable interactive demos.

When studying animation and graphics, new algorithms usually reach us through textbooks or papers—condensed into equations, or illustrated with a few carefully chosen figures along the way.

How was it for you?
For me, it was frustrating every single time. Because all I wanted was to run the algorithm as soon as possible and experience it directly.

What I didn’t expect was how much work it would take to turn a newly encountered algorithm into a working program.

Sometimes it was a neatly compiled program written in an unfamiliar language. Other times it was a MATLAB prototype. In some cases, you had to prepare a very specific solver, or equip the whole thing with a nontrivial accelerator just to get it running.

From a beginner’s perspective, many beautiful algorithms remained frozen as static images in books or PDFs—or worse, preserved as decade-old Vimeo or YouTube videos—almost as if they were mocking you:
“Oh, you want to try running me yourself?”

On rare occasions, getting your hands on a runnable demo or source code required either the author’s stubborn determination, or an unusual level of generosity on their part.

In this series, I collect gem-like materials I encountered while studying graphics, and rebuild them into forms that can be executed anywhere, immediately.


<ReplEditor class="h-32 rounded-md"
    :store="store"
    :initialValue="initialValue"
    filename="App.vue"
    :autoResize="false"/>

<div>
  <button @click="applyScript">Apply Script</button>
</div>
