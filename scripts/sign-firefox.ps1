$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$version = (& (Join-Path $PSScriptRoot "check-version.ps1")).Trim()
$source = Join-Path $root "extension\dist\firefox"
if (-not (Test-Path (Join-Path $source "manifest.json"))) {
  throw "Build the Firefox extension before signing."
}

function Publish-Output([string]$Name, [string]$Value) {
  if ($env:GITHUB_OUTPUT) {
    "$Name=$Value" | Out-File -FilePath $env:GITHUB_OUTPUT -Append -Encoding utf8
  }
}

$hasIssuer = -not [string]::IsNullOrWhiteSpace($env:AMO_JWT_ISSUER)
$hasSecret = -not [string]::IsNullOrWhiteSpace($env:AMO_JWT_SECRET)
if (-not $hasIssuer -and -not $hasSecret) {
  Publish-Output "firefox" "omitted"
  Write-Output "Firefox user build omitted. AMO_JWT_ISSUER and AMO_JWT_SECRET are not set."
  exit 0
}
if (-not $hasIssuer -or -not $hasSecret) {
  throw "AMO signing secrets are incomplete. Set both AMO_JWT_ISSUER and AMO_JWT_SECRET, or neither."
}

$artifacts = Join-Path $root "release\amo"
if (Test-Path $artifacts) { Remove-Item $artifacts -Recurse -Force }
New-Item -ItemType Directory -Force -Path $artifacts | Out-Null

Push-Location (Join-Path $root "extension")
try {
  & npx --no-install web-ext sign --channel unlisted --source-dir "dist\firefox" --artifacts-dir $artifacts
  if ($LASTEXITCODE -ne 0) { throw "web-ext sign failed." }
} finally {
  Pop-Location
}

$signed = Get-ChildItem $artifacts -Filter *.xpi | Select-Object -First 1
if (-not $signed) { throw "web-ext sign did not produce an XPI." }

Add-Type -AssemblyName System.IO.Compression.FileSystem
$archive = [System.IO.Compression.ZipFile]::OpenRead($signed.FullName)
try {
  $hasSignature = $false
  $manifestEntry = $null
  foreach ($entry in $archive.Entries) {
    if ($entry.FullName -like "META-INF/*") { $hasSignature = $true }
    if ($entry.FullName -eq "manifest.json") { $manifestEntry = $entry }
  }
  if (-not $hasSignature) { throw "Signed XPI is missing META-INF." }
  if (-not $manifestEntry) { throw "Signed XPI is missing manifest.json." }
  $reader = New-Object System.IO.StreamReader($manifestEntry.Open())
  try { $manifestText = $reader.ReadToEnd() } finally { $reader.Dispose() }
} finally {
  $archive.Dispose()
}

$manifest = $manifestText | ConvertFrom-Json
if ($manifest.version -ne $version) { throw "Signed XPI version is $($manifest.version), expected $version." }
$iss = Get-Content -Raw (Join-Path $root "installer\shareguard.iss")
$firefoxId = ([regex]::Match($iss, '#define FirefoxId "([^"]+)"')).Groups[1].Value
if ([string]$manifest.browser_specific_settings.gecko.id -ne $firefoxId) {
  throw "Signed XPI Firefox ID does not match allowed_extensions."
}

$release = Join-Path $root "release"
New-Item -ItemType Directory -Force -Path $release | Out-Null
$destination = Join-Path $release "ShareGuard-Firefox-v$version.xpi"
Copy-Item $signed.FullName $destination -Force
Publish-Output "firefox" "signed"
Write-Output $destination
