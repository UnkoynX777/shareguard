English | [Português (Brasil)](./README.pt-BR.md)

<p align="center">
  <img src="extension/public/icons/icon128.png" width="96" alt="ShareGuard icon">
</p>

<h1 align="center">ShareGuard</h1>

<p align="center">
  Share your screen. Keep private apps private.
</p>

<p align="center">
  Filter application audio from browser screen sharing without muting what you hear locally.
</p>

<p align="center">
  <a href="https://github.com/UnkoynX777/shareguard/releases/latest"><img alt="GitHub release" src="https://img.shields.io/github/v/release/UnkoynX777/shareguard?display_name=tag"></a>
  <a href="https://github.com/UnkoynX777/shareguard/actions/workflows/ci.yml"><img alt="CI" src="https://github.com/UnkoynX777/shareguard/actions/workflows/ci.yml/badge.svg"></a>
  <a href="./LICENSE"><img alt="MIT license" src="https://img.shields.io/badge/license-MIT-blue"></a>
  <img alt="Windows x64" src="https://img.shields.io/badge/platform-Windows%20x64-blue">
  <img alt="Chrome, Edge, and Firefox" src="https://img.shields.io/badge/browsers-Chrome%20%C2%B7%20Edge%20%C2%B7%20Firefox-blue">
</p>

<p align="center">
  <a href="https://github.com/UnkoynX777/shareguard/releases/latest"><strong>Download for Windows</strong></a>
  ·
  <a href="./docs/INSTALLATION.md">Installation</a>
  ·
  <a href="https://github.com/UnkoynX777/shareguard/releases">All releases</a>
</p>

ShareGuard lets you choose which Windows applications can be heard in a browser screen share. A blocked application stays audible to you. It is removed from the audio sent to the site. The video is left as it is.

Free and open source. Processing stays on your computer. There is no account and no audio upload.

## Download

Get the latest ShareGuard release for Windows.

[Download ShareGuard](https://github.com/UnkoynX777/shareguard/releases/latest)

Run `ShareGuard-Setup-vX.Y.Z-x64.exe`. It installs the native helper and copies the Chrome and Edge extension into a fixed folder for your user. After installing, add the extension in the browser:

Chrome · Edge · Firefox

[Installation guide](./docs/INSTALLATION.md)

Windows 10 or 11, 64-bit. Chrome 116+, current Edge, or Firefox 128+. Official files are only on that GitHub Release. `SHA256SUMS.txt` is there if you want to check a download. ShareGuard uses the [SignPath Foundation](https://signpath.org) for code signing.

## What you hear and what viewers hear

| Application | You hear | Viewers hear |
| --- | --- | --- |
| Discord | Yes | No, if you set it to Blocked |
| Spotify | Yes | No, if you set it to Blocked |
| Game | Yes | Yes, while it stays Allowed |
| Browser | Yes | Yes, while it stays Allowed |

Those names are examples. ShareGuard does not treat any executable specially. It lists what is running.

## Features

- Block or allow each application by executable name
- Local playback stays unchanged
- The process list updates while a share is active
- Rules remain when the application restarts with a new process ID
- One helper for Chrome, Edge, and Firefox
- WASAPI process loopback, not a virtual audio cable
- No cloud processing

## Quick start

1. Download and install ShareGuard from the [latest release](https://github.com/UnkoynX777/shareguard/releases/latest).
2. Add the browser extension. [Installation guide](./docs/INSTALLATION.md).
3. Open ShareGuard. **Native** should read **Connected**.
4. Turn on protection and set unwanted applications to Blocked.
5. Start a screen share that includes audio.

## Browser support

| Browser | Extension | Installation |
| --- | --- | --- |
| Google Chrome | Chromium | Manual |
| Microsoft Edge | Chromium | Manual |
| Mozilla Firefox | Firefox | Signed XPI |

Chrome and Edge use Developer mode and Load unpacked on the folder the setup installs. Firefox uses the signed XPI from the same release, when that file is published. Firefox ESR 115 is not supported. Details are in the [installation guide](./docs/INSTALLATION.md).

## How to use

The popup has a protection switch, a search box, and Allow or Blocked on each row. Playing audio is listed first. A blocked executable that is closed stays under Blocked apps. Show all processes reveals entries that are otherwise hidden because they have no window and no audio.

A new application is Allowed until its executable is blocked. Changing the list during a share updates the capture. Closing the popup does not stop the share.

If protection cannot be enforced, ShareGuard removes the shared audio and keeps the video. If the share has no audio track, it does not add one. Cancelling the screen picker still returns the browser's original error to the site.

## How it works

Browser extensions cannot capture per-process Windows audio. ShareGuard uses a small open-source helper, `shareguard-native.exe`, to talk to Windows Core Audio. The browser starts that helper through Native Messaging. The extension replaces the audio track of `getDisplayMedia` with a track built in the page. Your speakers and the Windows volume mixer are not changed.

```mermaid
flowchart LR
  apps[Windows applications] --> helper[Native helper]
  helper --> extension[Browser extension]
  extension --> share[Screen share]
```

More detail is in [Architecture](./docs/ARCHITECTURE.md).

## Privacy

ShareGuard processes application names and audio routing on your computer. There is no ShareGuard account, analytics, or telemetry. Shared audio is sent to the website you are sharing with, as it would be without ShareGuard, after the filter is applied. It is not uploaded to a ShareGuard server.

The helper log, when debug is enabled, does not contain audio samples.

## Building

Users do not need this section. Developers:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build-native.ps1
cd extension
npm ci
npm run build
```

[Building](./docs/BUILDING.md) · [Development](./docs/DEVELOPMENT.md) · [Contributing](./CONTRIBUTING.md)

## Troubleshooting

If **Native** stays **Unavailable**, start with [Troubleshooting](./docs/TROUBLESHOOTING.md). Security reports go to [SECURITY.md](./SECURITY.md), not to a public issue.

## Roadmap

Available now: per-application filtering, a per-user Windows setup, and browser extension packages from GitHub Releases.

Not available yet: code signing of the installer. A Firefox package is published only after Mozilla signs it for that release.

## Why isn't ShareGuard in the browser stores?

ShareGuard is distributed directly through GitHub. The extension source and the release packages are public in this repository.

Documentation is available in English and Brazilian Portuguese. The rules are in [Documentation](./docs/DOCUMENTATION.md).

## License

[MIT](./LICENSE). Created and maintained by [UnkoynX777](https://github.com/UnkoynX777). The [LICENSE](./LICENSE) file is the official license text and stays in English.
