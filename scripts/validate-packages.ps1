$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$version = (& (Join-Path $PSScriptRoot "check-version.ps1")).Trim()
$iss = Get-Content -Raw (Join-Path $root "installer\shareguard.iss")
$extensionId = ([regex]::Match($iss, '#define ExtensionId "([^"]+)"')).Groups[1].Value
$firefoxId = ([regex]::Match($iss, '#define FirefoxId "([^"]+)"')).Groups[1].Value
if (-not $extensionId -or -not $firefoxId) { throw "Installer is missing extension IDs." }
if ($iss -notmatch 'Extension\\Chromium') { throw "Installer does not copy the Chromium extension." }
if ($iss -notmatch 'PrivilegesRequired=lowest') { throw "Installer must stay per-user." }
if ($iss -notmatch 'allowed_origins') { throw "Installer Chromium manifest is missing allowed_origins." }
if ($iss -notmatch 'allowed_extensions') { throw "Installer Firefox manifest is missing allowed_extensions." }

$devHost = Get-Content -Raw (Join-Path $root "scripts\install-native-host.ps1")
if ($devHost -notmatch [regex]::Escape($extensionId)) { throw "Dev host registration does not allow the Chromium extension ID." }
if ($devHost -notmatch [regex]::Escape($firefoxId)) { throw "Dev host registration does not allow the Firefox extension ID." }

function Assert-CleanTree([string]$Path) {
  $bad = Get-ChildItem $Path -Recurse -File | Where-Object {
    $_.Extension -in ".map", ".ts", ".tsx" -or $_.Name -eq ".env" -or $_.FullName -match "node_modules"
  }
  if ($bad) {
    throw "Package contains files that must not ship: $($bad.Name -join ', ')"
  }
  $text = Get-ChildItem $Path -Recurse -File | Where-Object { $_.Extension -in ".js", ".json", ".html", ".css" } | Get-Content -Raw
  if (($text -join "`n") -match "BEGIN PRIVATE KEY|AMO_JWT_SECRET|api_secret") {
    throw "Package contains credential material."
  }
}

$chromium = Join-Path $root "extension\dist\chromium"
$chromiumManifestPath = Join-Path $chromium "manifest.json"
if (-not (Test-Path $chromiumManifestPath)) { throw "Missing Chromium manifest." }
$chromiumManifest = Get-Content -Raw $chromiumManifestPath | ConvertFrom-Json
if ($chromiumManifest.version -ne $version) { throw "Chromium manifest version is $($chromiumManifest.version), expected $version." }
$key = [string]$chromiumManifest.key
if (-not $key) { throw "Chromium manifest is missing the public key." }
$actualId = (& node (Join-Path $root "extension\scripts\extension-id.mjs") $key).Trim()
if ($actualId -ne $extensionId) {
  throw "Chromium extension ID $actualId does not match Native Messaging origin $extensionId."
}
Assert-CleanTree $chromium

$firefox = Join-Path $root "extension\dist\firefox"
$firefoxManifestPath = Join-Path $firefox "manifest.json"
if (-not (Test-Path $firefoxManifestPath)) { throw "Missing Firefox manifest." }
$firefoxManifest = Get-Content -Raw $firefoxManifestPath | ConvertFrom-Json
if ($firefoxManifest.version -ne $version) { throw "Firefox manifest version is $($firefoxManifest.version), expected $version." }
$gecko = [string]$firefoxManifest.browser_specific_settings.gecko.id
if ($gecko -ne $firefoxId) {
  throw "Firefox ID $gecko does not match Native Messaging allowed_extensions $firefoxId."
}
Assert-CleanTree $firefox

$zip = Join-Path $root "release\ShareGuard-Chromium-v$version.zip"
if (Test-Path $zip) {
  $check = Join-Path ([System.IO.Path]::GetTempPath()) "shareguard-zip-check"
  if (Test-Path $check) { Remove-Item $check -Recurse -Force }
  Expand-Archive -Path $zip -DestinationPath $check -Force
  $packed = Join-Path $check "ShareGuard-Chromium\manifest.json"
  if (-not (Test-Path $packed)) { throw "Chromium zip is missing ShareGuard-Chromium\manifest.json" }
  $packedManifest = Get-Content -Raw $packed | ConvertFrom-Json
  if ($packedManifest.version -ne $version) { throw "Chromium zip version does not match $version." }
  Remove-Item $check -Recurse -Force
}

$unsignedName = Join-Path $root "release\ShareGuard-Firefox-v$version.xpi"
if (Test-Path $unsignedName) {
  Add-Type -AssemblyName System.IO.Compression.FileSystem
  $archive = [System.IO.Compression.ZipFile]::OpenRead($unsignedName)
  try {
    $names = @($archive.Entries | ForEach-Object { $_.FullName })
  } finally {
    $archive.Dispose()
  }
  if (-not ($names -match "^META-INF/")) {
    throw "ShareGuard-Firefox-v$version.xpi exists but is not a Mozilla-signed package."
  }
}

Write-Output "Packages match version $version, Chromium ID $extensionId, Firefox ID $firefoxId."
