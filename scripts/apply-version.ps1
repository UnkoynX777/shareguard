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

$base = Get-Content -Raw (Join-Path $root "extension\manifests\manifest.base.json")
$base = [regex]::Replace($base, '"version":\s*"[0-9]+\.[0-9]+\.[0-9]+"', "`"version`": `"$Version`"")
Set-Content -Encoding utf8 -NoNewline -Path (Join-Path $root "extension\manifests\manifest.base.json") -Value $base

$package = Get-Content -Raw (Join-Path $root "extension\package.json")
$package = [regex]::Replace($package, '"version":\s*"[0-9]+\.[0-9]+\.[0-9]+"', "`"version`": `"$Version`"")
Set-Content -Encoding utf8 -NoNewline -Path (Join-Path $root "extension\package.json") -Value $package

$cmake = Get-Content -Raw (Join-Path $root "native\CMakeLists.txt")
$cmake = [regex]::Replace($cmake, 'VERSION\s+[0-9]+\.[0-9]+\.[0-9]+', "VERSION $Version")
Set-Content -Encoding utf8 -NoNewline -Path (Join-Path $root "native\CMakeLists.txt") -Value $cmake

$protocol = Get-Content -Raw (Join-Path $root "native\src\messaging\Protocol.hpp")
$protocol = [regex]::Replace($protocol, 'kNativeVersion = "[0-9]+\.[0-9]+\.[0-9]+"', "kNativeVersion = `"$Version`"")
Set-Content -Encoding utf8 -NoNewline -Path (Join-Path $root "native\src\messaging\Protocol.hpp") -Value $protocol

$iss = Get-Content -Raw (Join-Path $root "installer\shareguard.iss")
$iss = [regex]::Replace($iss, '#define AppVersion "[0-9]+\.[0-9]+\.[0-9]+"', "#define AppVersion `"$Version`"")
Set-Content -Encoding utf8 -NoNewline -Path (Join-Path $root "installer\shareguard.iss") -Value $iss

Write-Output $Version
