import type { ExtensionRules } from "../shared/types";
import { browserAdapter } from "../platform/browser/adapters";

const STORAGE_KEY = "shareguardRules";

function isRules(value: unknown): value is ExtensionRules {
  if (!value || typeof value !== "object") return false;
  const record = value as Record<string, unknown>;
  return (
    typeof record.protectionEnabled === "boolean" &&
    Array.isArray(record.blockedApplications) &&
    record.blockedApplications.every((item) => typeof item === "string") &&
    typeof record.showAllProcesses === "boolean"
  );
}

export function defaultRules(): ExtensionRules {
  return { protectionEnabled: true, blockedApplications: [], showAllProcesses: false };
}

export async function loadRules(): Promise<ExtensionRules> {
  const value = await browserAdapter().getStorage(STORAGE_KEY);
  if (!isRules(value)) {
    const rules = defaultRules();
    await browserAdapter().setStorage(STORAGE_KEY, rules);
    return rules;
  }
  return {
    protectionEnabled: value.protectionEnabled,
    blockedApplications: value.blockedApplications.map((name) => name.toLowerCase()),
    showAllProcesses: value.showAllProcesses,
  };
}

export async function saveRules(rules: ExtensionRules): Promise<void> {
  await browserAdapter().setStorage(STORAGE_KEY, rules);
}
