import { FAILURE_MESSAGE, UNSUPPORTED_BROWSER_MESSAGE } from "../shared/constants";
import type { ExtensionPort } from "../platform/browser/BrowserAdapter";
import { browserAdapter } from "../platform/browser/adapters";
import { detectCapabilities } from "../platform/capabilities/detectCapabilities";
import type { ExtensionRules, PopupSnapshot, ShareStatus } from "../shared/types";
import { helloMessage, policyMessage, snapshotRequest } from "../protocol/native-protocol";
import { loadRules, saveRules } from "../storage/rules-store";
import { createStatus } from "../state/extension-state";
import { NativeConnection } from "./native-connection";
import { ProcessStore } from "./process-store";
import { SessionController } from "./session-controller";

const native = new NativeConnection();
const processes = new ProcessStore();
let rules: ExtensionRules = { protectionEnabled: true, blockedApplications: [], showAllProcesses: false };
let status: ShareStatus = createStatus(rules, false, false);
let ready: Promise<void> | null = null;
let snapshotSeen = false;
let lastNotifyAt = 0;
const popups = new Set<ExtensionPort>();

const session = new SessionController(native, () => {
  status.sharing = session.sharing();
  status.capturing = session.isCapturing();
  status.lastError = session.error();
  publish();
});

native.addListener((message) => {
  if (message.type === "PROCESS_SNAPSHOT") {
    snapshotSeen = true;
    processes.replace(message.processes);
    publish();
  } else if (message.type === "PROCESS_DIFF") {
    processes.apply(message);
    publish();
  } else if (message.type === "AUDIO_POLICY_APPLIED") {
    status.captureStrategy = message.captureStrategy;
    status.blockedCount = message.blockedCount;
    publish();
  } else if (message.type === "ERROR") {
    status.lastError = message.message;
    if (session.sharing()) notify(message.message);
    publish();
  } else if (message.type === "CAPTURE_STARTED") {
    status.captureStrategy = message.captureStrategy;
    status.capturing = true;
    status.lastError = "";
    clearBadge();
    publish();
  }
});

native.onDisconnect((reason) => {
  ready = null;
  snapshotSeen = false;
  status.nativeConnected = false;
  status.capturing = false;
  const installed = /not found|forbidden|Access denied|invalid/i.test(reason);
  status.lastError = installed ? "Native helper unavailable" : reason;
  if (session.sharing()) {
    session.failClosed(status.lastError || FAILURE_MESSAGE);
    notify(status.lastError);
  }
  publish();
});

function snapshot(): PopupSnapshot {
  status.nativeConnected = native.connected();
  status.protectionEnabled = rules.protectionEnabled;
  status.sharing = session.sharing();
  status.blockedCount = rules.blockedApplications.length;
  return { rules, processes: processes.list(), status };
}

function publish(): void {
  const state = snapshot();
  for (const port of popups) port.postMessage({ type: "state", state });
}

function notify(message: string): void {
  const now = Date.now();
  if (now - lastNotifyAt < 5000) return;
  lastNotifyAt = now;
  const adapter = browserAdapter();
  void adapter.setBadge("!", "#9f1239");
  void adapter.notify("shareguard-protection", "ShareGuard", message || FAILURE_MESSAGE);
}

function clearBadge(): void {
  void browserAdapter().setBadge("");
}

async function handshake(): Promise<void> {
  snapshotSeen = false;
  native.connect();
  const adapter = browserAdapter();
  native.send(helloMessage(adapter.family(), adapter.extensionVersion()));
  const ack = await session.waitFor((message) => message.type === "HELLO_ACK" || message.type === "ERROR", 8000);
  if (ack.type === "ERROR") {
    status.lastError = ack.message;
    throw new Error(ack.code === "PROTOCOL_VERSION_MISMATCH" ? ack.message : ack.message || FAILURE_MESSAGE);
  }
  if (!snapshotSeen) {
    await session.waitFor((message) => message.type === "PROCESS_SNAPSHOT" || message.type === "ERROR", 8000);
  }
  native.send(policyMessage(rules));
  const applied = await session.waitFor(
    (message) => message.type === "AUDIO_POLICY_APPLIED" || message.type === "ERROR",
    8000,
  );
  if (applied.type === "ERROR") throw new Error(applied.message || FAILURE_MESSAGE);
  status.nativeConnected = true;
  status.lastError = "";
  if (rules.showAllProcesses) native.send(snapshotRequest(true));
}

