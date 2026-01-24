// minimal Flecs REST client for worker-hosted WASM apps.
// adapted from small/explorer/flecs.js (no connection manager / xhr).

function resolveRequestFn(source) {
  if (typeof source === 'function') return source;
  if (source && typeof source.request === 'function') {
    return source.request.bind(source);
  }
  if (source && typeof source.callCwrap === 'function') {
    return (method, path, params, body) => {
      const url = `${path}${buildQuery(params)}`;
      return source.callCwrap(
        'flecs_explorer_request',
        'string',
        ['string', 'string', 'string'],
        method,
        url,
        body || ''
      );
    };
  }
  if (source && typeof source.cwrap === 'function') {
    const nativeRequest = source.cwrap(
      'flecs_explorer_request',
      'string',
      ['string', 'string', 'string']
    );
    return (method, path, params, body) => {
      const url = `${path}${buildQuery(params)}`;
      return nativeRequest(method, url, body || '');
    };
  }
  throw new Error('flecs.connect requires a host, request function, or Module-like object');
}

function parseReply(reply) {
  if (reply === undefined || reply === null) return null;
  if (typeof reply !== 'string') return reply;
  const text = reply.trim();
  if (!text) return null;
  if (text[0] !== '{' && text[0] !== '[') return reply;
  try {
    return JSON.parse(text);
  } catch {
    return reply;
  }
}

function normalizeHost(host) {
  let url = String(host || '');
  if (!/^https?:\/\//i.test(url)) {
    url = `http://${url}`;
  }
  const protoIndex = url.indexOf('://');
  const portIndex = url.indexOf(':', protoIndex + 3);
  if (portIndex === -1) {
    url += ':27750';
  }
  return url.replace(/\/$/, '');
}

function trimQuery(query) {
  let result = String(query || '').replaceAll('\n', ' ');
  while (result.includes('  ')) {
    result = result.replaceAll('  ', ' ');
  }
  return result;
}

function sanitizeParams(params) {
  const result = {};
  if (!params) return result;
  for (const [key, value] of Object.entries(params)) {
    if (value === undefined) continue;
    if (key === 'poll_interval_ms' || key === 'host' || key === 'managed' || key === 'persist') {
      continue;
    }
    result[key] = value;
  }
  return result;
}

function buildQuery(params) {
  const clean = sanitizeParams(params);
  let count = 0;
  let qs = '';
  for (const [key, value] of Object.entries(clean)) {
    if (value === undefined) continue;
    qs += count ? '&' : '?';
    qs += `${key}=${value}`;
    count += 1;
  }
  return qs;
}

function escapePath(path) {
  let result = String(path || '');
  result = result.replaceAll('/', '%5C%2F');
  result = result.replaceAll('\\.', '@@');
  result = result.replaceAll('.', '/');
  result = result.replaceAll('@@', '.');
  result = result.replaceAll('#', '%23');
  return result;
}

function dispatch(promise, recv, err) {
  const wrapped = promise
    .then((reply) => {
      if (recv) recv(reply);
      return reply;
    })
    .catch((error) => {
      const message = error instanceof Error ? error.message : String(error);
      if (err) err(message);
      else console.error(message);
      throw error;
    });
  wrapped.abort = () => {};
  wrapped.cancel = () => {};
  wrapped.resume = () => {};
  wrapped.now = () => {};
  return wrapped;
}

function normalizeCallbacks(params, recv, err) {
  if (typeof params === 'function') {
    return { params: {}, recv: params, err: recv };
  }
  return { params: params || {}, recv, err };
}

