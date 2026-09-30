import { PAGE_CHANNEL } from "../shared/constants";
import { PAGE_EVENT, isPageRequest } from "../protocol/page-protocol";
import type { ExtensionPort } from "../platform/browser/BrowserAdapter";
import { browserAdapter } from "../platform/browser/adapters";

interface BridgeMessage {
  channel?: string;
  direction?: string;
  type?: string;
  requestId?: number;
  [key: string]: unknown;
}

let port: ExtensionPort | null = null;
let localSessions = 0;

function pageMessage(message: BridgeMessage): void {
  window.postMessage({ ...message, channel: PAGE_CHANNEL, direction: "extension" }, "*");
}

function onPortMessage(message: BridgeMessage): void {
  if (message.type === PAGE_EVENT.shareReady) localSessions += 1;
  if (message.type === PAGE_EVENT.audio && localSessions <= 0) return;
  pageMessage(message);
}

function ensurePort(): ExtensionPort {
  if (port) return port;
  port = browserAdapter().connect("content");
  port.onMessage.addListener((message: unknown) => {
    if (!message || typeof message !== "object") return;
    onPortMessageAndMaybeClose(message as BridgeMessage);
  });
  port.onDisconnect.addListener(() => {
    port = null;
    if (localSessions > 0) {
      localSessions = 0;
      pageMessage({ type: PAGE_EVENT.captureError });
    }
  });
  return port;
}

function releasePortIfIdle(messageType: string | undefined): void {
  if (
    localSessions === 0 &&
    (messageType === PAGE_EVENT.enabled ||
      messageType === "end-share" ||
      messageType === "share-failed" ||
      messageType === "share-skipped")
  ) {
    port?.disconnect();
    port = null;
  }
}

window.addEventListener("message", (event) => {
  if (event.source !== window) return;
  const data = event.data as BridgeMessage;
  if (!data || data.channel !== PAGE_CHANNEL || data.direction !== "page" || !data.type) return;
  if (!isPageRequest(data.type)) return;
  if (data.type === "end-share") localSessions = Math.max(0, localSessions - 1);
  ensurePort().postMessage(data);
  if (data.type === "end-share") releasePortIfIdle("end-share");
});

function onPortMessageAndMaybeClose(message: BridgeMessage): void {
  onPortMessage(message);
  releasePortIfIdle(message.type);
}
