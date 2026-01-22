<script setup lang="ts">
import { ref, computed, onMounted, onBeforeUnmount } from 'vue'
import { withBase } from 'vitepress'
import { SimpacWorker } from './worker-bridge.js'

const props = withDefaults(defineProps<{
  src: string
  wasmUrl?: string | null
  aspectRatio?: string
  maxHeight?: number
  canvasClass?: string
  pixelRatio?: number
  autoStart?: boolean
  exports?: string[] | null
  cwrap?: string[] | null
  moduleFactoryName?: string | null
  debug?: boolean
}>(), {
  aspectRatio: '16:9',
  maxHeight: 600,
  canvasClass: '',
  autoStart: true,
  wasmUrl: null,
  exports: () => ['_emscripten_resize'],
  cwrap: null,
  moduleFactoryName: 'createModule',
  debug: false
})

const emit = defineEmits<{
  (e: 'ready'): void
  (e: 'error', message: string): void
}>()

const canvasRef = ref<HTMLCanvasElement | null>(null)
const containerRef = ref<HTMLDivElement | null>(null)
const status = ref('Loading...')
const isLoading = ref(true)
const isReady = ref(false)
const showStatus = computed(() =>
  isLoading.value ||
  status.value.startsWith('Error') ||
  status.value === 'OffscreenCanvas not supported'
)

let worker: any = null
let resizeObserver: ResizeObserver | null = null
let windowResizeHandler: (() => void) | null = null
let canvasTransferred = false
let isUnmounted = false

const isClient = typeof window !== 'undefined'

const effectivePixelRatio = computed(() => {
  if (props.pixelRatio !== undefined) {
    return props.pixelRatio
  }
  if (!isClient) return 1

  const ua = navigator.userAgent || ''
  const isMobile = /Android|webOS|iPhone|iPad|iPod|BlackBerry|IEMobile|Opera Mini/i.test(ua)
  if (isMobile) {
    let effective = window.devicePixelRatio || 1
    while (effective >= 3) {
      effective /= 2
    }
    return effective
  }
  return 1
})

const isDesktop = computed(() => {
  if (!isClient) return true
  const ua = navigator.userAgent || ''
  return !/Android|webOS|iPhone|iPad|iPod|BlackBerry|IEMobile|Opera Mini/i.test(ua)
})

const containerStyle = computed(() => {
  const [w, h] = props.aspectRatio.split(':').map(Number)
  return {
    width: '100%',
    aspectRatio: `${w}/${h}`,
    maxHeight: `${props.maxHeight}px`
  }
})

function resolveUrl(input: string | null | undefined) {
  if (!input) return null
  if (!isClient) return input
  const value = input.trim()
  if (!value) return null

  if (/^[a-zA-Z][a-zA-Z\d+\-.]*:/.test(value) || value.startsWith('//')) {
    return value
  }
  if (value.startsWith('/')) {
    const basePath = withBase(value)
    return new URL(basePath, window.location.origin).toString()
  }
  return new URL(value, window.location.href).toString()
}

function computeCanvasSize() {
  const container = containerRef.value
  if (!container) return { width: 1, height: 1 }
  const dpr = effectivePixelRatio.value || 1
  const width = Math.max(1, Math.floor(container.clientWidth * dpr))
  const height = Math.max(1, Math.floor(container.clientHeight * dpr))
  return { width, height }
}

function syncCanvasSize(notifyWorker = false) {
  const canvas = canvasRef.value
  if (!canvas) return
  const { width, height } = computeCanvasSize()

  if (!canvasTransferred && (canvas.width !== width || canvas.height !== height)) {
    canvas.width = width
    canvas.height = height
  }

  if (notifyWorker && worker && worker.ready) {
    worker.call('_emscripten_resize', width, height).catch(() => {})
  }
}

function attachResizeObservers() {
  const container = containerRef.value
  if (!container || !isClient) return

  if ('ResizeObserver' in window) {
    resizeObserver = new ResizeObserver(() => syncCanvasSize(true))
    resizeObserver.observe(container)
  }

  windowResizeHandler = () => syncCanvasSize(true)
  window.addEventListener('resize', windowResizeHandler)
}

function detachResizeObservers() {
  resizeObserver?.disconnect()
  resizeObserver = null
  if (windowResizeHandler) {
    window.removeEventListener('resize', windowResizeHandler)
    windowResizeHandler = null
  }
}

