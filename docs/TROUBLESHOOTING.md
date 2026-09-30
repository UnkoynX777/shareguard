# Troubleshooting

## Native helper not installed

The popup says **Native helper not installed. Install ShareGuard for Windows to continue.** The **Native** line reads **Unavailable**. The empty list says **Native helper not installed**, and **Installation guide** links to the install steps.

That appears when the browser cannot start `com.shareguard.native`.

- Run `ShareGuard-Setup-vX.Y.Z-x64.exe` from the [latest release](https://github.com/UnkoynX777/shareguard/releases/latest).
- Quit the browser from the tray, not only the window, and open it again.
- Chrome and Edge need `com.shareguard.native.chromium.json`. Firefox needs `com.shareguard.native.firefox.json`.

Developers who build from source register the helper with `scripts/install-native-host.ps1`. That is not the user install path.

## Extension and native helper do not match

The message is **ShareGuard extension and native helper do not match. Install the same release for both.**

The setup and the extension have to be the same ShareGuard version. Download both from one GitHub Release, run the setup, reload the extension, and restart the browser.

`PROTOCOL_VERSION_MISMATCH` is the same situation when the helper rejects the connection. It is not a missing install.

## Extension not visible

Open `chrome://extensions`, `edge://extensions`, or Firefox **Add-ons and themes** and confirm ShareGuard is listed. Pin it from the puzzle or extensions icon in the toolbar.

If the folder `%LOCALAPPDATA%\ShareGuard\Extension\Chromium` was deleted after **Load unpacked**, Chrome and Edge lose the files. Run the setup again and load that folder again.

## Chrome Load unpacked missing

**Load unpacked** is shown only while **Developer mode** is on. Open `chrome://extensions` and turn **Developer mode** on. Then select the folder, not the zip.

## Edge Load unpacked missing

Same as Chrome, on `edge://extensions`. Turn **Developer mode** on, then **Load unpacked**, then select `%LOCALAPPDATA%\ShareGuard\Extension\Chromium`.

## Firefox says the extension is unsigned

Firefox stable will not permanently install an unsigned package. Install `ShareGuard-Firefox-vX.Y.Z.xpi` from the GitHub Release, using **Install Add-on From File**.

If that release says the Firefox package is unavailable, there is no signed file for that version. Do not rename another file to `ShareGuard-Firefox-vX.Y.Z.xpi`.

A temporary add-on is only for development. See [DEVELOPMENT.md](./DEVELOPMENT.md).

## Native Messaging connection failed

Confirm these registry values exist for the current user and point at the JSON files next to the helper:

- `HKCU\Software\Google\Chrome\NativeMessagingHosts\com.shareguard.native`
- `HKCU\Software\Microsoft\Edge\NativeMessagingHosts\com.shareguard.native`
- `HKCU\Software\Mozilla\NativeMessagingHosts\com.shareguard.native`

The Chromium JSON must allow `chrome-extension://bdkcdhphggeglifemnakdlcfbhcoempk/`. The Firefox JSON must allow `shareguard@shareguard.local`. Both point at the same `shareguard-native.exe`.

Restart the browser after the setup changes those files. A browser that is still running keeps the previous helper process.

## This Firefox version is not supported

That text is shown when Firefox cannot use Native Messaging for this extension. ShareGuard requires Firefox 128 or newer. Firefox ESR 115 does not provide the page hook ShareGuard uses.

## An application is missing

The list shows processes that are playing audio or have a visible window. Services without a window stay hidden until you enable **Show all processes**. Search matches the display name and the executable name.

A blocked executable that is not running stays under **Blocked apps**.

## Viewers still hear a blocked application

Protection has to be on before or during the share. With protection off, the original stream is used. The line reads **Protection enabled** or **Protection disabled**.

Windows process-loopback exclusion takes one target process tree per capture. Several blocked applications use a mix of the allowed applications that are producing audio. A root that also contains a blocked process is dropped. If that cannot be done, the extension removes shared audio instead of sending the system mix.

Local playback is not muted. Hearing the application yourself does not mean viewers hear it.

## No audio in the share

If the site or the browser did not request audio, ShareGuard does not add a track. Include audio in the browser share dialog when the site needs it.

If protection was on and the helper failed, ShareGuard removes the shared audio track and keeps the video. The popup message line shows the helper error. The page can also show: `ShareGuard could not protect this share, so shared audio was removed. Playback on this computer was not changed.`

## The share dialog was cancelled

The site should still receive the original `getDisplayMedia` failure, including `NotAllowedError`. ShareGuard does not replace that error with a silent stream.

## ShareGuard requires a newer version of Windows

Process-loopback activation failed and the Windows build is below 20348. The helper tries from build 19041, which is earlier than the build Microsoft documents. On a build below 20348, failure becomes `UNSUPPORTED_WINDOWS` with that sentence. Update Windows, or leave protection off if you accept the original share audio.

## Audio protection failed

`AUDIO_INITIALIZATION_FAILED` and `PROCESS_CAPTURE_FAILED` come from the helper when a device or a process capture cannot be opened. `CAPTURE_STOPPED_UNEXPECTEDLY` is sent when capture restarts too often. Video should remain. Shared audio is removed.

## Popup closed during a share

The share continues. The popup is not the capture session. Reopen it to change the block list. The helper applies the new policy without a new screen share.
