import type { AudioRuntime } from "../audio/AudioRuntime";
import { CAPACITY_FRAMES, HIGH_WATER_FRAMES, PREBUFFER_FRAMES, SAMPLE_RATE } from "../shared/constants";
import { workletSource } from "../worklet/shareguard-audio-worklet";

class StereoRing {
  private readonly left = new Float32Array(CAPACITY_FRAMES);
  private readonly right = new Float32Array(CAPACITY_FRAMES);
  private read = 0;
  private write = 0;
  private available = 0;
  private started = false;
  private fade = 0;

  push(interleaved: Float32Array): void {
    const frames = interleaved.length >> 1;
    const overflow = this.available + frames - CAPACITY_FRAMES;
    if (overflow > 0) {
      this.read = (this.read + overflow) % CAPACITY_FRAMES;
      this.available -= overflow;
    }
    for (let frame = 0; frame < frames; frame++) {
      this.left[this.write] = interleaved[frame * 2] ?? 0;
      this.right[this.write] = interleaved[frame * 2 + 1] ?? 0;
      this.write = (this.write + 1) % CAPACITY_FRAMES;
    }
    this.available += frames;
    if (this.available > HIGH_WATER_FRAMES) {
      const drop = this.available - PREBUFFER_FRAMES;
      this.read = (this.read + drop) % CAPACITY_FRAMES;
      this.available -= drop;
    }
  }

  pull(left: Float32Array, right: Float32Array): void {
    const frames = left.length;
    if (!this.started) {
      if (this.available < PREBUFFER_FRAMES) {
        left.fill(0);
        right.fill(0);
        return;
      }
      this.started = true;
      this.fade = 64;
    }
    if (this.available < frames) {
      left.fill(0);
      right.fill(0);
      if (this.available === 0) this.started = false;
      return;
    }
    for (let frame = 0; frame < frames; frame++) {
      let leftSample = this.left[this.read] ?? 0;
      let rightSample = this.right[this.read] ?? 0;
      if (this.fade > 0) {
        const gain = (64 - this.fade) / 64;
        leftSample *= gain;
        rightSample *= gain;
        this.fade -= 1;
      }
      left[frame] = leftSample;
      right[frame] = rightSample;
      this.read = (this.read + 1) % CAPACITY_FRAMES;
    }
    this.available -= frames;
  }
}

export function decodeS16Le(base64: string): Float32Array {
  const binary = atob(base64);
  const bytes = new Uint8Array(binary.length);
  for (let index = 0; index < binary.length; index++) bytes[index] = binary.charCodeAt(index);
  const view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
  const samples = Math.floor(bytes.length / 2);
  const output = new Float32Array(samples);
  for (let index = 0; index < samples; index++) output[index] = view.getInt16(index * 2, true) / 32768;
  return output;
}

export class PageAudioRuntime implements AudioRuntime {
  private context: AudioContext | null = null;
  private source: AudioNode | null = null;
  private worklet: AudioWorkletNode | null = null;
  private processor: ScriptProcessorNode | null = null;
  private ring = new StereoRing();
  private destinations: MediaStreamAudioDestinationNode[] = [];
  private tracks = 0;

  isActive(): boolean {
    return this.context !== null && this.source !== null;
  }

  async attach(context: AudioContext): Promise<void> {
    if (this.context === context && this.source) return;
    await this.close();
    this.context = context;
    if (context.state === "suspended") await context.resume().catch(() => undefined);
    try {
      const blob = new Blob([workletSource], { type: "text/javascript" });
      const url = URL.createObjectURL(blob);
      try {
        await context.audioWorklet.addModule(url);
      } finally {
        URL.revokeObjectURL(url);
      }
      const node = new AudioWorkletNode(context, "shareguard-pcm", {
        numberOfInputs: 0,
        numberOfOutputs: 1,
        outputChannelCount: [2],
      });
      this.worklet = node;
      this.source = node;
    } catch {
      const processor = context.createScriptProcessor(4096, 1, 2);
      processor.onaudioprocess = (event) => {
        const right = event.outputBuffer.numberOfChannels > 1 ? event.outputBuffer.getChannelData(1) : null;
        this.ring.pull(event.outputBuffer.getChannelData(0), right ?? event.outputBuffer.getChannelData(0));
      };
      const mute = context.createGain();
      mute.gain.value = 0;
      processor.connect(mute);
      mute.connect(context.destination);
      this.processor = processor;
      this.source = processor;
    }
  }

  async createTrack(): Promise<MediaStreamTrack> {
    if (!this.context || !this.source) {
      const context = new AudioContext({ sampleRate: SAMPLE_RATE, latencyHint: 0.15 });
      await this.attach(context);
    }
    if (!this.context || !this.source) throw new Error("Audio engine is unavailable.");
    if (this.context.state === "suspended") await this.context.resume().catch(() => undefined);
    const destination = this.context.createMediaStreamDestination();
    this.source.connect(destination);
    this.destinations.push(destination);
    this.tracks += 1;
    const track = destination.stream.getAudioTracks()[0];
    if (!track) throw new Error("Protected audio track was not created.");
    track.contentHint = "music";
    return track;
  }

  pushBase64(base64: string): void {
    if (!this.source || this.tracks === 0) return;
    const samples = this.resample(decodeS16Le(base64));
    if (this.worklet) {
      this.worklet.port.postMessage(samples);
      return;
    }
    this.ring.push(samples);
  }

  release(): void {
    this.tracks = Math.max(0, this.tracks - 1);
    if (this.tracks === 0) void this.close();
  }

  async close(): Promise<void> {
    this.worklet?.disconnect();
    this.processor?.disconnect();
    for (const destination of this.destinations) destination.disconnect();
    this.destinations = [];
    this.worklet = null;
    this.processor = null;
    this.source = null;
    this.tracks = 0;
    this.ring = new StereoRing();
    const context = this.context;
    this.context = null;
    if (context && context.state !== "closed") await context.close().catch(() => undefined);
  }

  private resample(interleaved: Float32Array): Float32Array {
    const rate = this.context?.sampleRate ?? SAMPLE_RATE;
    if (rate === SAMPLE_RATE || interleaved.length < 4) return interleaved;
    const inputFrames = interleaved.length >> 1;
    const outputFrames = Math.max(1, Math.round((inputFrames * rate) / SAMPLE_RATE));
    const output = new Float32Array(outputFrames * 2);
    for (let frame = 0; frame < outputFrames; frame++) {
      const position = (frame * SAMPLE_RATE) / rate;
      const index = Math.min(inputFrames - 1, Math.floor(position));
      const next = Math.min(inputFrames - 1, index + 1);
      const fraction = position - index;
      const left = interleaved[index * 2] ?? 0;
      const leftNext = interleaved[next * 2] ?? left;
      const right = interleaved[index * 2 + 1] ?? left;
      const rightNext = interleaved[next * 2 + 1] ?? right;
      output[frame * 2] = left * (1 - fraction) + leftNext * fraction;
      output[frame * 2 + 1] = right * (1 - fraction) + rightNext * fraction;
    }
    return output;
  }
}
