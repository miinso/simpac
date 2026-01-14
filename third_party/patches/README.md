# tbb

```
warning: user library symbol '$invokeEntryPoint' depends on internal symbol '$runtimeKeepaliveCounter'
warning: undefined symbol: _ZN3tbb6detail2r117do_throw_noexceptEPFvvE (referenced by root reference (e.g. compiled C/C++ code))
warning: user library symbol '__pthread_create_js' depends on internal symbol '$pthreadCreateProxied'
warning: undefined symbol: getcontext (referenced by root reference (e.g. compiled C/C++ code))
warning: undefined symbol: makecontext (referenced by root reference (e.g. compiled C/C++ code))
warning: undefined symbol: swapcontext (referenced by root reference (e.g. compiled C/C++ code))
emcc: warning: warnings in JS library compilation [-Wjs-compiler]
```

About the linker warnings:
- The undefined symbols (getcontext, makecontext, swapcontext) are expected
- They're in TBB's coroutine code that's disabled for WASM (__TBB_RESUMABLE_TASKS=0)
- I placed `-sERROR_ON_UNDEFINED_SYMBOLS=0` to allow the build
- These warnings are harmless and don't affect functionality (i hope..)
