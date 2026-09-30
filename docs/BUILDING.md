# Building

Commands below are run from the repository root unless a step says otherwise.

## Requirements

- Windows 10 or 11, 64-bit
- Visual Studio 2022 with the Desktop development with C++ workload and the Windows SDK
- CMake 3.20 or newer. The Visual Studio CMake is enough when it is installed with that workload
- Node.js 22 or newer
- Inno Setup 6.7, only for the installer

`native/CMakeLists.txt` stops the configure step when the compiler is not MSVC.

## Version check

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\check-version.ps1
```

This prints the shared version, currently `0.3.0`, or fails when these files disagree:

- `extension/manifests/manifest.base.json`
- `extension/package.json`
- `native/CMakeLists.txt`
- `native/src/messaging/Protocol.hpp`
- `installer/shareguard.iss`

## Native helper

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build-native.ps1
```

The script finds Visual Studio with `vswhere`, loads the x64 developer environment, then runs:

```text
cmake -S native -B native/build -A x64
cmake --build native/build --config Release
```

The executable is `native\build\Release\shareguard-native.exe`.

A capture that does not use a browser:

```powershell
.\native\build\Release\shareguard-native.exe --capture-wav .\filtered.wav --seconds 5 --block App.exe --debug
```

`--block` repeats. `--seconds` is clamped to 1..30. The WAV is 48 kHz stereo PCM. Debug logs go to stderr and, when debug is on, to `%LOCALAPPDATA%\ShareGuard\shareguard.log`. Audio samples are not written to the log. `SHAREGUARD_DEBUG` also turns debug logging on.

## Extension

```powershell
cd extension
npm ci
npm run typecheck
npm test
npm run build
npm run lint:firefox
```

`npm run build` writes:

- `extension/dist/chromium`
- `extension/dist/firefox`
- `release/shareguard-chromium.zip`
- `release/shareguard-firefox.zip`

Also available: `npm run build:chromium`, `npm run build:firefox`, `npm run dev:chromium`, and `npm run dev:firefox`.

`npm test` runs `extension/test/display-media-contract.test.ts`. It checks the `getDisplayMedia` wrapper contract. It does not open a browser.

`npm run lint:firefox` runs `web-ext lint` on `extension/dist/firefox`.

## Installer

Build the native helper first. Then, with Inno Setup 6 installed:

```powershell
& "${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe" .\installer\shareguard.iss
```

The result is `installer\Output\ShareGuard-Setup-v0.3.0-x64.exe`. The version in the file name comes from `AppVersion` in `installer/shareguard.iss`.

## Release build

The same commands, in this order, are what the release workflow runs: version check, native build, `npm ci`, typecheck, test, `npm run build`, `npm run lint:firefox`, then `ISCC.exe`.

A GitHub Release is created by pushing a tag such as `v0.3.0`. The tag must match the version in the files above. Published names:

- `ShareGuard-Setup-v0.3.0-x64.exe`
- `shareguard-chromium-v0.3.0.zip`
- `shareguard-firefox-v0.3.0.zip`
- `SHA256SUMS.txt`
