export const DISABLED_TAG = 'flecs.core.Disabled';
export const SYSTEMS_QUERY = 'flecs.system.System, !ChildOf($this|up, flecs), ?Disabled';

export function encodeEntityPath(path) {
  return String(path || '').replace(/ /g, '%20');
}

export function pathFromResult(item) {
  if (!item) return '';
  return item.parent ? `${item.parent}.${item.name}` : (item.name || '');
}

export function normalizeSystem(item) {
  const path = pathFromResult(item);
  const disabled = item?.fields?.is_set?.[2] === true;
  return {
    parent: item?.parent || '',
    name: item?.name || '',
    path,
    disabled,
    enabled: !disabled,
    raw: item,
  };
}

export function readComponentValue(reply, component) {
  const leaf = String(component || '').split('.').pop();
  return reply?.value ?? reply?.[component] ?? reply?.[leaf] ?? reply;
}
