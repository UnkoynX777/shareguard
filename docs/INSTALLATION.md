English | [Português (Brasil)](./INSTALLATION.pt-BR.md)

# Installing ShareGuard

ShareGuard on Windows has two parts.

1. ShareGuard Native, installed by the Windows setup.
2. The ShareGuard browser extension, added in the browser afterward.

The setup configures the native component. It does not install the extension inside Chrome, Edge, or Firefox. The extension is distributed from GitHub, so the browser asks you to add it yourself.

## Before you start

- Windows 10 or 11, 64-bit.
- Google Chrome 116 or newer, current Microsoft Edge, or Firefox 128 or newer.

Firefox ESR 115 is not supported.

Download only from [github.com/UnkoynX777/shareguard/releases](https://github.com/UnkoynX777/shareguard/releases).

## Step 1 — Install ShareGuard

1. Open the [latest release](https://github.com/UnkoynX777/shareguard/releases/latest).
2. Download `ShareGuard-Setup-vX.Y.Z-x64.exe`.
3. Run it and finish the setup.

The setup does not need an administrator account. It installs the native helper for the current Windows user and copies the Chrome and Edge extension files to:

```text
%LOCALAPPDATA%\ShareGuard\Extension\Chromium
```

On the last page, ShareGuard tells you the extension is still required. Leave **Open the installation guide** selected if you want this page again.

The installer is not code-signed. Windows SmartScreen can say it protected your PC. Official builds are only the files on the GitHub Release above. If you want to check the file before you run it, compare its SHA-256 with `SHA256SUMS.txt` from that same release:

```powershell
Get-FileHash .\ShareGuard-Setup-vX.Y.Z-x64.exe -Algorithm SHA256
```

Do not run a setup that came from somewhere else because a warning appeared, and do not run one that came from somewhere else because the warning did not appear.

After setup, quit Chrome, Edge, and Firefox completely, including the tray icon, then open the browser again.

## Step 2 — Install the browser extension

### Chrome

Because ShareGuard is distributed directly through GitHub instead of a browser store, Chrome requires Developer mode to load the extension manually.

1. Open `chrome://extensions`.
2. Turn on **Developer mode**.
3. Click **Load unpacked**.
4. Select this folder:

```text
%LOCALAPPDATA%\ShareGuard\Extension\Chromium
```

The Start menu entry **ShareGuard → Install Browser Extension** opens that folder. Select the folder itself. Do not select a zip file.

5. Pin ShareGuard to the toolbar.
6. Open ShareGuard.

Keep that folder. If you delete it, Chrome no longer has the extension files.

`ShareGuard-Chromium-vX.Y.Z.zip` on the release is the same extension. Extract it first. After extraction, the folder `ShareGuard-Chromium` contains `manifest.json`. Load that folder. Do not load the zip.

### Edge

Edge uses the same Chromium extension. There is no separate Edge download.

1. Install ShareGuard Setup.
2. Open `edge://extensions`.
3. Turn on **Developer mode**.
4. Click **Load unpacked**.
5. Select `%LOCALAPPDATA%\ShareGuard\Extension\Chromium`.
6. Pin ShareGuard.

### Firefox

Firefox stable does not keep an unsigned extension after you install it. The user package is `ShareGuard-Firefox-vX.Y.Z.xpi` from the GitHub Release. That file is signed by Mozilla for distribution from GitHub. It is not a public store listing.

If the release notes say the Firefox package is unavailable, there is no Firefox file to install for that version.

1. Install ShareGuard Setup.
2. Download `ShareGuard-Firefox-vX.Y.Z.xpi` from the same release as the setup.
3. Open Firefox.
4. Open **Add-ons and themes** (`about:addons`).
5. Open the settings menu (the gear).
6. Choose **Install Add-on From File**.
7. Select `ShareGuard-Firefox-vX.Y.Z.xpi`.
8. Click **Add**.

Do not load a zip, and do not use a temporary add-on, for a normal install.

## Step 3 — Verify ShareGuard

Open the ShareGuard extension.

The **Native** line should read **Connected**.

If you see that, installation is complete.

Turn on protection. The line reads **Protection enabled**. Sharing reads **Not sharing** until a screen share starts, then **Active**.

## Updating

Download the newer `ShareGuard-Setup-vX.Y.Z-x64.exe` from the [latest release](https://github.com/UnkoynX777/shareguard/releases/latest) and run it. It replaces the native helper and the files in `%LOCALAPPDATA%\ShareGuard\Extension\Chromium`. That path does not change with the version number, so Chrome and Edge keep pointing at the same folder.

Then open `chrome://extensions` or `edge://extensions`, find ShareGuard, and click **Reload**. Restart the browser if it was already running during setup.

Your block list stays in the browser.

Firefox does not update from that folder. Download the newer `ShareGuard-Firefox-vX.Y.Z.xpi` from the same release and install it from a file over the previous add-on.

There is no separate updater.

## Uninstalling

Use Windows Settings, Apps, ShareGuard. You can also use **ShareGuard → Uninstall ShareGuard** in the Start menu.

That removes the native helper, the Native Messaging registration for Chrome, Edge, and Firefox, the extension files under `%LOCALAPPDATA%\ShareGuard\Extension`, and the Start menu shortcuts.

Chrome or Edge can still list ShareGuard after that, because the setup does not edit the browser profile. Open the extensions page and remove ShareGuard there. Firefox: remove it from **Add-ons and themes**.

## Troubleshooting

See [TROUBLESHOOTING.md](./TROUBLESHOOTING.md).

## Why isn't ShareGuard in the browser stores?

ShareGuard is distributed directly through GitHub. The extension source and the release packages are public in this repository.
