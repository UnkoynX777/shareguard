import type { AudioRuntime } from "../audio/AudioRuntime";
import { CAPACITY_FRAMES, HIGH_WATER_FRAMES, LOW_WATER_FRAMES, PREBUFFER_FRAMES, SAMPLE_RATE } from "../shared/constants";
import { workletSource } from "../worklet/shareguard-audio-worklet";

class StereoRing {
  private readonly left = new Float32Array(CAPACITY_FRAMES);
  private readonly right = new Float32Array(CAPACITY_FRAMES);
  private read = 0;
  private write = 0;
  private available = 0;
  private started = false;
  private fade = 0;
  private dry = 0;

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
    const take = Math.min(this.available, frames);
    if (take < frames && this.available < LOW_WATER_FRAMES) {
      this.dry += 1;
      if (this.dry > 8) {
        this.started = false;
        this.dry = 0;
      }
    } else {
      this.dry = 0;
    }
    for (let frame = 0; frame < take; frame++) {
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
    this.available -= take;
    for (let frame = take; frame < frames; frame++) {
      left[frame] = 0;
      right[frame] = 0;
    }
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
  private pending = new Float32Array(0);
  private phase = 0;
  private expectedSequence = 0;
  private sequenceGaps = 0;
  private lastArrival = 0;
  private intervalTotal = 0;
  private jitterTotal = 0;
  private intervalCount = 0;
  private jitterMax = 0;
  private receivedFrames = 0;
  private workletUnderruns = 0;
  private workletOverruns = 0;
  private bufferedFrames = 0;
  private lastReport = 0;

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
      node.port.onmessage = (event: MessageEvent<{ type?: string; underruns?: number; overruns?: number; buffered?: number }>) => {
        if (event.data?.type !== "stats") return;
        this.workletUnderruns = event.data.underruns ?? 0;
        this.workletOverruns = event.data.overruns ?? 0;
        this.bufferedFrames = event.data.buffered ?? 0;
      };
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

  pushBase64(base64: string, sequence = 0): void {
    if (!this.source || this.tracks === 0) return;
    const now = performance.now();
    if (sequence > 0) {
      if (this.expectedSequence > 0 && sequence !== this.expectedSequence) {
        this.sequenceGaps += Math.max(1, sequence - this.expectedSequence);
      }
      this.expectedSequence = sequence + 1;
    }
    if (this.lastArrival > 0) {
      const interval = now - this.lastArrival;
      this.intervalTotal += interval;
      this.intervalCount += 1;
      const jitter = Math.abs(interval - 20);
      this.jitterTotal += jitter;
      if (jitter > this.jitterMax) this.jitterMax = jitter;
    }
    this.lastArrival = now;
    const samples = this.resample(decodeS16Le(base64));
    if (samples.length < 2) return;
    this.receivedFrames += samples.length >> 1;
    if (this.worklet) this.worklet.port.postMessage(samples);
    else this.ring.push(samples);
    this.report(now);
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
    this.pending = new Float32Array(0);
    this.phase = 0;
    this.expectedSequence = 0;
    this.jitterTotal = 0;
    const context = this.context;
    this.context = null;
    if (context && context.state !== "closed") await context.close().catch(() => undefined);
  }

  private report(now: number): void {
    if (this.lastReport !== 0 && now - this.lastReport < 2000) return;
    const seconds = this.lastReport === 0 ? 2 : (now - this.lastReport) / 1000;
    const received = Math.round(this.receivedFrames / seconds);
    const arrival = this.intervalCount > 0 ? this.intervalTotal / this.intervalCount : 0;
    const jitter = this.intervalCount > 0 ? this.jitterTotal / this.intervalCount : 0;
    console.debug(
      `ShareGuard audio received=${received}/s buffered=${Math.round((this.bufferedFrames * 1000) / SAMPLE_RATE)}ms arrival=${arrival.toFixed(1)}ms jitter=${jitter.toFixed(1)}ms jitterMax=${this.jitterMax.toFixed(1)}ms gaps=${this.sequenceGaps} underruns=${this.workletUnderruns} overruns=${this.workletOverruns}`,
    );
    this.receivedFrames = 0;
    this.intervalTotal = 0;
    this.jitterTotal = 0;
    this.intervalCount = 0;
    this.jitterMax = 0;
    this.lastReport = now;
  }

  private resample(interleaved: Float32Array): Float32Array {
    const rate = this.context?.sampleRate ?? SAMPLE_RATE;
    if (rate === SAMPLE_RATE || interleaved.length < 4) return interleaved;
    const merged = new Float32Array(this.pending.length + interleaved.length);
    merged.set(this.pending);
    merged.set(interleaved, this.pending.length);
    const inputFrames = merged.length >> 1;
    if (inputFrames < 2) {
      this.pending = merged;
      return new Float32Array(0);
    }
    const step = SAMPLE_RATE / rate;
    const output = new Float32Array(Math.ceil(inputFrames / step) * 2 + 2);
    let written = 0;
    let position = this.phase;
    while (position + 1 < inputFrames && written * 2 + 1 < output.length) {
      const index = Math.floor(position);
      const fraction = position - index;
      const left = merged[index * 2] ?? 0;
      const leftNext = merged[(index + 1) * 2] ?? left;
      const right = merged[index * 2 + 1] ?? left;
      const rightNext = merged[(index + 1) * 2 + 1] ?? right;
      output[written * 2] = left * (1 - fraction) + leftNext * fraction;
      output[written * 2 + 1] = right * (1 - fraction) + rightNext * fraction;
      written += 1;
      position += step;
    }
    const consumed = Math.min(inputFrames - 1, Math.floor(position));
    this.phase = position - consumed;
    this.pending = merged.slice(consumed * 2);
    return output.subarray(0, written * 2);
  }
}
