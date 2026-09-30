import { HOST_NAME } from "../shared/constants";
import type { NativeMessage } from "../shared/types";
import { isNativeMessage } from "../protocol/native-protocol";
import type { ExtensionPort } from "../platform/browser/BrowserAdapter";
import { browserAdapter } from "../platform/browser/adapters";

export type NativeListener = (message: NativeMessage) => void;

export class NativeConnection {
  private port: ExtensionPort | null = null;
  private readonly listeners = new Set<NativeListener>();
  private readonly disconnectListeners = new Set<(reason: string) => void>();

  connected(): boolean {
    return this.port !== null;
  }

  addListener(listener: NativeListener): void {
    this.listeners.add(listener);
  }

  onDisconnect(listener: (reason: string) => void): void {
    this.disconnectListeners.add(listener);
  }

  connect(): void {
    if (this.port) return;
    const port = browserAdapter().connectNative(HOST_NAME);
    this.port = port;
    port.onMessage.addListener((message: unknown) => {
      if (this.port !== port || !isNativeMessage(message)) return;
      for (const listener of this.listeners) listener(message);
    });
    port.onDisconnect.addListener(() => {
      if (this.port !== port) return;
      this.port = null;
      const reason = browserAdapter().lastDisconnectReason();
      for (const listener of this.disconnectListeners) listener(reason);
    });
  }

  send(message: object): void {
    this.connect();
    this.port?.postMessage(message);
  }

  disconnect(): void {
    const port = this.port;
    this.port = null;
    try {
      port?.disconnect();
    } catch {
      return;
    }
  }
}
