English | [Português (Brasil)](./CHANGELOG.pt-BR.md)

# Changelog

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and this project follows [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [0.3.1] - 2026-09-30

### Fixed

- Continuous shared audio is no longer chopped by silence inserted when the mixer ran ahead of capture. Playback waits on a short buffer so transport jitter does not become gaps.

### Added

- Brazilian Portuguese translations of the user and contributor documentation. English remains the canonical language.

## [0.3.0] - 2026-09-30

### Added

- Per-application filtering of browser screen-share audio on Windows, without changing local playback.
- One native helper, `shareguard-native.exe`, shared by Chrome, Edge, and Firefox through Native Messaging.
- Chromium and Firefox extension builds from one TypeScript codebase.
- A per-user installer that registers the native host for Chrome, Edge, and Firefox and copies the Chromium extension to `%LOCALAPPDATA%\ShareGuard\Extension\Chromium`.
- GitHub Release packages `ShareGuard-Setup-vX.Y.Z-x64.exe`, `ShareGuard-Chromium-vX.Y.Z.zip`, and, when Mozilla unlisted signing is configured, `ShareGuard-Firefox-vX.Y.Z.xpi`.

[Unreleased]: https://github.com/UnkoynX777/shareguard/compare/v0.3.1...HEAD
[0.3.1]: https://github.com/UnkoynX777/shareguard/releases/tag/v0.3.1
[0.3.0]: https://github.com/UnkoynX777/shareguard/releases/tag/v0.3.0
