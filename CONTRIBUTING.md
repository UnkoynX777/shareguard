# Contributing

[English](./README.md) | [Português (Brasil)](./README.pt-BR.md)

ShareGuard is free and open source under the [MIT license](./LICENSE). Created and maintained by [UnkoynX777](https://github.com/UnkoynX777).

## What to work on

- Bug reports and feature requests use the [issue forms](https://github.com/UnkoynX777/shareguard/issues).
- Security reports follow [SECURITY.md](./SECURITY.md). Do not file those as public issues.
- Behavior toward other people follows [CODE_OF_CONDUCT.md](./CODE_OF_CONDUCT.md).

## Requirements

- Windows 10 or 11, 64-bit
- Visual Studio 2022 with the Desktop development with C++ workload and the Windows SDK
- Node.js 22 or newer, only to build and test the extension
- Inno Setup 6.7, only to compile the installer

The native helper builds with MSVC. CMake rejects other compilers.

## Repository

```text
native/        C++20 helper
extension/     WebExtension source and browser manifests
installer/     Inno Setup script
scripts/       build, version check, dev host registration
docs/          installation, build, architecture, development, troubleshooting
```

## Setup

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build-native.ps1
cd extension
npm ci
npm run typecheck
npm test
npm run build
npm run lint:firefox
```

Details are in [docs/BUILDING.md](./docs/BUILDING.md) and [docs/DEVELOPMENT.md](./docs/DEVELOPMENT.md).

## Version

`VERSION` is the source of truth. `scripts/apply-version.ps1` copies it into the manifest, package, CMake project, native protocol version, and installer. `scripts/check-version.ps1` fails when they differ.

Release tags look like `v0.3.0` and must match that number. `v0.3.0-beta.1` is a pre-release of the same `VERSION`.

## Style

Match the file you are editing. C++ and TypeScript sources in this repository do not use comments. Explain the change in the pull request instead. Do not add site-specific branches such as a Google Meet or Discord special case. Do not add telemetry.

## Flow

1. Fork the repository.
2. Create a branch: `feat/...`, `fix/...`, `docs/...`, or `refactor/...`.
3. Make the change and run the build and tests that cover it.
4. Open a pull request against `main`.

Commits do not have to follow Conventional Commits. A short sentence that says why the change exists is enough.

## Pull requests

Say what changed, why, and how you tested it. Update the docs when the install or build steps change. Update `CHANGELOG.md` under `Unreleased` when the change is user-visible.
