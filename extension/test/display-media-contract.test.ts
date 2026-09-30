import assert from "node:assert/strict";
import test from "node:test";
import { invokeDisplayMedia, shouldReplaceAudio } from "../src/page/share-decision.ts";

test("audio is replaced only when protection is on and the stream has audio", () => {
  assert.equal(shouldReplaceAudio(true, 1), true);
  assert.equal(shouldReplaceAudio(true, 0), false);
  assert.equal(shouldReplaceAudio(false, 2), false);
});

test("getDisplayMedia is called once with the original receiver and constraints", async () => {
  const calls: Array<{ receiver: object; constraints: object | undefined }> = [];
  const device = { label: "devices" };
  const original = function (this: object, constraints?: { audio?: boolean; video?: boolean }) {
    calls.push({ receiver: this, constraints });
    return Promise.resolve({ audio: constraints?.audio === true });
  };
  const stream = await invokeDisplayMedia(
    original as (this: MediaDevices, constraints?: DisplayMediaStreamOptions) => Promise<MediaStream>,
    device as unknown as MediaDevices,
    { video: true, audio: true },
  );
  assert.equal(calls.length, 1);
  assert.equal(calls[0]?.receiver, device);
  assert.deepEqual(calls[0]?.constraints, { video: true, audio: true });
  assert.equal((stream as unknown as { audio: boolean }).audio, true);
});

test("picker cancellation is propagated", async () => {
  const error = new Error("denied");
  error.name = "NotAllowedError";
  const original = () => Promise.reject(error);
  await assert.rejects(
    invokeDisplayMedia(
      original as (this: MediaDevices, constraints?: DisplayMediaStreamOptions) => Promise<MediaStream>,
      {} as MediaDevices,
      undefined,
    ),
    (caught: unknown) => caught instanceof Error && caught.name === "NotAllowedError",
  );
});
