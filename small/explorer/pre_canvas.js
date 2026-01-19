// Ensure Module.canvas exists for environments that don't pass moduleArg.
if (typeof Module !== 'undefined' && !Module['canvas'] && typeof document !== 'undefined') {
  Module['canvas'] = document.getElementById('canvas');
}
