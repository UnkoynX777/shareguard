import { readdirSync, readFileSync, statSync } from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "..");
const exceptions = new Set([".github/PULL_REQUEST_TEMPLATE.md"]);
const skipDirectories = new Set(["node_modules", "dist", "build", "release", ".git"]);

function relative(file) {
  return path.relative(root, file).split(path.sep).join("/");
}

function walk(directory, files) {
  for (const entry of readdirSync(directory, { withFileTypes: true })) {
    if (skipDirectories.has(entry.name)) continue;
    const full = path.join(directory, entry.name);
    if (entry.isDirectory()) walk(full, files);
    else if (entry.name.endsWith(".md")) files.push(full);
  }
}

function translationPath(source) {
  return source.slice(0, -3) + ".pt-BR.md";
}

function englishPath(source) {
  return source.replace(/\.pt-BR\.md$/, ".md");
}

function baseName(source) {
  return path.posix.basename(source);
}

function readHead(file) {
  return readFileSync(file, "utf8").split(/\r?\n/).slice(0, 20).join("\n");
}

function stripFences(text) {
  return text.replace(/```[\s\S]*?```/g, "");
}

function linksIn(text) {
  const found = [];
  const pattern = /!\[[^\]]*\]\(([^)]+)\)|\[[^\]]*\]\(([^)]+)\)/g;
  for (const match of stripFences(text).matchAll(pattern)) {
    found.push((match[1] || match[2] || "").trim());
  }
  return found;
}

function isRemote(url) {
  return /^(https?:|mailto:)/i.test(url) || url.startsWith("#") || url.length === 0;
}

const files = [];
walk(root, files);
const markdown = files.map(relative).sort();
const errors = [];

for (const source of markdown) {
  if (source.endsWith(".pt-BR.md")) {
    const english = englishPath(source);
    if (!markdown.includes(english)) {
      errors.push(`Portuguese file has no English source: ${english}`);
    }
    continue;
  }
  if (exceptions.has(source)) continue;

  const translated = translationPath(source);
  const englishFile = path.join(root, source);
  const translatedFile = path.join(root, translated);
  if (!markdown.includes(translated)) {
    errors.push(`Missing PT-BR translation: ${translated}`);
    continue;
  }

  const englishHead = readHead(englishFile);
  const translatedHead = readHead(translatedFile);
  const englishSwitch = `English | [Português (Brasil)](./${baseName(translated)})`;
  const portugueseSwitch = `[English](./${baseName(source)}) | Português (Brasil)`;
  if (!englishHead.includes(englishSwitch)) {
    errors.push(`Missing language switcher: ${source}`);
  }
  if (!translatedHead.includes(portugueseSwitch)) {
    errors.push(`Missing language switcher: ${translated}`);
  }
}

for (const source of markdown) {
  if (exceptions.has(source)) continue;
  const file = path.join(root, source);
  const text = readFileSync(file, "utf8");
  for (const url of linksIn(text)) {
    if (isRemote(url)) continue;
    const target = decodeURIComponent(url.split("#")[0].split("?")[0]);
    if (target.length === 0) continue;
    const resolved = path.resolve(path.dirname(file), target);
    try {
      statSync(resolved);
    } catch {
      errors.push(`Broken link in ${source}: ${url}`);
    }
  }
}

if (errors.length > 0) {
  for (const error of errors) console.error(error);
  process.exit(1);
}

console.log("Documentation check passed.");
