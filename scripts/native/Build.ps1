# SPDX-License-Identifier: MIT
# AI-Change: 2026-09-22-native-foundation (see docs/ai/changes/)
[CmdletBinding()]
param([ValidateSet('Release','Debug')][string]$Configuration = 'Release', [string]$QtPrefix, [switch]$SkipTests)
$ErrorActionPreference = 'Stop'
$root = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
if (!$QtPrefix) { $QtPrefix = Join-Path $root '.tools\Qt\6.8.3\mingw_64' }
$oldPath = $env:PATH
Push-Location $root
try {
    $env:PATH = "$root\.tools\Qt\Tools\mingw1310_64\bin;$QtPrefix\bin;$root\.tools\bootstrap\Scripts;$env:PATH"
    $preset = 'native-' + $Configuration.ToLowerInvariant()
    & cmake --preset $preset "-DCMAKE_PREFIX_PATH=$QtPrefix"
    if ($LASTEXITCODE) { throw 'CMake configure failed' }
    & cmake --build --preset $preset
    if ($LASTEXITCODE) { throw 'Native compile/link failed' }
    if (!$SkipTests) {
        & ctest --preset $preset
        if ($LASTEXITCODE) { throw 'Native regression suite failed; see out/<preset>/native/*tests.txt' }
    }
} finally { $env:PATH = $oldPath; Pop-Location }
