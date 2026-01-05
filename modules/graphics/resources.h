#pragma once

#include <memory>
#include <string>

#if !defined(__EMSCRIPTEN__)
#include "tools/cpp/runfiles/runfiles.h"
using bazel::tools::cpp::runfiles::Runfiles;

#if defined(_WIN32)
extern "C" __declspec(dllimport) unsigned long __stdcall GetModuleFileNameA(void*, char*, unsigned long);
#elif defined(__linux__)
#include <unistd.h>
#include <limits.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#include <limits.h>
#endif
#endif

namespace graphics {
namespace detail {
    inline bool initialized = false;
#if !defined(__EMSCRIPTEN__)
    inline std::unique_ptr<Runfiles> runfiles;

    inline std::string get_exe_path() {
#if defined(_WIN32)
        char path[260];
        unsigned long len = GetModuleFileNameA(nullptr, path, 260);
        return (len > 0 && len < 260) ? std::string(path) : "";
#elif defined(__linux__)
        char path[PATH_MAX];
        ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
        if (len != -1) { path[len] = '\0'; return path; }
        return "";
#elif defined(__APPLE__)
        char path[PATH_MAX];
        uint32_t size = sizeof(path);
        return (_NSGetExecutablePath(path, &size) == 0) ? path : "";
#endif
    }

    inline void init_once() {
        if (initialized) return;
        initialized = true;
        std::string err;
        runfiles.reset(Runfiles::CreateForTest(&err));
        if (!runfiles) {
            auto exe = get_exe_path();
            if (!exe.empty()) runfiles.reset(Runfiles::Create(exe, &err));
        }
    }
#endif
}

// Get the normalized absolute path for a resource (platform-aware)
[[nodiscard]] inline std::string normalized_path(const char* path) {
#if defined(__EMSCRIPTEN__)
    return std::string(path);
#else
    detail::init_once();
    if (!detail::runfiles) return std::string(path);
    
    return detail::runfiles->Rlocation(std::string("_main/") + path);
#endif
}

// Short alias for convenience
[[nodiscard]] inline std::string npath(const char* path) {
    return normalized_path(path);
}

} // namespace graphics
