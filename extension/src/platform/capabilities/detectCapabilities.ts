import type { BrowserCapabilities } from "../browser/BrowserAdapter";

interface RuntimeProbe {
  connectNative?: unknown;
  getBrowserInfo?: unknown;
}

interface NamespaceProbe {
  runtime?: RuntimeProbe;
  offscreen?: unknown;
}

function namespaces(): { browser?: NamespaceProbe; chrome?: NamespaceProbe } {
  const root = globalThis as { browser?: NamespaceProbe; chrome?: NamespaceProbe };
  return { browser: root.browser, chrome: root.chrome };
}

export function browserFamily(): "chromium" | "firefox" {
  const runtime = namespaces().browser?.runtime;
  if (typeof runtime?.getBrowserInfo === "function") return "firefox";
  return "chromium";
}

export function detectCapabilities(): BrowserCapabilities {
  const { browser, chrome } = namespaces();
  const runtime = browser?.runtime ?? chrome?.runtime;
  const media = (globalThis as { navigator?: { mediaDevices?: { getDisplayMedia?: unknown } } }).navigator?.mediaDevices;
  const workerScope = (globalThis as { ServiceWorkerGlobalScope?: abstract new () => object }).ServiceWorkerGlobalScope;
  const inServiceWorker = typeof workerScope === "function" && globalThis instanceof workerScope;
  return {
    nativeMessaging: typeof runtime?.connectNative === "function",
    mainWorldInjection: true,
    backgroundServiceWorker: inServiceWorker,
    backgroundDocument: typeof globalThis.document !== "undefined" && !inServiceWorker,
    offscreenDocument: browser?.offscreen != null || chrome?.offscreen != null,
    webAudio: typeof AudioContext === "function",
    audioWorklet: typeof AudioWorkletNode === "function",
    displayMedia: typeof media?.getDisplayMedia === "function",
  };
}
