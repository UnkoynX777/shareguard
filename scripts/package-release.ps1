$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$version = (& (Join-Path $PSScriptRoot "check-version.ps1")).Trim()
$source = Join-Path $root "extension\dist\chromium"
$manifest = Join-Path $source "manifest.json"
if (-not (Test-Path $manifest)) {
  throw "Build the Chromium extension first. Missing $manifest"
}

$stage = Join-Path ([System.IO.Path]::GetTempPath()) "shareguard-chromium-stage"
if (Test-Path $stage) { Remove-Item $stage -Recurse -Force }
$folder = Join-Path $stage "ShareGuard-Chromium"
New-Item -ItemType Directory -Force -Path $folder | Out-Null
Copy-Item (Join-Path $source "*") $folder -Recurse -Force

$release = Join-Path $root "release"
New-Item -ItemType Directory -Force -Path $release | Out-Null
$zip = Join-Path $release "ShareGuard-Chromium-v$version.zip"
if (Test-Path $zip) { Remove-Item $zip -Force }
Compress-Archive -Path $folder -DestinationPath $zip -Force

$check = Join-Path $stage "check"
New-Item -ItemType Directory -Force -Path $check | Out-Null
Expand-Archive -Path $zip -DestinationPath $check -Force
$packed = Join-Path $check "ShareGuard-Chromium\manifest.json"
if (-not (Test-Path $packed)) {
  throw "Chromium zip must contain ShareGuard-Chromium\manifest.json"
}

Remove-Item $stage -Recurse -Force
Write-Output $zip
