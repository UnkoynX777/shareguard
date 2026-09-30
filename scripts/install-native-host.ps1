$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$exe = Join-Path $root "native\build\Release\shareguard-native.exe"
if (-not (Test-Path $exe)) {
  throw "Build the native helper first: scripts\build-native.ps1"
}

$extensionId = "bdkcdhphggeglifemnakdlcfbhcoempk"
$firefoxId = "shareguard@shareguard.local"
$dir = Join-Path $env:LOCALAPPDATA "ShareGuard"
New-Item -ItemType Directory -Force -Path $dir | Out-Null
$chromiumManifest = Join-Path $dir "com.shareguard.native.chromium.json"
$firefoxManifest = Join-Path $dir "com.shareguard.native.firefox.json"
$escapedExe = $exe.Replace("\", "\\")
$utf8 = New-Object System.Text.UTF8Encoding $false

$chromiumJson = @"
{
  "name": "com.shareguard.native",
  "description": "ShareGuard native audio helper",
  "path": "$escapedExe",
  "type": "stdio",
  "allowed_origins": [
    "chrome-extension://$extensionId/"
  ]
}
"@
$firefoxJson = @"
{
  "name": "com.shareguard.native",
  "description": "ShareGuard native audio helper",
  "path": "$escapedExe",
  "type": "stdio",
  "allowed_extensions": [
    "$firefoxId"
  ]
}
"@
[System.IO.File]::WriteAllText($chromiumManifest, $chromiumJson.Trim() + "`n", $utf8)
[System.IO.File]::WriteAllText($firefoxManifest, $firefoxJson.Trim() + "`n", $utf8)
Remove-Item (Join-Path $dir "com.shareguard.native.json") -Force -ErrorAction SilentlyContinue

function Register-Host([string]$registryPath, [string]$manifestPath) {
  New-Item -Path $registryPath -Force | Out-Null
  Set-Item -Path $registryPath -Value $manifestPath
}

function Report-Browser([string]$name, [string[]]$paths) {
  foreach ($path in $paths) {
    if (Test-Path $path) {
      Write-Output "$name detected"
      return
    }
  }
  Write-Output "$name not detected; native host still registered"
}

$programFiles32 = ${env:ProgramFiles(x86)}
Report-Browser "Google Chrome" @(
  (Join-Path $env:ProgramFiles "Google\Chrome\Application\chrome.exe"),
  (Join-Path $programFiles32 "Google\Chrome\Application\chrome.exe")
)
Report-Browser "Microsoft Edge" @(
  (Join-Path $programFiles32 "Microsoft\Edge\Application\msedge.exe"),
  (Join-Path $env:ProgramFiles "Microsoft\Edge\Application\msedge.exe")
)
Report-Browser "Mozilla Firefox" @(
  (Join-Path $env:ProgramFiles "Mozilla Firefox\firefox.exe"),
  (Join-Path $programFiles32 "Mozilla Firefox\firefox.exe")
)

Register-Host "HKCU:\Software\Google\Chrome\NativeMessagingHosts\com.shareguard.native" $chromiumManifest
Register-Host "HKCU:\Software\Microsoft\Edge\NativeMessagingHosts\com.shareguard.native" $chromiumManifest
Register-Host "HKCU:\Software\Mozilla\NativeMessagingHosts\com.shareguard.native" $firefoxManifest
Write-Output "Registered $chromiumManifest"
Write-Output "Registered $firefoxManifest"
Write-Output "Restart Chrome, Edge, and Firefox completely."
Write-Output "Load extension\dist\chromium in Chrome or Edge, and extension\dist\firefox in Firefox."
