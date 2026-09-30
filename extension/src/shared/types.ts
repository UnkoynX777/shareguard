export interface ProcessView {
  id: string;
  rootPid: number;
  name: string;
  executable: string;
  audioActive: boolean;
  processCount: number;
}

export interface ProcessDiffMessage {
  type: "PROCESS_DIFF";
  added: ProcessView[];
  removed: string[];
  updated: ProcessView[];
}

export interface ExtensionRules {
  protectionEnabled: boolean;
  blockedApplications: string[];
  showAllProcesses: boolean;
}

export interface ShareStatus {
  nativeConnected: boolean;
  protectionEnabled: boolean;
  sharing: boolean;
  capturing: boolean;
  captureStrategy: string;
  blockedCount: number;
  lastError: string;
}

export interface PopupSnapshot {
  rules: ExtensionRules;
  processes: ProcessView[];
  status: ShareStatus;
}

export interface NativeHelloAck {
  type: "HELLO_ACK";
  protocolVersion: number;
  nativeVersion: string;
  processLoopbackSupported: boolean;
}

export interface NativeError {
  type: "ERROR";
  code: string;
  message: string;
}

export interface NativeStatus {
  type: "STATUS";
  capturing: boolean;
  sharing: boolean;
  protectionEnabled: boolean;
  captureStrategy: string;
  blockedCount: number;
  activeSourceCount: number;
  processLoopbackSupported: boolean;
  lastError: string;
}

export interface NativeAudioFrame {
  type: "AUDIO_FRAME";
  sequence: number;
  sampleRate: number;
  channels: number;
  format: string;
  data: string;
}

export interface NativeSnapshot {
  type: "PROCESS_SNAPSHOT";
  processes: ProcessView[];
}

export interface NativePolicyApplied {
  type: "AUDIO_POLICY_APPLIED";
  captureStrategy: string;
  blockedCount: number;
}

export interface NativeCaptureState {
  type: "CAPTURE_STARTED" | "CAPTURE_STOPPED";
  captureStrategy: string;
}

export type NativeMessage =
  | NativeHelloAck
  | NativeError
  | NativeStatus
  | NativeAudioFrame
  | NativeSnapshot
  | ProcessDiffMessage
  | NativePolicyApplied
  | NativeCaptureState
  | { type: "PONG" };

export function isProcessView(value: unknown): value is ProcessView {
  if (!value || typeof value !== "object") return false;
  const record = value as Record<string, unknown>;
  return (
    typeof record.id === "string" &&
    typeof record.rootPid === "number" &&
    typeof record.name === "string" &&
    typeof record.executable === "string" &&
    typeof record.audioActive === "boolean"
  );
}
