# Troubleshooting

## Native helper unavailable

The popup shows this when `runtime.connectNative("com.shareguard.native")` fails.

- Run the installer, or `scripts/install-native-host.ps1` if you are developing.
- Quit the browser from the tray, not only the window, and open it again.
- Chrome and Edge need the Chromium manifest. Firefox needs the Firefox manifest. A host registered only for Chrome does not satisfy Firefox.
- The Chromium extension must be the unpacked package whose ID is `bdkcdhphggeglifemnakdlcfbhcoempk`.
- The Firefox add-on ID must be `shareguard@shareguard.local`.

## This Firefox version is not supported

That text is shown when `runtime.connectNative` is missing. ShareGuard requires Firefox 128 or newer because the page hook uses the MAIN world. Firefox ESR 115 does not provide it.

## Extension cannot connect after a rebuild

The registry still points at an old executable, or the browser is still running the previous helper process. Register the host again and restart the browser.

If you installed with the installer and later ran `install-native-host.ps1`, the registry now points at `native\build\Release\shareguard-native.exe`. If you then deleted the build directory, the host path is broken. Run the installer again, or rebuild and register the dev host.

## An application is missing

The list shows processes that are playing audio or have a visible window. Services without a window stay hidden until you enable Show all processes. Search matches the display name and the executable name.

A blocked executable that is not running stays under Blocked apps.

## Viewers still hear a blocked application

Protection has to be on before or during the share. With protection off, the original stream is used.

Windows process-loopback exclusion takes one target process tree per capture. Several blocked applications use a mix of the allowed applications that are producing audio. A root that also contains a blocked process is dropped. If that cannot be done, the extension removes shared audio instead of sending the system mix.

Local playback is not muted. Hearing the application yourself does not mean viewers hear it.

## No audio in the share

If the site or the browser did not request audio, ShareGuard does not add a track. Check the browser share dialog and include audio there when the site needs it.

If protection was on and the helper failed, ShareGuard removes the shared audio track and keeps the video. The popup message line shows the helper error. The page can also show: `ShareGuard could not protect this share, so shared audio was removed. Playback on this computer was not changed.`

## The share dialog was cancelled

The site should still receive the original `getDisplayMedia` failure, including `NotAllowedError`. ShareGuard does not replace that error with a silent stream.

## ShareGuard requires a newer version of Windows

Process-loopback activation failed and the Windows build is below 20348. The helper tries from build 19041, which is earlier than the build Microsoft documents. On a build below 20348, failure becomes `UNSUPPORTED_WINDOWS` with that sentence. Update Windows, or leave protection off if you accept the original share audio.

## Audio protection failed

`AUDIO_INITIALIZATION_FAILED` and `PROCESS_CAPTURE_FAILED` come from the helper when a device or a process capture cannot be opened. `CAPTURE_STOPPED_UNEXPECTEDLY` is sent when capture restarts too often. Video should remain. Shared audio is removed.

`PROTOCOL_VERSION_MISMATCH` means the extension and the helper are not the same protocol. Install matching versions and restart the browser.

## Firefox Native Messaging

Confirm `HKCU\Software\Mozilla\NativeMessagingHosts\com.shareguard.native` exists and that its default value is the full path of `com.shareguard.native.firefox.json`. That JSON must use `allowed_extensions` with `shareguard@shareguard.local`, not `allowed_origins`.

A temporary Firefox add-on disappears when Firefox exits. Load it again from `about:debugging`.

## Chrome or Edge Native Messaging

Confirm the Chrome or Edge `NativeMessagingHosts\com.shareguard.native` value points at `com.shareguard.native.chromium.json`. That JSON must list `chrome-extension://bdkcdhphggeglifemnakdlcfbhcoempk/`.

## Popup closed during a share

The share is supposed to continue. The popup is not the capture session. Reopen it to change the block list. The helper applies the new policy without a new screen share.
