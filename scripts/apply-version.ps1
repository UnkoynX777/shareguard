param(
  [string]$Version = ""
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$versionFile = Join-Path $root "VERSION"
if (-not $Version) {
  $Version = (Get-Content -Raw $versionFile).Trim()
}
if ($Version -notmatch '^[0-9]+\.[0-9]+\.[0-9]+$') {
  throw "Version must look like 0.3.0. Got: $Version"
}

Set-Content -Encoding ascii -Path $versionFile -Value $Version -NoNewline
Add-Content -Encoding ascii -Path $versionFile -Value ""

function Write-Utf8([string]$Path, [string]$Value) {
  $Value = $Value.TrimStart([char]0xFEFF)
  $utf8 = New-Object System.Text.UTF8Encoding $false
  [System.IO.File]::WriteAllText($Path, $Value, $utf8)
}

$basePath = Join-Path $root "extension\manifests\manifest.base.json"
$base = [regex]::Replace((Get-Content -Raw $basePath), '"version":\s*"[0-9]+\.[0-9]+\.[0-9]+"', "`"version`": `"$Version`"")
Write-Utf8 $basePath $base

$packagePath = Join-Path $root "extension\package.json"
$package = [regex]::Replace((Get-Content -Raw $packagePath), '"version":\s*"[0-9]+\.[0-9]+\.[0-9]+"', "`"version`": `"$Version`"")
Write-Utf8 $packagePath $package

$cmakePath = Join-Path $root "native\CMakeLists.txt"
$cmake = [regex]::Replace((Get-Content -Raw $cmakePath), 'VERSION\s+[0-9]+\.[0-9]+\.[0-9]+', "VERSION $Version")
Write-Utf8 $cmakePath $cmake

$protocolPath = Join-Path $root "native\src\messaging\Protocol.hpp"
$protocol = [regex]::Replace((Get-Content -Raw $protocolPath), 'kNativeVersion = "[0-9]+\.[0-9]+\.[0-9]+"', "kNativeVersion = `"$Version`"")
Write-Utf8 $protocolPath $protocol

$issPath = Join-Path $root "installer\shareguard.iss"
$iss = [regex]::Replace((Get-Content -Raw $issPath), '#define AppVersion "[0-9]+\.[0-9]+\.[0-9]+"', "#define AppVersion `"$Version`"")
Write-Utf8 $issPath $iss

Write-Output $Version
