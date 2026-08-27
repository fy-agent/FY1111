import { invoke } from "@tauri-apps/api/core";
import type { ProfileDraft, RuntimeStatus, TargetDraft } from "./types";

export interface CompanionHost {
  listPorts(): Promise<string[]>;
  captureTarget(): Promise<TargetDraft>;
  loadProfile(): Promise<ProfileDraft | null>;
  saveProfile(draft: ProfileDraft): Promise<ProfileDraft>;
  startDryRun(): Promise<RuntimeStatus>;
  enableLiveForRun(): Promise<RuntimeStatus>;
  pollRuntimeEvent(): Promise<RuntimeStatus>;
  stop(): Promise<RuntimeStatus>;
}

export const tauriHost: CompanionHost = {
  listPorts: () => invoke<string[]>("list_ports"),
  captureTarget: () => invoke<TargetDraft>("capture_target_after_delay"),
  loadProfile: () => invoke<ProfileDraft | null>("load_profile"),
  saveProfile: (draft) => invoke<ProfileDraft>("save_profile", { draft }),
  startDryRun: () => invoke<RuntimeStatus>("start_dry_run"),
  enableLiveForRun: () => invoke<RuntimeStatus>("enable_live_for_run"),
  pollRuntimeEvent: () => invoke<RuntimeStatus>("poll_runtime_event"),
  stop: () => invoke<RuntimeStatus>("stop_runtime")
};

export function createFixtureHost(): CompanionHost {
  const stopped = (): RuntimeStatus => ({ state: "STOPPED", liveEnabled: false, lastEvent: "尚无事件。", gapMissed: null });
  let status = stopped();
  let saved: ProfileDraft | null = null;
  return {
    listPorts: async () => ["浏览器夹具串口"],
    captureTarget: async () => ({ processName: "Fixture.exe", processPath: "C:\\Fixture\\Fixture.exe" }),
    loadProfile: async () => saved,
    saveProfile: async (draft) => { saved = { ...draft, revision: "fixture-revision" }; return saved; },
    startDryRun: async () => {
      status = { ...stopped(), state: "DRY_RUN", lastEvent: "ENCODER_CW → CTRL+TAB · 演练模式" };
      return status;
    },
    enableLiveForRun: async () => {
      status = { ...stopped(), state: "LIVE", liveEnabled: true, lastEvent: "本浏览器夹具会话已启用实时权限。" };
      return status;
    },
    pollRuntimeEvent: async () => status,
    stop: async () => {
      status = stopped();
      return status;
    }
  };
}
