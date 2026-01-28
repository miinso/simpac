import { reactive } from 'vue';

export const defaultInteractiveClasses = [
  'inline-flex',
  'items-baseline',
  'align-baseline',
  'rounded',
  'px-0.5',
  'py-0.5',
  'cursor-pointer',
  'pointer-events-auto',
  'select-none',
  'transition',
  'duration-150',
  'hover:ring-2',
  'hover:ring-slate-300',
  'ring-offset-1',
  'ring-offset-white',
];

export function createTerms(defs) {
  const terms = reactive({});
  for (const [key, def] of Object.entries(defs)) {
    const term = { ...def };
    if (def.kind === 'toggle') {
      term.active = def.default ?? true;
    } else {
      term.value = def.default;
    }
    terms[key] = term;
  }
  return terms;
}

export function bindInteractiveMath({
  terms,
  defs,
  selector = '[data-term],[data-param]',
  classes = defaultInteractiveClasses,
  onToggle,
} = {}) {
  function ensureClasses(el, def) {
    if (el.dataset.bound === '1') return;
    el.dataset.bound = '1';
    classes.forEach((cls) => el.classList.add(cls));
    def?.classes?.forEach((cls) => el.classList.add(cls));
  }

  function sync() {
    if (typeof window === 'undefined') return;
    document.querySelectorAll(selector).forEach((el) => {
      const key = el.dataset.term || el.dataset.param;
      const term = terms?.[key];
      if (!term) return;
      ensureClasses(el, defs?.[key]);
      if (term.kind === 'toggle') {
        el.classList.toggle('opacity-40', !term.active);
      }
    });
  }

  function findTarget(event) {
    const direct = event.target?.closest?.(selector);
    if (direct) return direct;
    if (typeof document.elementsFromPoint !== 'function') return null;
    const stack = document.elementsFromPoint(event.clientX, event.clientY);
    return stack.find((el) => el.matches?.(selector)) || null;
  }

  function handleClick(event) {
    const target = findTarget(event);
    const key = target?.dataset.term;
    if (!key) return;
    const term = terms?.[key];
    if (!term || term.kind !== 'toggle') return;
    term.active = !term.active;
    if (onToggle) onToggle(key, term);
    sync();
  }

  function attach() {
    if (typeof window === 'undefined') return;
    sync();
    document.addEventListener('click', handleClick);
  }

  function detach() {
    if (typeof window === 'undefined') return;
    document.removeEventListener('click', handleClick);
  }

  return { attach, detach, sync };
}
