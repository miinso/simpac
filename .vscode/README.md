# VS Code + Bazel Integration

F5 debugging for any Bazel C++ target, regardless of source file location.

Works on **Windows**, **Linux**, and **macOS**.

## How It Works

### The Problem

VS Code's `launch.json` needs a static `program` path, but:
- Source files can be anywhere (e.g., `boids/boids.cpp`)
- Target might be in a parent package (e.g., `//homeworks:boids`)
- Bazel outputs to `bazel-out/<config>/bin/...`
- VS Code evaluates variables BEFORE `preLaunchTask` runs

### The Solution

1. **Find target from source** via `bazel query`:
   ```sh
   bazel query "kind(cc_binary, rdeps(//..., <source_file>))"
   ```
   Finds any `cc_binary` that includes this source as a dependency.

2. **Get exe path** via `bazel cquery`:
   ```sh
   bazel cquery //pkg:target --output=files
   ```
   Returns actual output path like `bazel-out/x64_windows-fastbuild/bin/.../target.exe`

3. **Copy to fixed location** (`.vscode/bin/debug[.exe]`):
   - launch.json always points here
   - No dynamic path resolution needed
   - No extensions required

4. **Copy runfiles** if they exist:
   - Data dependencies in `<exe>.runfiles/`
   - Copied alongside exe for runtime access

## Usage

### F5 Debug
Press F5 on any `.cpp` file. It will:
1. Find the `cc_binary` target containing that file
2. Build the target
3. Copy exe + runfiles to `.vscode/bin/`
4. Launch debugger

### Tasks (Ctrl+Shift+B)

| Task | Description |
|------|-------------|
| `bazel-run` | Find target, build, run via `bazel run` |
| `bazel-build` | Find target, build only |
| `bazel-build-debug` | Build, copy exe+runfiles to `.vscode/bin/` |
| `bazel-run-debug` | Build, run exe directly (no debugger) |

## Platform Support

| Platform | Shell | Debugger |
|----------|-------|----------|
| Windows | PowerShell | cppvsdbg (VS debugger) |
| Linux | bash | gdb |
| macOS | bash | lldb |

## Files

```
.vscode/
├── tasks.json      # Build tasks (bazel query + cquery)
├── launch.json     # Debug config → .vscode/bin/debug
├── bin/            # (generated, gitignored)
│   ├── debug[.exe]
│   └── debug.runfiles/
└── README.md
```

## Key Insight

The magic is using Bazel's own tools to resolve paths:

```sh
# Find which target builds a source file
bazel query "kind(cc_binary, rdeps(//..., path/to/file.cpp))"
# → //package:target

# Get the actual output file path
bazel cquery //package:target --output=files
# → bazel-out/.../bin/package/target[.exe]
```

This works because `rdeps(//..., file)` finds all targets that transitively depend on the file, and `kind(cc_binary, ...)` filters to only executable targets.

## Requirements

- Bazel in PATH
- No VS Code extensions needed
