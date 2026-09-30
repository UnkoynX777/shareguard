import { FAILURE_MESSAGE, PAGE_CHANNEL, SAMPLE_RATE } from "../shared/constants";
import { PAGE_EVENT, PAGE_REQUEST } from "../protocol/page-protocol";
import { PageAudioRuntime } from "./audio-runtime";
import { invokeDisplayMedia, shouldReplaceAudio } from "./share-decision";

type GetDisplayMedia = (this: MediaDevices, constraints?: DisplayMediaStreamOptions) => Promise<MediaStream>;

interface PageMessage {
  channel?: string;
  direction?: string;
  type?: string;
  requestId?: number;
  enabled?: boolean;
  message?: string;
  data?: string;
}

const runtime = new PageAudioRuntime();
const pending = new Map<number, (message: PageMessage) => void>();
const protectedStreams = new Set<MediaStream>();
let nextRequestId = 1;
let enabledCache: boolean | null = null;
let shares = 0;
let prepared: Promise<AudioContext> | null = null;

function post(type: string, extra: Record<string, unknown> = {}): number {
  const requestId = nextRequestId++;
  window.postMessage({ channel: PAGE_CHANNEL, direction: "page", type, requestId, ...extra }, "*");
  return requestId;
}

function request(type: string, timeoutMs: number): Promise<PageMessage> {
  const requestId = post(type);
  return new Promise((resolve, reject) => {
    const timer = window.setTimeout(() => {
      pending.delete(requestId);
      reject(new Error("timeout"));
    }, timeoutMs);
    pending.set(requestId, (message) => {
      window.clearTimeout(timer);
      pending.delete(requestId);
      resolve(message);
    });
  });
}

function stripAudio(stream: MediaStream): void {
  for (const track of stream.getAudioTracks()) {
    stream.removeTrack(track);
    track.stop();
  }
}

function prepareContext(): Promise<AudioContext> {
  if (!prepared) {
    const context = new AudioContext({ sampleRate: SAMPLE_RATE, latencyHint: 0.15 });
    prepared = context.resume().then(() => context).catch(() => context);
  }
  return prepared;
}

async function discardPrepared(): Promise<void> {
  const pendingContext = prepared;
  prepared = null;
  if (!pendingContext || shares > 0) return;
  const context = await pendingContext.catch(() => null);
  if (context && shares === 0) await context.close().catch(() => undefined);
}

function watchShare(stream: MediaStream, track: MediaStreamTrack): void {
  let released = false;
  const release = () => {
    if (released) return;
    released = true;
    protectedStreams.delete(stream);
    shares = Math.max(0, shares - 1);
    track.stop();
    runtime.release();
    post("end-share");
    if (shares === 0) prepared = null;
  };
  track.addEventListener("ended", release);
  for (const video of stream.getVideoTracks()) video.addEventListener("ended", release);
}

async function protect(stream: MediaStream, contextPromise: Promise<AudioContext>): Promise<MediaStream> {
  const enabledMessage = await request(PAGE_REQUEST.getEnabled, 1500).catch(() => null);
  const enabled = enabledMessage ? enabledMessage.enabled !== false : enabledCache !== false;
  enabledCache = enabled;
  if (!shouldReplaceAudio(enabled, stream.getAudioTracks().length)) {
    await discardPrepared();
    return stream;
  }

  const started = await request(PAGE_REQUEST.beginShare, 16000).catch(() => null);
  if (started?.type === "share-skipped") {
    await discardPrepared();
    return stream;
  }
  if (!started || started.type !== PAGE_EVENT.shareReady) {
    await discardPrepared();
    stripAudio(stream);
    return stream;
  }

  try {
    if (!runtime.isActive()) {
      const context = await contextPromise;
      await runtime.attach(context);
      prepared = null;
    } else {
      await discardPrepared();
    }
    const track = await runtime.createTrack();
    stripAudio(stream);
    stream.addTrack(track);
    shares += 1;
    protectedStreams.add(stream);
    watchShare(stream, track);
    return stream;
  } catch {
    post("end-share");
    await runtime.close();
    await discardPrepared();
    stripAudio(stream);
    post(PAGE_REQUEST.protectFailed, { message: FAILURE_MESSAGE });
    return stream;
  }
}

function install(): void {
  const prototype = window.MediaDevices?.prototype;
  if (!prototype || typeof prototype.getDisplayMedia !== "function") return;
  const current = prototype.getDisplayMedia as GetDisplayMedia & { __shareguard?: boolean };
  if (current.__shareguard) return;
  const original = prototype.getDisplayMedia;
  const wrapped: GetDisplayMedia & { __shareguard?: boolean } = function (constraints) {
    if (enabledCache === false) return original.call(this, constraints);
    const contextPromise = prepareContext();
    return invokeDisplayMedia(original, this, constraints).then(
      (stream) => protect(stream, contextPromise),
      (error: unknown) => {
        void discardPrepared();
        return Promise.reject(error);
      },
    );
  };
  wrapped.__shareguard = true;
  Object.defineProperty(wrapped, "name", { value: "getDisplayMedia" });
  prototype.getDisplayMedia = wrapped;
  const devices = navigator.mediaDevices;
  if (devices && devices.getDisplayMedia !== wrapped) {
    try {
      devices.getDisplayMedia = wrapped;
    } catch {
      return;
    }
  }
}

window.addEventListener("message", (event) => {
  if (event.source !== window) return;
  const data = event.data as PageMessage;
  if (!data || data.channel !== PAGE_CHANNEL || data.direction !== "extension") return;
  if (data.type === PAGE_EVENT.audio && typeof data.data === "string" && shares > 0) {
    runtime.pushBase64(data.data);
    return;
  }
  if (data.type === PAGE_EVENT.captureError) {
    for (const stream of protectedStreams) stripAudio(stream);
    return;
  }
  if (data.requestId != null && pending.has(data.requestId)) pending.get(data.requestId)?.(data);
});

install();
window.addEventListener("DOMContentLoaded", install, { once: true });
void request(PAGE_REQUEST.getEnabled, 1500)
  .then((message) => {
    enabledCache = message.enabled !== false;
  })
  .catch(() => undefined);
