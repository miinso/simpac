import { readComponentValue } from './engine-contract.js';

const defaultScriptUpdate = {
  try: true,
  check_file: true,
  save_file: false,
};

export function createScriptApi(conn) {
  async function get(name) {
    const reply = await conn.get(name, {
      component: 'flecs.script.Script',
    });
    const script = readComponentValue(reply, 'flecs.script.Script');
    return script?.code || '';
  }

  async function apply(name, code, params = {}) {
    return conn.scriptUpdate(name, String(code ?? ''), {
      ...defaultScriptUpdate,
      ...params,
    });
  }

  return {
    get,
    apply,
  };
}
