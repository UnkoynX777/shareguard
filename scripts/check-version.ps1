param(
  [string]$Expected = ""
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot

function Read-JsonVersion([string]$Path, [string]$Property) {
  $json = Get-Content -Raw -Path $Path | ConvertFrom-Json
  $value = $json.$Property
  if (-not $value) {
    throw "Missing $Property in $Path"
  }
  return [string]$value
}

$versions = [ordered]@{
  "extension/manifests/manifest.base.json" = Read-JsonVersion (Join-Path $root "extension\manifests\manifest.base.json") "version"
  "extension/package.json" = Read-JsonVersion (Join-Path $root "extension\package.json") "version"
  "native/CMakeLists.txt" = ([regex]::Match((Get-Content -Raw (Join-Path $root "native\CMakeLists.txt")), "VERSION\s+([0-9]+\.[0-9]+\.[0-9]+)")).Groups[1].Value
  "native/src/messaging/Protocol.hpp" = ([regex]::Match((Get-Content -Raw (Join-Path $root "native\src\messaging\Protocol.hpp")), 'kNativeVersion = "([0-9]+\.[0-9]+\.[0-9]+)"')).Groups[1].Value
  "installer/shareguard.iss" = ([regex]::Match((Get-Content -Raw (Join-Path $root "installer\shareguard.iss")), '#define AppVersion "([0-9]+\.[0-9]+\.[0-9]+)"')).Groups[1].Value
}

$distinct = @($versions.Values | Select-Object -Unique)
if ($distinct.Count -ne 1 -or [string]::IsNullOrWhiteSpace([string]$distinct[0])) {
  $versions.GetEnumerator() | ForEach-Object { Write-Output "$($_.Key)=$($_.Value)" }
  throw "Version files do not match."
}

$version = [string]$distinct[0]
if ($Expected -and $Expected -ne $version) {
  throw "Tag version $Expected does not match project version $version."
}

Write-Output $version
