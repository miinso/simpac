---
title: Remote API Guide
---

# Remote API Guide

This page documents how to use the injected `window.flecs` client with worker-hosted WASM apps.
Examples assume you already mounted `<Simpac />` and have a `ref` named `appRef`.

## Connect

```js
const conn = window.flecs.connect(appRef.value);
// or connect to a remote host
const remote = window.flecs.connect('localhost');
```

## Query by tag

```js
// Query all entities with a tag
const reply = await conn.query('graphics.ActiveCamera', {
  values: true,
  full_paths: true
});

const results = reply?.results;
if (!results) {
  throw new Error('No results in reply');
}

for (const item of results) {
  console.log(item.name, item.tags);
}
```

## Query by tag + component values

```js
const reply = await conn.query('graphics.ActiveCamera, graphics.Camera, graphics.Position', {
  values: true,
  full_paths: true
});

const entity = reply?.results?.[0];
if (!entity) {
  throw new Error('No entities returned');
}
const camera = entity?.components?.['graphics.Camera'];
if (!camera) {
  throw new Error('Missing graphics.Camera');
}
const fovy = camera.fovy;
const pos = entity?.components?.['graphics.Position']; // { x, y, z }
const posX = pos?.x;
```

## Query by component

```js
const reply = await conn.query('graphics.Camera', { values: true, full_paths: true });
const results = reply?.results;
if (!results) {
  throw new Error('No results in reply');
}
for (const item of results) {
  const cam = item.components?.['graphics.Camera'];
  console.log(item.name, cam?.fovy);
}
```

## Entity and component reads

```js
// entity info
const entity = await conn.entity('MainCamera', { values: true });

// component read (recommended signature)
const camReply = await conn.get('MainCamera', {
  component: 'graphics.Camera',
  values: true
});

// component read (string form)
const camReply2 = await conn.get('MainCamera', 'graphics.Camera');
```

## Component updates

```js
// patch a component (Vector3f fields are { x, y, z })
await conn.set('MainCamera', 'graphics.Camera', {
  fovy: 75,
});
await conn.set('MainCamera', 'graphics.Position', {
  x: 5,
  y: 5,
  z: 5
});

// add/remove tags
await conn.add('MainCamera', 'graphics.ActiveCamera');
await conn.remove('MainCamera', 'graphics.ActiveCamera');

// enable/disable component
await conn.enable('MainCamera', 'graphics.Camera');
await conn.disable('MainCamera', 'graphics.Camera');
```

## Create/delete entities

```js
await conn.create('NewEntity');
await conn.delete('NewEntity');
```

## World stats and script

```js
const world = await conn.world();
await conn.scriptUpdate('main', 'Tag {}');
```

## Actions

```js
await conn.action('reset');
```

## Accessing response data

Common patterns for data access:

```js
// query responses: data typically lives under results[].components
const item = reply?.results?.[0];
const cam = item?.components?.['graphics.Camera'];
const posX = item?.components?.['graphics.Position']?.x;

// component reads: data might be under value/ComponentName (depending on server)
let value = camReply?.value;
if (!value) value = camReply?.Camera;
if (!value) value = camReply;
```

## Utilities

```js
window.flecs.escapePath('Parent.Child');
window.flecs.trimQuery('Position, Velocity');
```
