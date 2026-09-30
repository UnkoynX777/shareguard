import esbuild from "esbuild";
import { execFileSync } from "node:child_process";
import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";

const extensionRoot = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "..");
const projectRoot = path.resolve(extensionRoot, "..");
const args = process.argv.slice(2);
const watch = args.includes("--watch");
const requested = args.filter((arg) => arg === "chromium" || arg === "firefox");
const targets = requested.length > 0 ? requested : ["chromium", "firefox"];

function readJson(file) {
  return JSON.parse(fs.readFileSync(file, "utf8"));
}

function merge(base, override) {
  const output = { ...base };
  for (const [key, value] of Object.entries(override)) {
    const current = output[key];
    if (
      value &&
      typeof value === "object" &&
      !Array.isArray(value) &&
      current &&
      typeof current === "object" &&
      !Array.isArray(current)
    ) {
      output[key] = merge(current, value);
    } else {
      output[key] = value;
    }
  }
  return output;
}

function copyBundle(target) {
  const destination = path.join(extensionRoot, "dist", target);
  fs.mkdirSync(destination, { recursive: true });
  const bundle = path.join(extensionRoot, "dist", ".bundle");
  for (const name of ["background.js", "content.js", "page-hook.js", "popup.js"]) {
    fs.copyFileSync(path.join(bundle, name), path.join(destination, name));
  }
  fs.copyFileSync(path.join(extensionRoot, "src", "popup", "popup.html"), path.join(destination, "popup.html"));
  fs.copyFileSync(path.join(extensionRoot, "src", "popup", "popup.css"), path.join(destination, "popup.css"));
  fs.cpSync(path.join(extensionRoot, "public", "icons"), path.join(destination, "icons"), { recursive: true });
  const manifest = merge(
    readJson(path.join(extensionRoot, "manifests", "manifest.base.json")),
    readJson(path.join(extensionRoot, "manifests", `manifest.${target}.json`)),
  );
  fs.writeFileSync(path.join(destination, "manifest.json"), `${JSON.stringify(manifest, null, 2)}\n`);
}

function zipTarget(target) {
  const release = path.join(projectRoot, "release");
  fs.mkdirSync(release, { recursive: true });
  const zip = path.join(release, `shareguard-${target}.zip`);
  fs.rmSync(zip, { force: true });
  const source = path.join(extensionRoot, "dist", target);
  execFileSync(
    "powershell",
    [
      "-NoProfile",
      "-Command",
      `Compress-Archive -Path '${source}\\*' -DestinationPath '${zip}' -Force`,
    ],
    { stdio: "inherit" },
  );
}

function removeLegacyFlatOutput() {
  const dist = path.join(extensionRoot, "dist");
  for (const name of ["background.js", "content.js", "page-hook.js", "popup.js", "popup.html", "popup.css", "manifest.json"]) {
    fs.rmSync(path.join(dist, name), { force: true });
  }
  fs.rmSync(path.join(dist, "icons"), { recursive: true, force: true });
}

function publish() {
  removeLegacyFlatOutput();
  for (const target of targets) copyBundle(target);
  if (!watch) {
    for (const target of targets) zipTarget(target);
  }
}

const context = await esbuild.context({
  absWorkingDir: extensionRoot,
  entryPoints: {
    background: "src/background/entry.ts",
    content: "src/content/content-script.ts",
    "page-hook": "src/page/display-media-hook.ts",
    popup: "src/popup/popup.ts",
  },
  bundle: true,
  outdir: path.join(extensionRoot, "dist", ".bundle"),
  format: "iife",
  target: "es2022",
  sourcemap: false,
  logLevel: "info",
  plugins: [
    {
      name: "publish-targets",
      setup(build) {
        build.onEnd((result) => {
          if (result.errors.length === 0) publish();
        });
      },
    },
  ],
});

if (watch) {
  await context.watch();
} else {
  await context.rebuild();
  await context.dispose();
}