function ensureReady(): Promise<void> {
  if (ready && native.connected()) return ready;
  ready = handshake().catch((error: unknown) => {
    ready = null;
    status.nativeConnected = native.connected();
    throw error;
  });
  return ready;
}

async function applyRules(next: ExtensionRules): Promise<void> {
  rules = {
    protectionEnabled: next.protectionEnabled,
    blockedApplications: [...new Set(next.blockedApplications.map((name) => name.toLowerCase()))],
    showAllProcesses: next.showAllProcesses,
  };
  await saveRules(rules);
  if (native.connected()) {
    native.send(policyMessage(rules));
    native.send(snapshotRequest(rules.showAllProcesses));
  }
  publish();
}

export function startBackground(): void {
  const capabilities = detectCapabilities();
  if (!capabilities.nativeMessaging) {
    status.lastError =
      browserAdapter().family() === "firefox" ? UNSUPPORTED_BROWSER_MESSAGE : "Native helper unavailable";
  }
  browserAdapter().onConnect((port) => {
    if (port.name === "popup") {
      popups.add(port);
      void ensureReady().then(publish).catch(() => publish());
      port.onDisconnect.addListener(() => {
        popups.delete(port);
        if (popups.size === 0 && !session.sharing() && !session.isCapturing()) {
          ready = null;
          native.disconnect();
        }
      });
      port.onMessage.addListener((message: unknown) => {
        if (!message || typeof message !== "object") return;
        const popupMessage = message as { type?: string; rules?: ExtensionRules };
        if (popupMessage.type === "get-state") port.postMessage({ type: "state", state: snapshot() });
        if (popupMessage.type === "set-rules" && popupMessage.rules) void applyRules(popupMessage.rules);
      });
      port.postMessage({ type: "state", state: snapshot() });
      return;
    }

    if (port.name !== "content") return;
    const client = session.addClient(port);
    port.onMessage.addListener((message: unknown) => {
      if (!message || typeof message !== "object") return;
      const contentMessage = message as { type?: string; requestId?: number; message?: string };
      if (contentMessage.type === "get-enabled") {
        port.postMessage({ type: "enabled", requestId: contentMessage.requestId, enabled: rules.protectionEnabled });
        return;
      }
      if (contentMessage.type === "begin-share") {
        void (async () => {
          try {
            await ensureReady();
            if (!rules.protectionEnabled) {
              port.postMessage({ type: "share-skipped", requestId: contentMessage.requestId });
              return;
            }
            await session.begin(client);
            port.postMessage({ type: "share-ready", requestId: contentMessage.requestId });
            publish();
          } catch (error) {
            const text = error instanceof Error ? error.message : FAILURE_MESSAGE;
            notify(text);
            try {
              port.postMessage({ type: "share-failed", requestId: contentMessage.requestId, message: text });
            } catch {
              return;
            }
            publish();
          }
        })();
        return;
      }
      if (contentMessage.type === "end-share") {
        session.end(client);
        if (!session.sharing() && popups.size === 0) {
          ready = null;
          native.disconnect();
        }
        return;
      }
      if (contentMessage.type === "protect-failed") notify(contentMessage.message || FAILURE_MESSAGE);
    });
  });

  void loadRules().then((loaded) => {
    rules = loaded;
    status = createStatus(rules, false, false);
    if (!capabilities.nativeMessaging) {
      status.lastError =
        browserAdapter().family() === "firefox" ? UNSUPPORTED_BROWSER_MESSAGE : "Native helper unavailable";
    }
  });
}
