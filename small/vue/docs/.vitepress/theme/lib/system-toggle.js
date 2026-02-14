import { DISABLED_TAG, SYSTEMS_QUERY, encodeEntityPath, normalizeSystem } from './engine-contract.js';

export function createSystemApi(conn, options = {}) {
  const disabledTag = options.disabledTag || DISABLED_TAG;
  const systemsQuery = options.query || SYSTEMS_QUERY;

  async function list(params = {}) {
    const reply = await conn.query(systemsQuery, {
      full_paths: true,
      fields: true,
      values: false,
      ...params,
    });
    return (reply?.results || [])
      .map(normalizeSystem)
      .filter((row) => row.path)
      .sort((a, b) => a.path.localeCompare(b.path));
  }

  async function listMap(params = {}) {
    const rows = await list(params);
    return new Map(rows.map((row) => [row.path, row]));
  }

  async function setEnabled(path, enabled) {
    const target = encodeEntityPath(path);
    if (enabled) return conn.remove(target, disabledTag);
    return conn.add(target, disabledTag);
  }

  async function toggle(path, currentEnabled) {
    return setEnabled(path, !currentEnabled);
  }

  return {
    disabledTag,
    query: systemsQuery,
    list,
    listMap,
    setEnabled,
    toggle,
  };
}