function start() {
  if (worker || isUnmounted) return
  const canvas = canvasRef.value
  const container = containerRef.value
  if (!canvas || !container) return

  const resolvedSrc = resolveUrl(props.src)
  const resolvedWasmUrl = resolveUrl(props.wasmUrl ?? null)
  if (!resolvedSrc) {
    status.value = 'Missing app src'
    isLoading.value = false
    emit('error', status.value)
    return
  }

  if (!('transferControlToOffscreen' in HTMLCanvasElement.prototype)) {
    status.value = 'OffscreenCanvas not supported'
    isLoading.value = false
    emit('error', status.value)
    return
  }

  syncCanvasSize(false)
  status.value = 'Loading...'
  isLoading.value = true

  worker = new SimpacWorker(canvas, resolvedWasmUrl, resolvedSrc, {
    exports: props.exports ?? null,
    cwrap: props.cwrap ?? null,
    allowContextMenu: true,
    webglContextAttributes: isDesktop.value ? { preserveDrawingBuffer: true } : null,
    moduleFactoryName: props.moduleFactoryName === undefined ? 'createModule' : props.moduleFactoryName,
    onReady: () => {
      isReady.value = true
      isLoading.value = false
      status.value = 'Ready'
      emit('ready')
      syncCanvasSize(true)
    },
    onError: (message, detail) => {
      const text = detail ? `${message} ${detail}` : message
      status.value = `Error: ${text}`
      isLoading.value = false
      emit('error', text)
      if (props.debug) {
        console.error('[Simpac]', text)
      }
    },
    onLog: props.debug ? (text) => console.log('[WASM]', text) : null
  })

  worker.start()
  canvasTransferred = true
  attachResizeObservers()
}

function shutdown() {
  if (!worker) return
  worker.shutdown()
}

function terminate() {
  if (!worker) return
  worker.terminate()
  worker = null
  isReady.value = false
  isLoading.value = false
  status.value = 'Stopped'
}

function call(fn: string, ...args: any[]) {
  if (!worker || !worker.ready) {
    return Promise.reject(new Error('worker not ready'))
  }
  return worker.call(fn, ...args)
}

function callCwrap(name: string, returnType: string, argTypes: string[], ...args: any[]) {
  if (!worker || !worker.ready) {
    return Promise.reject(new Error('worker not ready'))
  }
  return worker.callCwrap(name, returnType, argTypes, ...args)
}

function readFile(path: string, encoding = 'utf8') {
  if (!worker || !worker.ready) {
    return Promise.reject(new Error('worker not ready'))
  }
  return worker.fsRead(path, encoding)
}

function writeFile(path: string, contents: string, encoding = 'utf8') {
  if (!worker || !worker.ready) {
    return Promise.reject(new Error('worker not ready'))
  }
  return worker.fsWrite(path, contents, encoding)
}

function request(method: string, path: string, params: Record<string, any> = {}, body = '') {
  const query = new URLSearchParams(params).toString()
  const url = query ? `${path}?${query}` : path
  return callCwrap('flecs_explorer_request', 'string', ['string', 'string', 'string'], method, url, body)
    .then((reply: string) => {
      if (!reply) return null
      try {
        return JSON.parse(reply)
      } catch {
        return reply
      }
    })
}

defineExpose({
  start,
  shutdown,
  terminate,
  call,
  callCwrap,
  readFile,
  writeFile,
  request,
  isReady
})

onMounted(() => {
  isUnmounted = false
  if (props.autoStart) {
    start()
  } else {
    status.value = 'Idle'
    isLoading.value = false
  }
})

onBeforeUnmount(() => {
  isUnmounted = true
  detachResizeObservers()
  if (worker) {
    worker.terminate()
    worker = null
  }
})
</script>

<template>
  <div ref="containerRef" class="simpac-container" :style="containerStyle">
    <canvas ref="canvasRef" class="simpac-canvas" :class="canvasClass" />
    <div v-if="showStatus" class="simpac-status">{{ status }}</div>
  </div>
</template>

<style scoped>
.simpac-container {
  position: relative;
  background: #1a1a1a;
  border-radius: 8px;
  overflow: hidden;
}
.simpac-canvas {
  display: block;
  width: 100%;
  height: 100%;
  background: #f5f5f5;
}
.simpac-status {
  position: absolute;
  top: 8px;
  left: 8px;
  color: #fff;
  font-size: 12px;
  background: rgba(0, 0, 0, 0.7);
  padding: 4px 8px;
  border-radius: 4px;
}
</style>
