param(
    [int]$Port = 8000
)

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = Resolve-Path (Join-Path $scriptDir "..\\..")

Push-Location $repoRoot
try {
    & bazel build --config=wasm -c dbg //small/explorer:webapp
    if ($LASTEXITCODE -ne 0) {
        throw "Bazel build failed."
    }

    $binDir = Join-Path $repoRoot "bazel-bin\\small\\explorer"
    $outDir = $scriptDir
$files = @("explorer_demo.js", "explorer_demo.wasm")

    foreach ($file in $files) {
        $src = Join-Path $binDir $file
    if (Test-Path $src) {
        Copy-Item -Force $src $outDir
    } else {
        Write-Warning "Missing output (skipped): $src"
    }
}
}
finally {
    Pop-Location
}

$python = Get-Command python -ErrorAction SilentlyContinue
if (-not $python) {
    $python = Get-Command py -ErrorAction SilentlyContinue
}
if (-not $python) {
    throw "Python not found. Install Python or run a server manually."
}

& $python.Source (Join-Path $scriptDir "serve.py") $Port
