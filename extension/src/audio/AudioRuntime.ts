export interface AudioRuntime {
  isActive(): boolean;
  attach(context: AudioContext): Promise<void>;
  createTrack(): Promise<MediaStreamTrack>;
  pushBase64(payload: string): void;
  release(): void;
  close(): Promise<void>;
}
