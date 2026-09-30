import type { ProcessDiffMessage, ProcessView } from "../shared/types";
import { readProcesses } from "../protocol/native-protocol";

export class ProcessStore {
  private readonly processes = new Map<string, ProcessView>();

  replace(value: unknown): void {
    this.processes.clear();
    for (const process of readProcesses(value)) {
      this.processes.set(process.id, process);
    }
  }

  apply(diff: ProcessDiffMessage): void {
    for (const id of diff.removed) this.processes.delete(id);
    for (const process of readProcesses(diff.added)) this.processes.set(process.id, process);
    for (const process of readProcesses(diff.updated)) this.processes.set(process.id, process);
  }

  list(): ProcessView[] {
    return [...this.processes.values()];
  }
}
