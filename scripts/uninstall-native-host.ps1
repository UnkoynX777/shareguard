$ErrorActionPreference = "Stop"
Remove-Item "HKCU:\Software\Google\Chrome\NativeMessagingHosts\com.shareguard.native" -Recurse -Force -ErrorAction SilentlyContinue
Remove-Item "HKCU:\Software\Microsoft\Edge\NativeMessagingHosts\com.shareguard.native" -Recurse -Force -ErrorAction SilentlyContinue
Remove-Item "HKCU:\Software\Mozilla\NativeMessagingHosts\com.shareguard.native" -Recurse -Force -ErrorAction SilentlyContinue
$dir = Join-Path $env:LOCALAPPDATA "ShareGuard"
Remove-Item (Join-Path $dir "com.shareguard.native.json") -Force -ErrorAction SilentlyContinue
Remove-Item (Join-Path $dir "com.shareguard.native.chromium.json") -Force -ErrorAction SilentlyContinue
Remove-Item (Join-Path $dir "com.shareguard.native.firefox.json") -Force -ErrorAction SilentlyContinue
Remove-Item (Join-Path $dir "shareguard-native.exe") -Force -ErrorAction SilentlyContinue
Write-Output "Removed the ShareGuard native messaging host for Chrome, Edge, and Firefox."
