---
title: Remote API Guide
---

# Remote API Guide

Quick examples for the injected `window.flecs` client.

## Connect

```js
const conn = window.flecs.connect(appRef.value);
const remote = window.flecs.connect('localhost');
```

## Query by tag

```js
// Query all entities with a tag
const reply = await conn.query('graphics.ActiveCamera', {
  values: true,
  full_paths: true
});

for (const item of reply?.results || []) {
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
const camera = entity?.components?.['graphics.Camera'];
const pos = entity?.components?.['graphics.Position']; // { x, y, z }
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
await conn.set('MainCamera', 'graphics.Camera', { fovy: 75 });
await conn.set('MainCamera', 'graphics.Position', { x: 5, y: 5, z: 5 });

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

## Accessing response data

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
