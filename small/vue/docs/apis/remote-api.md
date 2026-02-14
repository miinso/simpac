---
title: Remote API Guide
---

# Remote API Guide

This document mirrors the actual implementation in:

- `small/vue/docs/.vitepress/theme/components/remote.js`

## What You Get

```js
const { flecs } = window;
const conn = flecs.connect(target);
```

Top-level exports:

- `flecs.connect(target)`
- `flecs.escapePath(path)`
- `flecs.trimQuery(query)`

## Connect Targets

`flecs.connect(target)` accepts:

1. host string (remote HTTP mode)

```js
const conn = flecs.connect('localhost');      // -> http://localhost:27750
const conn2 = flecs.connect('127.0.0.1:27750');
const conn3 = flecs.connect('https://host');
```

2. request function

```js
const conn = flecs.connect((method, path, params, body) => Promise.resolve('{}'));
```

3. request-like object

- `{ request(method, path, params, body) }`
- `{ callCwrap(...) }` (`flecs_explorer_request`)
- `{ cwrap(...) }` (`flecs_explorer_request`)

This is why `flecs.connect(appRef.value)` works with `Simpac`.

## Connection Object

Every connection has:

- `mode`: `'wasm'` or `'remote'`
- `connect()`: returns itself
- `disconnect()`: no-op

Methods (all return Promise-like objects):

- `request(path, params?, recv?, err?)`
- `entity(path, params?, recv?, err?)`
- `query(expr, params?, recv?, err?)`
- `queryName(name, params?, recv?, err?)`
- `get(path, paramsOrComponent?, recv?, err?)`
- `set(path, component, value, params?, recv?, err?)`
- `add(path, component, recv?, err?)`
- `remove(path, component, recv?, err?)`
- `enable(path, component, recv?, err?)`
- `disable(path, component, recv?, err?)`
- `create(path, recv?, err?)`
- `delete(path, recv?, err?)`
- `world(recv?, err?)`
- `scriptUpdate(path, code, params?, recv?, err?)`
- `action(actionName, recv?, err?)`

Aliases:

- `component` -> `get`
- `setComponent` -> `set`
- `addComponent` -> `add`
- `removeComponent` -> `remove`
- `createEntity` -> `create`
- `deleteEntity` -> `delete`

## Promise + Callback Style

Both styles are supported:

```js
// Promise
const reply = await conn.query('Position', { values: true });

// Callback
conn.query('Position', { values: true }, (reply) => {
  console.log(reply);
}, (message) => {
  console.error(message);
});
```

Returned object includes no-op compatibility methods:

- `abort()`
- `cancel()`
- `resume()`
- `now()`

## Request/Parse Behavior

Reply handling:

- `null`/`undefined` -> `null`
- non-string -> returned as-is
- string starting with `{` or `[` -> `JSON.parse` attempted
- parse failure -> original string returned

## Path Handling Rules

Entity/component paths are transformed with `escapePath()`:

- `/` -> `%5C%2F`
- `.` -> `/` (but `\.` is preserved as literal dot)
- `#` -> `%23`

Examples:

```js
flecs.escapePath('Parent.Child');   // Parent/Child
flecs.escapePath('A\\.B.C');        // A.B/C
```

## Query/Param Rules

`query(expr, ...)`:

- `expr == null` throws (`flecs.query: invalid query parameter`)
- newlines -> spaces
- repeated spaces collapse
- `', '` -> `','`

Sanitized params (always removed if present):

- `poll_interval_ms`
- `host`
- `managed`
- `persist`

Params with `undefined` values are dropped.

## `remote` vs `wasm` mode differences

`mode === 'remote'` when `connect(stringHost)`:

- host normalization:
  - protocol missing -> `http://` prefix
  - port missing -> `:27750` append
  - trailing slash removed
- query expr/name are URI-encoded before send
- `set()` object payload is JSON stringified and URI-encoded

`mode === 'wasm'` when using function/request/cwrap/callCwrap source:

- requests are delegated to supplied source
- no remote host normalization

## Method Examples

### Query

```js
await conn.query('graphics.ActiveCamera, graphics.Camera', {
  values: true,
  full_paths: true
});

await conn.queryName('MySavedQuery', { values: true });
```

### Entity / Component Read

```js
await conn.entity('MainCamera', { values: true, full_paths: true });

// object form
await conn.get('MainCamera', { component: 'graphics.Camera', values: true });

// string shorthand
await conn.get('MainCamera', 'graphics.Camera');
```

### Component Write / Tag Ops

```js
await conn.set('MainCamera', 'graphics.Camera', { fovy: 75 });
await conn.set('MainCamera', 'graphics.Position', { x: 0, y: 5, z: 10 });

await conn.add('MainCamera', 'graphics.ActiveCamera');
await conn.remove('MainCamera', 'graphics.ActiveCamera');

await conn.enable('MainCamera', 'graphics.Camera');
await conn.disable('MainCamera', 'graphics.Camera');
```

### Entity / World / Script / Action

```js
await conn.create('NewEntity');
await conn.delete('NewEntity');

await conn.world();
await conn.scriptUpdate('SceneScript', 'Tag {}', { try: true });
await conn.action('my-action');
```

## Practical Response Access Pattern

```js
const reply = await conn.query('graphics.ActiveCamera, graphics.Position', {
  values: true,
  full_paths: true
});
const item = reply?.results?.[0];
const pos = item?.components?.['graphics.Position'];

const compReply = await conn.get('MainCamera', { component: 'graphics.Camera', values: true });
const cam = compReply?.value || compReply?.Camera || compReply;
```
