English | [Português (Brasil)](./BUILDING.pt-BR.md)

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

This prints the shared version, currently `0.3.1`, or fails when these disagree with `VERSION`:

- `VERSION`
- `extension/manifests/manifest.base.json`
- `extension/package.json`
- `native/CMakeLists.txt`
- `native/src/messaging/Protocol.hpp`
- `installer/shareguard.iss`

Change `VERSION`, then run `scripts/apply-version.ps1`. Do not edit the other five by hand.

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

The build fails if the Chromium manifest key does not produce extension ID `bdkcdhphggeglifemnakdlcfbhcoempk`, or if the Firefox ID is not `shareguard@shareguard.local`. Those IDs are what the native host allows.

The user Chromium zip is separate:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\package-release.ps1
powershell -ExecutionPolicy Bypass -File .\scripts\validate-packages.ps1
```

That writes `release/ShareGuard-Chromium-v0.3.1.zip`. Inside it, `ShareGuard-Chromium/manifest.json` is the extension root. `extension/dist/firefox` stays the unsigned development build. It is not named `ShareGuard-Firefox-vX.Y.Z.xpi`.

Also available: `npm run build:chromium`, `npm run build:firefox`, `npm run dev:chromium`, and `npm run dev:firefox`.

`npm test` runs `extension/test/display-media-contract.test.ts`. It checks the `getDisplayMedia` wrapper contract. It does not open a browser.

`npm run lint:firefox` runs `web-ext lint` on `extension/dist/firefox`.

## Installer

Build the extension and the native helper first. The script copies `extension/dist/chromium` into the setup. Then, with Inno Setup 6 installed:

```powershell
& "${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe" .\installer\shareguard.iss
```

The result is `installer\Output\ShareGuard-Setup-v0.3.1-x64.exe`. The version in the file name comes from `AppVersion` in `installer/shareguard.iss`.

## Release build

The release workflow, on a tag such as `v0.3.1`, checks the version, builds the extension, packages the Chromium zip, validates IDs, signs Firefox when `AMO_JWT_ISSUER` and `AMO_JWT_SECRET` are set, builds the native helper, then compiles the setup. A tag such as `v0.3.1-beta.1` is published as a pre-release and still has to match `VERSION` `0.3.1`.

Published names:

- `ShareGuard-Setup-v0.3.1-x64.exe`
- `ShareGuard-Chromium-v0.3.1.zip`
- `ShareGuard-Firefox-v0.3.1.xpi` only when Mozilla signing succeeded
- `SHA256SUMS.txt`

If the signing secrets are absent, the workflow still publishes the setup and the Chromium zip, and the release notes say the Firefox user package is unavailable. If signing is attempted and fails, the release is not created. An unsigned file is never uploaded as `ShareGuard-Firefox-vX.Y.Z.xpi`.
