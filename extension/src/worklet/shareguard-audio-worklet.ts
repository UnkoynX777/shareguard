import { CAPACITY_FRAMES, HIGH_WATER_FRAMES, LOW_WATER_FRAMES, PREBUFFER_FRAMES } from "../shared/constants";

export const workletSource = `
class ShareGuardProcessor extends AudioWorkletProcessor {
  constructor() {
    super();
    this.capacity = ${CAPACITY_FRAMES};
    this.left = new Float32Array(this.capacity);
    this.right = new Float32Array(this.capacity);
    this.read = 0;
    this.write = 0;
    this.available = 0;
    this.started = false;
    this.fade = 0;
    this.underruns = 0;
    this.overruns = 0;
    this.callbacks = 0;
    this.dry = 0;
    this.port.onmessage = (event) => {
      const samples = event.data;
      if (!(samples instanceof Float32Array)) return;
      this.enqueue(samples);
    };
  }

  enqueue(samples) {
    const frames = samples.length >> 1;
    const overflow = this.available + frames - this.capacity;
    if (overflow > 0) {
      this.read = (this.read + overflow) % this.capacity;
      this.available -= overflow;
      this.overruns += overflow;
    }
    for (let frame = 0; frame < frames; frame++) {
      this.left[this.write] = samples[frame * 2];
      this.right[this.write] = samples[frame * 2 + 1];
      this.write = (this.write + 1) % this.capacity;
    }
    this.available += frames;
    if (this.available > ${HIGH_WATER_FRAMES}) {
      const drop = this.available - ${PREBUFFER_FRAMES};
      this.read = (this.read + drop) % this.capacity;
      this.available -= drop;
      this.overruns += drop;
    }
  }

  pull(output, frames) {
    if (!this.started) {
      if (this.available < ${PREBUFFER_FRAMES}) {
        output[0].fill(0);
        if (output[1]) output[1].fill(0);
        return;
      }
      this.started = true;
      this.fade = 64;
    }
    const take = Math.min(this.available, frames);
    if (take < frames) {
      this.underruns += 1;
      this.dry += 1;
      if (this.available < ${LOW_WATER_FRAMES} && this.dry > 40) {
        this.started = false;
        this.dry = 0;
      }
    } else {
      this.dry = 0;
    }
    for (let frame = 0; frame < take; frame++) {
      let left = this.left[this.read];
      let right = this.right[this.read];
      if (this.fade > 0) {
        const gain = (64 - this.fade) / 64;
        left *= gain;
        right *= gain;
        this.fade--;
      }
      output[0][frame] = left;
      if (output[1]) output[1][frame] = right;
      this.read = (this.read + 1) % this.capacity;
    }
    this.available -= take;
    for (let frame = take; frame < frames; frame++) {
      output[0][frame] = 0;
      if (output[1]) output[1][frame] = 0;
    }
  }

  process(_inputs, outputs) {
    const output = outputs[0];
    if (!output || !output[0]) return true;
    this.pull(output, output[0].length);
    this.callbacks += 1;
    if (this.callbacks >= 750) {
      this.callbacks = 0;
      this.port.postMessage({
        type: "stats",
        underruns: this.underruns,
        overruns: this.overruns,
        buffered: this.available,
      });
    }
    return true;
  }
}
registerProcessor("shareguard-pcm", ShareGuardProcessor);
`;
