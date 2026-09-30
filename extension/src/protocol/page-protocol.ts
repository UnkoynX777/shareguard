export const PAGE_REQUEST = {
  getEnabled: "get-enabled",
  beginShare: "begin-share",
  endShare: "end-share",
  protectFailed: "protect-failed",
} as const;

const PAGE_REQUESTS = new Set<string>(Object.values(PAGE_REQUEST));

export function isPageRequest(type: string): boolean {
  return PAGE_REQUESTS.has(type);
}

export const PAGE_EVENT = {
  enabled: "enabled",
  shareReady: "share-ready",
  shareFailed: "share-failed",
  audio: "audio",
  captureError: "capture-error",
} as const;
