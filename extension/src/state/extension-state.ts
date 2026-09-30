import type { ExtensionRules, ShareStatus } from "../shared/types";

export function createStatus(rules: ExtensionRules, nativeConnected: boolean, sharing: boolean): ShareStatus {
  return {
    nativeConnected,
    protectionEnabled: rules.protectionEnabled,
    sharing,
    capturing: false,
    captureStrategy: "SystemLoopback",
    blockedCount: rules.blockedApplications.length,
    lastError: "",
  };
}
