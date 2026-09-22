# SPDX-License-Identifier: MIT
# AI-Change: 2026-09-22-resume-native (OpenAI / GPT-6 Astra Pro)
# Creates a new local development staging folder; never overwrites a prior package.
[CmdletBinding()]
param([string]$QtPrefix, [string]$OutputRoot)
$ErrorActionPreference = 'Stop'
$root = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
if (!$QtPrefix) { $QtPrefix = Join-Path $root '.tools\Qt\6.8.3\mingw_64' }
if (!$OutputRoot) { $OutputRoot = Join-Path $root 'release-local' }
$exe = Join-Path $root 'out\native-release\native\mterm.exe'
$deploy = Join-Path $QtPrefix 'bin\windeployqt.exe'
if (!(Test-Path $exe)) { throw 'Build native Release before packaging.' }
if (!(Test-Path $deploy)) { throw 'Qt deployment tool missing from QtPrefix.' }
$stamp = (Get-Date).ToUniversalTime().ToString('yyyyMMddTHHmmssZ')
$name = 'MTerm-native-preview-' + $stamp + '-' + [Guid]::NewGuid().ToString('N').Substring(0,8)
$target = Join-Path $OutputRoot $name
if (Test-Path $target) { throw 'Refusing to reuse an existing package directory.' }
New-Item -ItemType Directory -Path $target -Force | Out-Null
$oldPath = $env:PATH
try {
    $env:PATH = "$root\.tools\Qt\Tools\mingw1310_64\bin;$QtPrefix\bin;$env:PATH"
    Copy-Item -LiteralPath $exe -Destination (Join-Path $target 'mterm.exe')
    # Widgets/raster preview needs neither shader compilers nor network/SQL-server plugins.
    & $deploy --release --compiler-runtime --no-translations --no-opengl-sw --no-system-d3d-compiler --no-system-dxc-compiler --skip-plugin-types generic,networkinformation,tls --exclude-plugins qsqlmimer,qsqlodbc,qsqlpsql --dir $target (Join-Path $target 'mterm.exe')
    if ($LASTEXITCODE) { throw 'Qt runtime deployment failed; partial staging folder preserved.' }
} finally { $env:PATH = $oldPath }
Copy-Item -LiteralPath (Join-Path $root 'LICENSE') -Destination (Join-Path $target 'LICENSE-MTerm.txt')
Copy-Item -LiteralPath (Join-Path $root 'native\third_party\libvterm\LICENSE') -Destination (Join-Path $target 'LICENSE-libvterm.txt')
@'
MTerm native development preview: C++20 / Qt Widgets, not the Electron reference.
Run mterm.exe. Default data is separate from legacy AstraCommander storage.
Use --workspace PATH and --data-dir PATH for an explicit workspace/data location.
No browser or Node runtime is required. Optional model CLIs are installed separately.
This folder is unsigned local staging, NOT a public release or installer.
Qt dynamic-link redistribution/license/source obligations, compiler runtime notices,
signing, clean-machine installation and update safety require release review.
Do not publish this staging folder as a certified release.
'@ | Set-Content -LiteralPath (Join-Path $target 'PREVIEW-README.txt') -Encoding utf8
$files = @(Get-ChildItem -LiteralPath $target -File -Recurse)
$forbidden = $files | Where-Object { $_.Name -match '^(node|electron|QtWebEngineProcess)\.exe$|Qt6WebEngine.*\.dll$' }
if ($forbidden) { throw 'Unexpected heavyweight runtime in native staging.' }
$entries = @($files | ForEach-Object {
    [pscustomobject]@{ path=$_.FullName.Substring($target.Length+1).Replace('\','/'); bytes=$_.Length; sha256=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant() }
})
$commit = (& git -C $root rev-parse HEAD).Trim()
$dirty = [bool](& git -C $root status --porcelain --untracked-files=no)
[pscustomobject]@{ formatVersion=1; product='MTerm'; kind='unsigned-local-development-preview'; createdUtc=$stamp; sourceCommit=$commit; sourceDirty=$dirty; files=$entries; totalBytes=($files | Measure-Object Length -Sum).Sum } |
    ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $target 'package-manifest.json') -Encoding utf8
Write-Host "MTERM_PACKAGE=$target"
Write-Host "RUNTIME_FILES=$($files.Count) TOTAL_BYTES=$(($files | Measure-Object Length -Sum).Sum)"
