$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vswhere)) {
  throw "Visual Studio Installer (vswhere.exe) was not found."
}
$install = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $install) {
  throw "Visual Studio with the Desktop development with C++ workload was not found."
}
$vcvars = Join-Path $install "VC\Auxiliary\Build\vcvars64.bat"
$cmake = Join-Path $install "Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
if (-not (Test-Path $cmake)) {
  $cmakeCmd = Get-Command cmake -ErrorAction SilentlyContinue
  if (-not $cmakeCmd) {
    throw "CMake was not found. Install CMake or the CMake tools that ship with Visual Studio."
  }
  $cmake = $cmakeCmd.Source
}
$source = Join-Path $root "native"
$build = Join-Path $root "native\build"
$command = "call `"$vcvars`" && `"$cmake`" -S `"$source`" -B `"$build`" -A x64 && `"$cmake`" --build `"$build`" --config Release"
cmd /c $command
if ($LASTEXITCODE -ne 0) {
  exit $LASTEXITCODE
}
