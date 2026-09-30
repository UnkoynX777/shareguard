import crypto from "node:crypto";

export function chromiumExtensionId(key) {
  const digest = crypto.createHash("sha256").update(Buffer.from(key, "base64")).digest();
  const hex = [...digest.subarray(0, 16)].map((byte) => byte.toString(16).padStart(2, "0")).join("");
  return [...hex].map((char) => String.fromCharCode(97 + Number.parseInt(char, 16))).join("");
}

const invoked = process.argv[1] && process.argv[1].endsWith("extension-id.mjs");
if (invoked && process.argv[2]) {
  process.stdout.write(chromiumExtensionId(process.argv[2]));
}
