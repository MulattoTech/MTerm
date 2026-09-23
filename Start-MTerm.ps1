# SPDX-License-Identifier: MIT
# Modified: 2026-09-22-native-ux; active checkout launch.
# AI-Change: 2026-09-22-native-foundation (see docs/ai/changes/)
[CmdletBinding()]
param([string]$Workspace, [ValidateSet('Release','Debug')][string]$Configuration = 'Release', [switch]$UseLastWorkspace)
$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
# Development launch follows this checkout after relocation; the application data is not moved.
if (!$Workspace -and !$UseLastWorkspace) { $Workspace = $root }
if ($Workspace -and !(Test-Path -LiteralPath $Workspace -PathType Container)) { throw 'Workspace directory does not exist.' }
$exe = Join-Path $root ("out\native-" + $Configuration.ToLowerInvariant() + '\native\mterm.exe')
if (!(Test-Path $exe)) { throw 'Native executable missing. Run scripts\native\Bootstrap-Windows.ps1 then scripts\native\Build.ps1.' }
$oldPath = $env:PATH
try {
    $env:PATH = "$root\.tools\Qt\Tools\mingw1310_64\bin;$root\.tools\Qt\6.8.3\mingw_64\bin;$env:PATH"
    $options = @()
    if ($Workspace) { $options = @('--workspace', ('"' + $Workspace.Replace('"','') + '"')) }
    if ($options.Count) { Start-Process -FilePath $exe -ArgumentList $options -WorkingDirectory $root }
    else { Start-Process -FilePath $exe -WorkingDirectory $root }
} finally { $env:PATH = $oldPath }
