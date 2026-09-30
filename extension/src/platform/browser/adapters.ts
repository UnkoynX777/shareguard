import type { BrowserAdapter, BrowserFamily, ExtensionPort } from "./BrowserAdapter";
import { browserFamily } from "../capabilities/detectCapabilities";

interface StorageArea {
  get(key: string): Promise<Record<string, unknown>>;
  set(items: Record<string, unknown>): Promise<void>;
}

interface ActionApi {
  setBadgeText(details: { text: string }): Promise<void> | void;
  setBadgeBackgroundColor(details: { color: string }): Promise<void> | void;
}

interface NotificationApi {
  create(id: string, options: { type: "basic"; iconUrl: string; title: string; message: string; priority: number }): Promise<string> | void;
}

interface RuntimeApi {
  connect(connectInfo: { name: string }): ExtensionPort;
  connectNative(host: string): ExtensionPort;
  onConnect: { addListener(listener: (port: ExtensionPort) => void): void };
  getURL(path: string): string;
  getManifest(): { version: string };
  lastError?: { message?: string };
}

interface WebExtensionNamespace {
  runtime: RuntimeApi;
  storage: { local: StorageArea };
  action: ActionApi;
  notifications: NotificationApi;
}

class WebExtensionAdapter implements BrowserAdapter {
  constructor(
    private readonly namespace: WebExtensionNamespace,
    private readonly familyName: BrowserFamily,
  ) {}

  family(): BrowserFamily {
    return this.familyName;
  }

  connectNative(host: string): ExtensionPort {
    return this.namespace.runtime.connectNative(host);
  }

  connect(name: string): ExtensionPort {
    return this.namespace.runtime.connect({ name });
  }

  onConnect(listener: (port: ExtensionPort) => void): void {
    this.namespace.runtime.onConnect.addListener(listener);
  }

  async getStorage(key: string): Promise<unknown> {
    const stored = await this.namespace.storage.local.get(key);
    return stored[key];
  }

  async setStorage(key: string, value: unknown): Promise<void> {
    await this.namespace.storage.local.set({ [key]: value });
  }

  getRuntimeUrl(path: string): string {
    return this.namespace.runtime.getURL(path);
  }

  extensionVersion(): string {
    return this.namespace.runtime.getManifest().version;
  }

  async setBadge(text: string, color?: string): Promise<void> {
    const textResult = this.namespace.action.setBadgeText({ text });
    if (isPromise(textResult)) await textResult;
    if (!color) return;
    const colorResult = this.namespace.action.setBadgeBackgroundColor({ color });
    if (isPromise(colorResult)) await colorResult;
  }

  async notify(id: string, title: string, message: string): Promise<void> {
    const result = this.namespace.notifications.create(id, {
      type: "basic",
      iconUrl: this.getRuntimeUrl("icons/icon128.png"),
      title,
      message,
      priority: 2,
    });
    if (isPromise(result)) await result;
  }

  lastDisconnectReason(): string {
    return this.namespace.runtime.lastError?.message ?? "Native helper disconnected";
  }
}

function isPromise(value: unknown): value is Promise<unknown> {
  return typeof value === "object" && value !== null && "then" in value;
}

function namespaceFor(family: BrowserFamily): WebExtensionNamespace {
  const root = globalThis as { browser?: WebExtensionNamespace; chrome?: WebExtensionNamespace };
  if (family === "firefox" && root.browser) return root.browser;
  if (!root.chrome) throw new Error("WebExtension runtime is unavailable.");
  return root.chrome;
}

export class ChromiumAdapter extends WebExtensionAdapter {
  constructor() {
    super(namespaceFor("chromium"), "chromium");
  }
}

export class FirefoxAdapter extends WebExtensionAdapter {
  constructor() {
    super(namespaceFor("firefox"), "firefox");
  }
}

let adapter: BrowserAdapter | null = null;

export function browserAdapter(): BrowserAdapter {
  if (!adapter) adapter = browserFamily() === "firefox" ? new FirefoxAdapter() : new ChromiumAdapter();
  return adapter;
}
