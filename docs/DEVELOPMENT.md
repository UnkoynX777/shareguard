# Development

Build the helper and the extension first. See [BUILDING.md](./BUILDING.md).

## Layout

```text
native/src          helper implementation
extension/src       shared extension code
extension/manifests manifest.base.json plus Chromium and Firefox overrides
extension/scripts   esbuild and manifest merge
installer           Inno Setup script
scripts             native build, version check, dev host registration
```

`extension/src/platform/browser` is the browser boundary. Chrome and Edge use `ChromiumAdapter`. Firefox uses `FirefoxAdapter`. There is no separate Edge adapter.

The Chromium manifest contains a public extension key. That key pins the unpacked extension ID to `bdkcdhphggeglifemnakdlcfbhcoempk`. It is not a private credential. Do not replace it unless you also change the native host `allowed_origins`.

## Native Messaging on a dev machine

The installer points the registry at `%LOCALAPPDATA%\ShareGuard\shareguard-native.exe`. For a helper you just built:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\install-native-host.ps1
```

That writes two JSON manifests under `%LOCALAPPDATA%\ShareGuard` and points Chrome, Edge, and Firefox at `native\build\Release\shareguard-native.exe`. Running the installer again points those same registry keys back at the copy under `%LOCALAPPDATA%\ShareGuard`. Use one of the two, then restart the browsers.

To remove the dev registration:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\uninstall-native-host.ps1
```

That removes the three registry keys, the JSON manifests, and `%LOCALAPPDATA%\ShareGuard\shareguard-native.exe`. It does not delete `native\build\Release\shareguard-native.exe`.

## Load the extension

Chrome and Edge: open `chrome://extensions` or `edge://extensions`, enable Developer mode, and load `extension/dist/chromium`.

Firefox: `npm run firefox` builds the Firefox package and runs `web-ext`. Or load `extension/dist/firefox/manifest.json` from `about:debugging` as a temporary add-on. Firefox drops that add-on on exit.

`npm run dev:chromium` and `npm run dev:firefox` rebuild while you edit. Reload the extension in the browser after a rebuild. Restart the browser after you replace the native executable.

## Logging

Pass `--debug` to the helper, or set `SHAREGUARD_DEBUG`. Logs go to stderr and to `%LOCALAPPDATA%\ShareGuard\shareguard.log`. The file is capped at about 1 MB. PCM is not logged.

The popup shows the helper error text in the message line. `Native` reads `Connected` or `Unavailable`.

## What not to add

Do not special-case a website hostname. The hook is `getDisplayMedia`. Do not add a second native executable per browser. Do not send audio to a server. This repository has no `.env` file.
