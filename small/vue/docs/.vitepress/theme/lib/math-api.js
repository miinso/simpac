import { bindInteractiveMath, createTerms } from '../components/imath.js';

export function createMathApi() {
  function bind(options = {}) {
    return bindInteractiveMath(options);
  }

  return {
    createTerms,
    bind,
  };
}

export function bindSystemToggles({ terms, defs, systems, onError } = {}) {
  return async (key, term) => {
    const systemPath = defs?.[key]?.bind?.system;
    if (!systemPath || !systems) return;
    const next = !!term?.active;
    try {
      await systems.setEnabled(systemPath, next);
    } catch (error) {
      if (term && term.kind === 'toggle') term.active = !next;
      if (onError) onError(error, key, term);
    }
  };
}
