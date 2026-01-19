#!/usr/bin/env python3
import json
import os
import subprocess
import sys

from bazel_tools.tools.python.runfiles import runfiles


def _iter_manifest_entries(manifest_path):
    with open(manifest_path, "r", encoding="utf-8", newline="\n") as handle:
        for line in handle:
            line = line.rstrip("\n")
            if not line:
                continue
            if line.startswith(" "):
                escaped_link, escaped_target = line[1:].split(" ", maxsplit=1)
                link = (
                    escaped_link.replace(r"\s", " ")
                    .replace(r"\n", "\n")
                    .replace(r"\b", "\\")
                )
                target = (
                    escaped_target.replace(r"\n", "\n")
                    .replace(r"\b", "\\")
                )
            else:
                link, target = line.split(" ", maxsplit=1)
            if not target:
                target = link
            yield link, target


def _find_in_manifest(manifest_path, candidates):
    if not manifest_path or not os.path.exists(manifest_path):
        return None
    best = None
    best_rank = len(candidates)
    for link, target in _iter_manifest_entries(manifest_path):
        for rank, candidate in enumerate(candidates):
            if link == candidate or link.endswith("/" + candidate):
                if rank < best_rank:
                    best = target
                    best_rank = rank
                if best_rank == 0:
                    return best
    return best


def _resolve_runfile(path, r):
    if not path:
        return None
    if os.path.isabs(path) and os.path.exists(path):
        return path
    runfiles_dir = os.environ.get("RUNFILES_DIR")
    if runfiles_dir:
        candidate = os.path.join(runfiles_dir, path)
        if os.path.exists(candidate):
            return candidate
    if r:
        try_paths = [path]
        try:
            repo = r.CurrentRepository()
        except ValueError:
            repo = ""
        if repo:
            try_paths.append(repo + "/" + path)
        for candidate in try_paths:
            loc = r.Rlocation(candidate)
            if loc and os.path.exists(loc):
                return loc
    manifest = os.environ.get("RUNFILES_MANIFEST_FILE")
    candidates = [path, os.path.basename(path)]
    return _find_in_manifest(manifest, candidates)


def _find_js_from_args(args, r):
    files = []
    for arg in args:
        if not arg:
            continue
        files.extend(arg.split())
    for path in files:
        if path.endswith(".js"):
            resolved = _resolve_runfile(path, r)
            if resolved:
                return resolved
    return None


def _find_js_from_runfiles(r):
    if not r:
        return None
    return _resolve_runfile("modules/graphics_test.js", r)


def main():
    r = runfiles.Create()
    js_path = _find_js_from_args(sys.argv[1:], r)
    if not js_path:
        js_path = _find_js_from_runfiles(r)
    if not js_path or not os.path.exists(js_path):
        sys.stderr.write("graphics_test_node: missing graphics_test.js\n")
        return 1
    js_dir = os.path.dirname(js_path)
    node_script = """\
(async () => {
  const mod = require(%s);
  await mod();
})().catch((err) => {
  console.error(err && err.stack ? err.stack : err);
  process.exit(1);
});
""" % json.dumps(js_path)
    proc = subprocess.Popen(
        ["node", "-e", node_script],
        cwd=js_dir,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        bufsize=1,
    )
    if not proc.stdout:
        return proc.wait()
    with proc.stdout:
        for line in proc.stdout:
            sys.stdout.write(line)
            sys.stdout.flush()
    return proc.wait()


if __name__ == "__main__":
    raise SystemExit(main())
