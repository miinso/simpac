#pragma once

#include <memory>
#include <string>

#if !defined(__EMSCRIPTEN__)
#include <cctype>
#include <fstream>
#include "tools/cpp/runfiles/runfiles.h"
using bazel::tools::cpp::runfiles::Runfiles;

#if defined(_WIN32)
extern "C" __declspec(dllimport) unsigned long __stdcall GetModuleFileNameA(void*, char*, unsigned long);
extern "C" __declspec(dllimport) unsigned long __stdcall GetFileAttributesA(const char*);
#elif defined(__linux__)
#include <unistd.h>
#include <limits.h>
#include <sys/stat.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#include <limits.h>
#include <sys/stat.h>
#endif
#endif

// NOTE: runfiles keeps this bazel-aware, not bazel-only (cwd/path fallback still works).
// TODO: check fs lib contenders (std::filesystem/ghc::filesystem/physfs) + watcher (filewatch/efsw/native).

namespace graphics {
namespace detail {
    inline bool initialized = false;
#if !defined(__EMSCRIPTEN__)
    inline std::unique_ptr<Runfiles> runfiles;
    inline std::string repo_prefix = "_main";
    inline std::string package_prefix;

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

    inline std::string normalize_slashes(std::string path) {
        for (char& ch : path) {
            if (ch == '\\') {
                ch = '/';
            }
        }
        return path;
    }

    inline bool is_absolute_path(const std::string& path) {
        if (path.empty()) return false;
        if (path[0] == '/') return true;
        if (path.size() >= 3 && std::isalpha(static_cast<unsigned char>(path[0])) &&
            path[1] == ':' && (path[2] == '\\' || path[2] == '/')) {
            return true;
        }
        if (path.size() >= 2 && path[0] == '\\' && path[1] == '\\') {
            return true;
        }
        return false;
    }

    inline bool path_exists(const std::string& path) {
        if (path.empty()) return false;
#if defined(_WIN32)
        return GetFileAttributesA(path.c_str()) != 0xFFFFFFFFu;
#else
        struct stat info;
        return stat(path.c_str(), &info) == 0;
#endif
    }

    inline std::string detect_repo_prefix_from_mapping() {
        if (!runfiles) return "";
        std::string mapping_path = runfiles->Rlocation("_repo_mapping");
        if (mapping_path.empty()) return "";
        std::ifstream mapping(mapping_path);
        if (!mapping.is_open()) return "";
        std::string line;
        while (std::getline(mapping, line)) {
            if (line.empty() || line[0] != ',') continue;
            size_t first = line.find(',');
            if (first == std::string::npos) continue;
            size_t second = line.find(',', first + 1);
            if (second == std::string::npos) continue;
            std::string target = line.substr(second + 1);
            if (!target.empty()) return target;
        }
        return "";
    }

    inline std::string detect_package_prefix(const std::string& exe_path,
                                             std::string* repo_from_path) {
        if (repo_from_path) repo_from_path->clear();
        if (exe_path.empty()) return "";

        std::string normalized = normalize_slashes(exe_path);
        size_t last_slash = normalized.find_last_of('/');
        if (last_slash == std::string::npos) return "";

        const std::string runfiles_marker = ".runfiles/";
        size_t runfiles_pos = normalized.rfind(runfiles_marker, last_slash);
        if (runfiles_pos != std::string::npos) {
            size_t repo_start = runfiles_pos + runfiles_marker.size();
            size_t repo_end = normalized.find('/', repo_start);
            if (repo_end != std::string::npos && repo_end < last_slash) {
                if (repo_from_path) {
                    *repo_from_path = normalized.substr(repo_start, repo_end - repo_start);
                }
                size_t pkg_start = repo_end + 1;
                if (last_slash > pkg_start) {
                    return normalized.substr(pkg_start, last_slash - pkg_start);
                }
            }
        }

        const std::string bin_marker = "/bazel-bin/";
        size_t bin_pos = normalized.rfind(bin_marker, last_slash);
        if (bin_pos != std::string::npos) {
            size_t pkg_start = bin_pos + bin_marker.size();
            if (last_slash > pkg_start) {
                return normalized.substr(pkg_start, last_slash - pkg_start);
            }
        }

        const std::string out_marker = "/bazel-out/";
        size_t out_pos = normalized.rfind(out_marker, last_slash);
        if (out_pos != std::string::npos) {
            size_t bin_dir = normalized.find("/bin/", out_pos + out_marker.size());
            if (bin_dir != std::string::npos && bin_dir < last_slash) {
                size_t pkg_start = bin_dir + 5;
                if (last_slash > pkg_start) {
                    return normalized.substr(pkg_start, last_slash - pkg_start);
                }
            }
        }

        return "";
    }

    inline std::string runfiles_path_for(const std::string& path) {
        if (repo_prefix.empty()) return path;
        if (path.rfind(repo_prefix + "/", 0) == 0) return path;
        return repo_prefix + "/" + path;
    }

    inline void init_once() {
        if (initialized) return;
        initialized = true;
        std::string err;
        auto exe = get_exe_path();
        runfiles.reset(Runfiles::CreateForTest(&err));
        if (!runfiles && !exe.empty()) {
            runfiles.reset(Runfiles::Create(exe, &err));
        }
        std::string repo_from_path;
        package_prefix = detect_package_prefix(exe, &repo_from_path);
        if (!repo_from_path.empty()) {
            repo_prefix = repo_from_path;
        }
        std::string repo_from_mapping = detect_repo_prefix_from_mapping();
        if (!repo_from_mapping.empty()) {
            repo_prefix = repo_from_mapping;
        }
    }
#endif
}

// Get the normalized absolute path for a resource (platform-aware)
[[nodiscard]] inline std::string normalized_path(const char* path) {
#if defined(__EMSCRIPTEN__)
    return path ? std::string(path) : std::string();
#else
    // resolve via runfiles, then package-local, then cwd fallback.
    if (!path) return std::string();
    std::string input(path);
    if (detail::is_absolute_path(input)) return input;
    detail::init_once();
    if (!detail::runfiles) return input;

    std::string direct = detail::runfiles->Rlocation(detail::runfiles_path_for(input));
    if (detail::path_exists(direct)) return direct;

    if (!detail::package_prefix.empty()) {
        std::string local = detail::runfiles->Rlocation(
            detail::runfiles_path_for(detail::package_prefix + "/" + input));
        if (detail::path_exists(local)) return local;
    }

    if (detail::path_exists(input)) return input;
    return direct.empty() ? input : direct;
#endif
}

// Short alias for convenience
[[nodiscard]] inline std::string npath(const char* path) {
    return normalized_path(path);
}

} // namespace graphics
