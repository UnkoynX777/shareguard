import type { ExtensionRules, PopupSnapshot, ProcessView } from "../shared/types";
import { browserAdapter } from "../platform/browser/adapters";

const protection = document.querySelector<HTMLElement>("#protection");
const dot = document.querySelector<HTMLElement>("#dot");
const nativeLabel = document.querySelector<HTMLElement>("#native");
const sharingLabel = document.querySelector<HTMLElement>("#sharing");
const blockedLabel = document.querySelector<HTMLElement>("#blocked");
const message = document.querySelector<HTMLElement>("#message");
const list = document.querySelector<HTMLElement>("#list");
const search = document.querySelector<HTMLInputElement>("#search");
const protectionToggle = document.querySelector<HTMLInputElement>("#protection-toggle");
const showAll = document.querySelector<HTMLInputElement>("#show-all");
const guide = document.querySelector<HTMLElement>("#guide");

let current: PopupSnapshot | null = null;
const port = browserAdapter().connect("popup");

function matches(process: ProcessView, query: string): boolean {
  if (!query) return true;
  return process.name.toLowerCase().includes(query) || process.executable.toLowerCase().includes(query);
}

function section(title: string, apps: ProcessView[], blocked: Set<string>, query: string): void {
  const visible = apps.filter((app) => matches(app, query));
  if (!list || visible.length === 0) return;
  const heading = document.createElement("div");
  heading.className = "section";
  heading.textContent = title;
  list.append(heading);
  for (const app of visible) {
    const row = document.createElement("div");
    row.className = "row";
    const label = document.createElement("div");
    const name = document.createElement("div");
    name.className = "name";
    name.textContent = app.name || app.executable;
    const exe = document.createElement("div");
    exe.className = "exe";
    exe.textContent = app.executable;
    label.append(name, exe);
    const button = document.createElement("button");
    const isBlocked = blocked.has(app.id);
    button.className = isBlocked ? "block" : "allow";
    button.textContent = isBlocked ? "Blocked" : "Allow";
    button.addEventListener("click", () => toggle(app.id));
    row.append(label, button);
    list.append(row);
  }
}

function render(): void {
  if (!current?.rules || !current.processes || !current.status) return;
  if (!list || !protection || !dot || !nativeLabel || !sharingLabel || !blockedLabel || !message) return;
  const rules = current.rules;
  const blocked = new Set(rules.blockedApplications);
  const runningIds = new Set(current.processes.map((process) => process.id));
  const query = search?.value.trim().toLowerCase() ?? "";
  const playing = current.processes.filter((process) => process.audioActive);
  const blockedOpen = current.processes.filter((process) => !process.audioActive && blocked.has(process.id));
  const others = current.processes.filter((process) => !process.audioActive && !blocked.has(process.id));
  const closed = rules.blockedApplications
    .filter((id) => !runningIds.has(id))
    .map((id) => ({ id, rootPid: 0, name: id, executable: id, audioActive: false, processCount: 0 }));

  protection.textContent = rules.protectionEnabled ? "Protection enabled" : "Protection disabled";
  dot.dataset.tone = !rules.protectionEnabled ? "off" : current.status.lastError ? "warn" : "ok";
  nativeLabel.textContent = current.status.nativeConnected ? "Connected" : "Unavailable";
  sharingLabel.textContent = current.status.sharing ? "Active" : "Not sharing";
  blockedLabel.textContent = String(rules.blockedApplications.length);
  message.textContent = current.status.lastError;
  if (guide) guide.hidden = current.status.nativeConnected;
  if (protectionToggle) protectionToggle.checked = rules.protectionEnabled;
  if (showAll) showAll.checked = rules.showAllProcesses;
  list.replaceChildren();
  section("Playing audio", playing, blocked, query);
  section("Blocked and open", blockedOpen, blocked, query);
  section("Other running apps", others, blocked, query);
  section("Blocked apps", closed, blocked, query);
  if (!list.childElementCount) {
    const empty = document.createElement("p");
    empty.className = "empty";
    empty.textContent = current.status.nativeConnected ? "No matching applications" : "Native helper not installed";
    list.append(empty);
  }
}

function sendRules(next: ExtensionRules): void {
  port.postMessage({ type: "set-rules", rules: next });
}

function toggle(id: string): void {
  if (!current) return;
  const blocked = new Set(current.rules.blockedApplications);
  if (blocked.has(id)) blocked.delete(id);
  else blocked.add(id);
  sendRules({ ...current.rules, blockedApplications: [...blocked] });
}

protectionToggle?.addEventListener("change", () => {
  if (!current || !protectionToggle) return;
  sendRules({ ...current.rules, protectionEnabled: protectionToggle.checked });
});

showAll?.addEventListener("change", () => {
  if (!current || !showAll) return;
  sendRules({ ...current.rules, showAllProcesses: showAll.checked });
});

search?.addEventListener("input", render);

port.onMessage.addListener((message: unknown) => {
  if (!message || typeof message !== "object") return;
  const stateMessage = message as { type?: string; state?: PopupSnapshot };
  if (stateMessage.type === "state" && stateMessage.state) {
    current = stateMessage.state;
    render();
  }
});

port.postMessage({ type: "get-state" });
