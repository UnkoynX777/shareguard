export function shouldReplaceAudio(protectionEnabled: boolean, audioTrackCount: number): boolean {
  return protectionEnabled && audioTrackCount > 0;
}

export function invokeDisplayMedia(
  original: (this: MediaDevices, constraints?: DisplayMediaStreamOptions) => Promise<MediaStream>,
  device: MediaDevices,
  constraints: DisplayMediaStreamOptions | undefined,
): Promise<MediaStream> {
  return original.call(device, constraints);
}
