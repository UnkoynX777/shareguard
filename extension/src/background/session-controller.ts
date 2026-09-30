import { FAILURE_MESSAGE } from "../shared/constants";
import type { NativeMessage } from "../shared/types";
import type { ExtensionPort } from "../platform/browser/BrowserAdapter";
import { NativeConnection } from "./native-connection";

export interface ContentClient {
  port: ExtensionPort;
  sessions: number;
}

interface Pending {
  resolve: (message: NativeMessage) => void;
  reject: (error: Error) => void;
  accept: (message: NativeMessage) => boolean;
}

export class SessionController {
  private readonly clients = new Set<ContentClient>();
  private readonly pending: Pending[] = [];
  private capturing = false;
  private starting: Promise<void> | null = null;
  private lastError = "";

  constructor(
    private readonly native: NativeConnection,
    private readonly onChange: () => void,
  ) {
    native.addListener((message) => this.onNative(message));
  }

  clientCount(): number {
    return this.clients.size;
  }

  sharing(): boolean {
    return this.totalSessions() > 0;
  }

  isCapturing(): boolean {
    return this.capturing;
  }

  error(): string {
    return this.lastError;
  }

  addClient(port: ExtensionPort): ContentClient {
    const client: ContentClient = { port, sessions: 0 };
    this.clients.add(client);
    port.onDisconnect.addListener(() => {
      const hadSessions = client.sessions;
      this.clients.delete(client);
      if (hadSessions > 0 && this.totalSessions() === 0) this.stopCapture();
      this.onChange();
    });
    return client;
  }

  async begin(client: ContentClient): Promise<void> {
    await this.ensureCapture();
    client.sessions += 1;
    this.onChange();
  }

  end(client: ContentClient): void {
    if (client.sessions > 0) client.sessions -= 1;
    if (this.totalSessions() === 0) this.stopCapture();
    this.onChange();
  }

  failClosed(message: string): void {
    this.lastError = message;
    this.capturing = false;
    this.broadcast({ type: "capture-error", message });
    this.onChange();
  }

  forwardAudio(data: string, sequence = 0): void {
    if (!this.capturing) return;
    for (const client of this.clients) {
      if (client.sessions > 0) client.port.postMessage({ type: "audio", data, sequence });
    }
  }

  waitFor(accept: (message: NativeMessage) => boolean, timeoutMs: number): Promise<NativeMessage> {
    return new Promise((resolve, reject) => {
      const pending: Pending = {
        resolve: (message) => {
          clearTimeout(timer);
          resolve(message);
        },
        reject: (error) => {
          clearTimeout(timer);
          reject(error);
        },
        accept,
      };
      const timer = setTimeout(() => {
        const index = this.pending.indexOf(pending);
        if (index >= 0) this.pending.splice(index, 1);
        reject(new Error(FAILURE_MESSAGE));
      }, timeoutMs);
      this.pending.push(pending);
    });
  }

  private totalSessions(): number {
    let count = 0;
    for (const client of this.clients) count += client.sessions;
    return count;
  }

  private ensureCapture(): Promise<void> {
    if (this.capturing) return Promise.resolve();
    if (!this.starting) {
      this.starting = this.startCapture().finally(() => {
        this.starting = null;
      });
    }
    return this.starting;
  }

  private async startCapture(): Promise<void> {
    this.native.send({ type: "START_CAPTURE" });
    const message = await this.waitFor(
      (item) => item.type === "CAPTURE_STARTED" || item.type === "ERROR",
      15000,
    );
    if (message.type === "ERROR") {
      this.lastError = message.message || FAILURE_MESSAGE;
      throw new Error(this.lastError);
    }
    this.capturing = true;
    this.lastError = "";
  }

  stopCapture(): void {
    this.capturing = false;
    try {
      if (this.native.connected()) this.native.send({ type: "STOP_CAPTURE" });
    } catch {
      return;
    }
  }

  private onNative(message: NativeMessage): void {
    if (message.type === "AUDIO_FRAME") {
      this.forwardAudio(message.data, message.sequence);
      return;
    }
    if (message.type === "CAPTURE_STOPPED") this.capturing = false;
    if (message.type === "ERROR" && this.sharing()) {
      this.lastError = message.message || FAILURE_MESSAGE;
      this.capturing = false;
      this.broadcast({ type: "capture-error", message: this.lastError, code: message.code });
    }
    const waiting = this.pending.splice(0);
    const unmatched: Pending[] = [];
    for (const waiter of waiting) {
      if (waiter.accept(message)) waiter.resolve(message);
      else unmatched.push(waiter);
    }
    this.pending.push(...unmatched);
    if (message.type === "ERROR" || message.type === "CAPTURE_STARTED" || message.type === "CAPTURE_STOPPED") {
      this.onChange();
    }
  }

  private broadcast(message: object): void {
    for (const client of this.clients) {
      if (client.sessions > 0) client.port.postMessage(message);
    }
  }
}
