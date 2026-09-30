import esbuild from "esbuild";
import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { chromiumExtensionId } from "./extension-id.mjs";

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

function issDefine(name) {
  const iss = fs.readFileSync(path.join(projectRoot, "installer", "shareguard.iss"), "utf8");
  const match = new RegExp(`#define ${name} "([^"]+)"`).exec(iss);
  if (!match) throw new Error(`installer/shareguard.iss is missing #define ${name}`);
  return match[1];
}

function assertIdentity(target, manifest) {
  if (target === "chromium") {
    const expected = issDefine("ExtensionId");
    const actual = chromiumExtensionId(manifest.key);
    if (actual !== expected) {
      throw new Error(`Chromium extension ID ${actual} does not match allowed_origins ID ${expected}`);
    }
  }
  if (target === "firefox") {
    const expected = issDefine("FirefoxId");
    const actual = manifest.browser_specific_settings?.gecko?.id;
    if (actual !== expected) {
      throw new Error(`Firefox extension ID ${actual} does not match allowed_extensions ID ${expected}`);
    }
  }
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
  assertIdentity(target, manifest);
  fs.writeFileSync(path.join(destination, "manifest.json"), `${JSON.stringify(manifest, null, 2)}\n`);
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
