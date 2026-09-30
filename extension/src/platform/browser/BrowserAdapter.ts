export type BrowserFamily = "chromium" | "firefox";

export interface ExtensionPort {
  name: string;
  postMessage(message: unknown): void;
  disconnect(): void;
  onMessage: { addListener(listener: (message: unknown) => void): void };
  onDisconnect: { addListener(listener: () => void): void };
}

export interface BrowserCapabilities {
  nativeMessaging: boolean;
  mainWorldInjection: boolean;
  backgroundServiceWorker: boolean;
  backgroundDocument: boolean;
  offscreenDocument: boolean;
  webAudio: boolean;
  audioWorklet: boolean;
  displayMedia: boolean;
}

export interface BrowserAdapter {
  family(): BrowserFamily;
  connectNative(host: string): ExtensionPort;
  connect(name: string): ExtensionPort;
  onConnect(listener: (port: ExtensionPort) => void): void;
  getStorage(key: string): Promise<unknown>;
  setStorage(key: string, value: unknown): Promise<void>;
  getRuntimeUrl(path: string): string;
  extensionVersion(): string;
  setBadge(text: string, color?: string): Promise<void>;
  notify(id: string, title: string, message: string): Promise<void>;
  lastDisconnectReason(): string;
}
