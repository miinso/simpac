---
title: Repl API
---

# Repl API

`@vue/repl` usage guide for this project.

## Static Imports (Recommended)

```vue
<script setup>
import { Repl as ReplEditor, useStore, File } from '@vue/repl'

const store = useStore()
</script>

<template>
  <ReplEditor :store="store" />
</template>
```

If your markdown page is named `repl.md`, use an alias like `ReplEditor` to avoid name collisions.

## Import Paths

```ts
// main entry: component + store utilities
import { Repl, useStore, File } from '@vue/repl'

// core-only entry: store utilities only
import { useStore, File } from '@vue/repl/core'

// codemirror editor component entry
import CodeMirrorEditor from '@vue/repl/codemirror-editor'
```

## Repl Props

`Repl` supports these props:

- `theme?: 'dark' | 'light'` (default: `'light'`)
- `store?: Store` (default: `useStore()`)
- `autoResize?: boolean` (default: `true`)
- `initialValue?: string` (default: `''`)
- `filename?: string` (default: `'text.txt'`)
- `editorOptions?: { autoSaveText?: string | false }` (default: `{}`)

`v-model` is also supported (`boolean`) and controls the internal auto-save toggle (default `true`).

Example:

```vue
<script setup>
import { ref } from 'vue'
import { Repl as ReplEditor, useStore } from '@vue/repl'

const store = useStore()
const autoSave = ref(true)
</script>

<template>
  <ReplEditor
    v-model="autoSave"
    :store="store"
    theme="light"
    :autoResize="false"
    filename="App.vue"
    initialValue="// hello"
    :editorOptions="{ autoSaveText: 'Sync' }"
  />
</template>
```

## Exposed Methods

`Repl` exposes:

- `getValue(): string`
- `setValue(code: string): void`

Example:

```vue
<script setup>
import { ref } from 'vue'
import { Repl as ReplEditor, useStore } from '@vue/repl'

const replRef = ref(null)
const store = useStore()

function dumpCode() {
  console.log(replRef.value?.getValue())
}
</script>

<template>
  <ReplEditor ref="replRef" :store="store" />
</template>
```

## Store API (`useStore`)

```ts
const store = useStore()
```

State:

- `store.files`
- `store.activeFile`
- `store.activeFilename`

Methods:

- `store.init()`
- `store.setActive(filename)`
- `store.addFile(fileOrFilename)`
- `store.deleteFile(filename)` (shows `confirm()`)
- `store.renameFile(oldFilename, newFilename)`
- `store.serialize()`
- `store.deserialize(hash)`

## Why `text.txt` Appears By Default

In this fork, `useStore()` starts with:

- `activeFilename = 'text.txt'`
- immediate default-file creation

So `text.txt` is always created unless you override/cleanup after creating the store.

Simple cleanup pattern:

```ts
import { File, useStore } from '@vue/repl'

const store = useStore()

delete store.files['text.txt']
store.addFile(new File('scene.flecs', 'Tag {}'))
store.setActive('scene.flecs')
```

## URL State

Use store serialization helpers for quick save/restore:

```ts
const hash = store.serialize()
store.deserialize(hash)
```

## SSR Note

This repo uses an SSR-safe `@vue/repl` package build (stub for Node SSR path), so static imports in markdown are safe for VitePress build.