function createConnection(requestFn, mode) {
  const encodeQuery = mode === 'remote';
  const send = (method, path, params = {}, body = '', recv, err) => {
    const promise = requestFn(method, path, sanitizeParams(params), body)
      .then(parseReply);
    return dispatch(promise, recv, err);
  };

  const withPath = (prefix, path) => `${prefix}${escapePath(path)}`;

  const conn = {
    mode,
    connect: () => conn,
    disconnect: () => {},
    request: (path, params, recv, err, abort) => {
      const args = normalizeCallbacks(params, recv, err);
      return send('GET', path, args.params, '', args.recv, args.err);
    },
    entity: (path, params, recv, err, abort) => {
      const args = normalizeCallbacks(params, recv, err);
      return send('GET', withPath('/entity/', path), args.params, '', args.recv, args.err);
    },
    query: (expr, params, recv, err, abort) => {
      if (expr === undefined || expr === null) {
        throw new Error('flecs.query: invalid query parameter');
      }
      const args = normalizeCallbacks(params, recv, err);
      const exprValue = trimQuery(String(expr)).replaceAll(', ', ',');
      const exprParam = encodeQuery ? encodeURIComponent(exprValue) : exprValue;
      return send(
        'GET',
        '/query',
        { ...args.params, expr: exprParam },
        '',
        args.recv,
        args.err
      );
    },
    queryName: (name, params, recv, err, abort) => {
      const args = normalizeCallbacks(params, recv, err);
      const nameValue = String(name);
      const nameParam = encodeQuery ? encodeURIComponent(nameValue) : nameValue;
      return send(
        'GET',
        '/query',
        { ...args.params, name: nameParam },
        '',
        args.recv,
        args.err
      );
    },
    get: (path, params, recv, err) => {
      if (typeof params === 'string') {
        return send('GET', withPath('/component/', path), { component: params }, '', recv, err);
      }
      const args = normalizeCallbacks(params, recv, err);
      const component = args.params.component;
      if (component !== undefined) delete args.params.component;
      return send(
        'GET',
        withPath('/component/', path),
        component ? { ...args.params, component } : args.params,
        '',
        args.recv,
        args.err
      );
    },
    set: (path, component, value, params, recv, err) => {
      let payload = value;
      if (typeof payload === 'object') {
        payload = JSON.stringify(payload);
        if (encodeQuery) {
          payload = encodeURIComponent(payload);
        }
      }
      const args = normalizeCallbacks(params, recv, err);
      return send(
        'PUT',
        withPath('/component/', path),
        { ...args.params, component, value: payload },
        '',
        args.recv,
        args.err
      );
    },
    add: (path, component, recv, err) =>
      send('PUT', withPath('/component/', path), { component }, '', recv, err),
    remove: (path, component, recv, err) =>
      send('DELETE', withPath('/component/', path), { component }, '', recv, err),
    enable: (path, component, recv, err) =>
      send('PUT', withPath('/toggle/', path), { enable: true, component }, '', recv, err),
    disable: (path, component, recv, err) =>
      send('PUT', withPath('/toggle/', path), { enable: false, component }, '', recv, err),
    create: (path, recv, err) =>
      send('PUT', withPath('/entity/', path), {}, '', recv, err),
    delete: (path, recv, err) =>
      send('DELETE', withPath('/entity/', path), {}, '', recv, err),
    world: (recv, err) =>
      send('GET', '/world', {}, '', recv, err),
    scriptUpdate: (path, code, params, recv, err) => {
      const args = normalizeCallbacks(params, recv, err);
      const body = code == null ? '' : String(code);
      return send(
        'PUT',
        withPath('/script/', path),
        { ...args.params },
        body,
        args.recv,
        args.err
      );
    },
    action: (action, recv, err) =>
      send('PUT', `/action/${action}`, {}, '', recv, err),
    escapePath,
    trimQuery
  };

  conn.component = (path, component, params, recv, err) =>
    conn.get(path, { ...(params || {}), component }, recv, err);
  conn.setComponent = conn.set;
  conn.addComponent = conn.add;
  conn.removeComponent = conn.remove;
  conn.createEntity = conn.create;
  conn.deleteEntity = conn.delete;

  return conn;
}

export function createFlecsClient(source) {
  const requestFn = resolveRequestFn(source);
  return createConnection(requestFn, 'wasm');
}

function createRemoteConnection(host) {
  const base = normalizeHost(host);
  const requestFn = async (method, path, params, body) => {
    const url = `${base}${path}${buildQuery(params)}`;
    const options = { method };
    if (body && method !== 'GET' && method !== 'HEAD') {
      options.headers = { 'Content-Type': 'text/plain' };
      options.body = body;
    }
    const response = await fetch(url, options);
    const text = await response.text();
    if (!response.ok) {
      throw new Error(text || `HTTP ${response.status}`);
    }
    return text;
  };
  return createConnection(requestFn, 'remote');
}

export const flecs = {
  connect(target) {
    if (typeof target === 'string') {
      return createRemoteConnection(target);
    }
    if (!target) {
      throw new Error('flecs.connect requires a host string or request source');
    }
    return createFlecsClient(target);
  },
  escapePath,
  trimQuery
};
