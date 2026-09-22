# SPDX-License-Identifier: MIT
# AI-Change: 2026-09-22-native-foundation (see docs/ai/changes/)
[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$root = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
Push-Location $root
try {
    $python = Join-Path $root '.tools\bootstrap\Scripts\python.exe'
    if (!(Test-Path $python)) {
        & python -m venv .tools\bootstrap
        if ($LASTEXITCODE) { throw 'Python virtual environment creation failed' }
    }
    & $python -m pip install --disable-pip-version-check -r scripts\native\toolchain-requirements.txt
    if ($LASTEXITCODE) { throw 'Native development tool installation failed' }
    if (!(Test-Path '.tools\Qt\6.8.3\mingw_64\bin\Qt6Core.dll')) {
        & $python -m aqt install-qt windows desktop 6.8.3 win64_mingw --archives qtbase -O .tools\Qt
        if ($LASTEXITCODE) { throw 'Qt download/install failed' }
    }
    if (!(Test-Path '.tools\Qt\Tools\mingw1310_64\bin\g++.exe')) {
        & $python -m aqt install-tool windows desktop tools_mingw1310 -O .tools\Qt
        if ($LASTEXITCODE) { throw 'MinGW download/install failed' }
    }
    Write-Host 'Project-local native tools ready. Run scripts\native\Build.ps1.'
} finally { Pop-Location }
