# Installation

ShareGuard on Windows has two parts. The installer sets up the native helper. The browser extension is a separate package. The extension is not in the Chrome Web Store, Edge Add-ons, or Firefox Add-ons.

## Requirements

- Windows 10 or 11, 64-bit. Process loopback is attempted from build 19041. Microsoft documents that API from build 20348. If Windows refuses it while applications are blocked, ShareGuard removes the shared audio instead of sending the original mix.
- Google Chrome 116 or newer, or current Chromium-based Microsoft Edge, or Firefox 128 or newer. Firefox ESR 115 cannot load the page hook.

## Download

Open the [latest release](https://github.com/UnkoynX777/shareguard/releases/latest).

- `ShareGuard-Setup-vX.Y.Z-x64.exe` installs the native helper.
- `shareguard-chromium-vX.Y.Z.zip` is the extension for Chrome and Edge.
- `shareguard-firefox-vX.Y.Z.zip` is the extension for Firefox.
- `SHA256SUMS.txt` lists the SHA-256 hashes.

The installer is not code-signed. Windows SmartScreen can warn about an unsigned file. Check the hash before you run it.

## Install the helper

Run `ShareGuard-Setup-vX.Y.Z-x64.exe`. It does not ask for an administrator account. It copies `shareguard-native.exe` to `%LOCALAPPDATA%\ShareGuard` and registers the Native Messaging host for the current user:

- Chrome: `HKCU\Software\Google\Chrome\NativeMessagingHosts\com.shareguard.native`
- Edge: `HKCU\Software\Microsoft\Edge\NativeMessagingHosts\com.shareguard.native`
- Firefox: `HKCU\Software\Mozilla\NativeMessagingHosts\com.shareguard.native`

Chrome and Edge share one manifest (`allowed_origins`). Firefox uses another (`allowed_extensions`) and the same executable. Close Chrome, Edge, and Firefox completely after installation, then open them again.

## Install the extension

### Chrome or Edge

1. Unzip `shareguard-chromium-vX.Y.Z.zip`.
2. Open `chrome://extensions` or `edge://extensions`.
3. Turn on Developer mode.
4. Choose Load unpacked and select the unzipped folder.

The Chromium extension ID is `bdkcdhphggeglifemnakdlcfbhcoempk`. The native host allows that ID. Loading a rebuilt copy that does not contain the same manifest key will not connect.

### Firefox

Firefox removes temporary add-ons when it exits. Until the package is signed by Mozilla, that is the available install path.

1. Unzip `shareguard-firefox-vX.Y.Z.zip`.
2. Open `about:debugging#/runtime/this-firefox`.
3. Choose Load Temporary Add-on and select `manifest.json` inside the unzipped folder.

The add-on ID is `shareguard@shareguard.local`. Load the add-on again after each Firefox restart.

## First use

1. Open the ShareGuard popup.
2. Native should read Connected. If it reads Unavailable, see [TROUBLESHOOTING.md](./TROUBLESHOOTING.md).
3. Turn on ShareGuard Protection.
4. Set the applications you do not want viewers to hear to Blocked. A new application stays Allowed until you block its executable.
5. Start a screen share that includes audio.

Blocked applications stay audible on your computer. They are removed from the audio sent to the site. The video is unchanged. Closing the popup does not stop the share.

If the share has no audio track, ShareGuard leaves the stream alone. It does not add audio.

## Upgrade

Download the new release, run the new installer, and load the matching extension package. Rules stay in the browser (`storage.local`). They are not stored by the installer.

Restart the browsers after replacing the helper. A browser that already launched the old executable keeps that process until it exits.

## Uninstall

Use Windows Settings, Apps, ShareGuard. That removes the helper, the Native Messaging manifests, and the Chrome, Edge, and Firefox registry values for the current user.

Remove the extension yourself:

- Chrome or Edge: `chrome://extensions` or `edge://extensions`, then Remove.
- Firefox: close Firefox. A temporary add-on is already gone on exit.
