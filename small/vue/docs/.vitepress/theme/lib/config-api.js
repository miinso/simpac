import { reactive } from 'vue';
import { readComponentValue } from './engine-contract.js';

export function createConfigApi(conn) {
  async function get(entity, component, params = {}) {
    const reply = await conn.get(entity, {
      component,
      values: true,
      ...params,
    });
    return readComponentValue(reply, component);
  }

  async function set(entity, component, value, params = {}) {
    return conn.set(entity, component, value, params);
  }

  function bind(entity, component, initial = {}) {
    const fields = reactive({ ...initial });

    async function load(params = {}) {
      const value = await get(entity, component, params);
      if (value && typeof value === 'object') {
        Object.assign(fields, value);
      }
      return fields;
    }

    async function save(params = {}) {
      return set(entity, component, { ...fields }, params);
    }

    async function patch(partial, params = {}) {
      Object.assign(fields, partial || {});
      return save(params);
    }

    return {
      entity,
      component,
      fields,
      load,
      save,
      patch,
    };
  }

  return {
    get,
    set,
    bind,
  };
}
