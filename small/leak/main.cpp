#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#if defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
#else
#define EMSCRIPTEN_KEEPALIVE
#endif

namespace {
std::vector<void*> g_blocks;
size_t g_bytes = 0;
}

extern "C" {
EMSCRIPTEN_KEEPALIVE void leak_hold(uint32_t bytes) {
    if (bytes == 0) {
        return;
    }
    void* ptr = std::malloc(bytes);
    if (!ptr) {
        return;
    }
    std::memset(ptr, 0xA5, bytes);
    g_blocks.push_back(ptr);
    g_bytes += bytes;
}

EMSCRIPTEN_KEEPALIVE void leak_clear() {
    for (void* ptr : g_blocks) {
        std::free(ptr);
    }
    g_blocks.clear();
    g_bytes = 0;
}

EMSCRIPTEN_KEEPALIVE uint32_t leak_held_bytes() {
    return static_cast<uint32_t>(g_bytes);
}

EMSCRIPTEN_KEEPALIVE uint32_t leak_block_count() {
    return static_cast<uint32_t>(g_blocks.size());
}

EMSCRIPTEN_KEEPALIVE void emscripten_shutdown() {
    leak_clear();
}
}

int main() {
#if defined(PLATFORM_WEB)
    std::puts("Leak test wasm module ready.");
#else
    std::puts("Leak test native app ready.");
#endif
    return 0;
}
