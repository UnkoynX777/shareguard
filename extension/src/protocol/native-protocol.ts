import type { ExtensionRules, NativeMessage, ProcessView } from "../shared/types";
import type { BrowserFamily } from "../platform/browser/BrowserAdapter";
import { PROTOCOL_VERSION } from "../shared/constants";

export function helloMessage(
  browser: BrowserFamily,
  extensionVersion: string,
): { type: "HELLO"; protocolVersion: number; client: { browser: BrowserFamily; extensionVersion: string } } {
  return { type: "HELLO", protocolVersion: PROTOCOL_VERSION, client: { browser, extensionVersion } };
}

export function policyMessage(
  rules: ExtensionRules,
  revision: number,
): {
  type: "SET_AUDIO_POLICY";
  protectionEnabled: boolean;
  blocked: string[];
  revision: number;
} {
  return {
    type: "SET_AUDIO_POLICY",
    protectionEnabled: rules.protectionEnabled,
    blocked: rules.blockedApplications,
    revision,
  };
}

export function snapshotRequest(includeBackground: boolean): {
  type: "GET_PROCESS_SNAPSHOT";
  includeBackground: boolean;
} {
  return { type: "GET_PROCESS_SNAPSHOT", includeBackground };
}

export function isNativeMessage(value: unknown): value is NativeMessage {
  if (!value || typeof value !== "object") return false;
  return typeof (value as { type?: unknown }).type === "string";
}

export function readProcesses(value: unknown): ProcessView[] {
  if (!Array.isArray(value)) return [];
  return value.filter((item): item is ProcessView => {
    if (!item || typeof item !== "object") return false;
    const record = item as Record<string, unknown>;
    return typeof record.id === "string" && typeof record.name === "string" && typeof record.executable === "string";
  }).map((item) => ({
    id: item.id,
    rootPid: typeof item.rootPid === "number" ? item.rootPid : 0,
    name: item.name,
    executable: item.executable,
    audioActive: item.audioActive === true,
    processCount: typeof item.processCount === "number" ? item.processCount : 1,
  }));
}
