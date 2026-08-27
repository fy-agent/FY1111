import { invoke } from "@tauri-apps/api/core";
import type { DeviceSettings, NetworkStatus, ProfileDraft, RuntimeStatus, TargetDraft } from "./types";
import { DEFAULT_DEVICE_SETTINGS, EMPTY_NETWORK } from "./types";
import { ssidLooksFiveG } from "./validation";

export interface CompanionHost {
  listPorts(): Promise<string[]>;
  captureTarget(): Promise<TargetDraft>;
  loadProfile(): Promise<ProfileDraft | null>;
  saveProfile(draft: ProfileDraft): Promise<ProfileDraft>;
  startDryRun(): Promise<RuntimeStatus>;
  enableLiveForRun(): Promise<RuntimeStatus>;
  pollRuntimeEvent(): Promise<RuntimeStatus>;
  stop(): Promise<RuntimeStatus>;
  loadDeviceSettings(): Promise<DeviceSettings>;
  saveDeviceSettings(draft: DeviceSettings): Promise<DeviceSettings>;
  applyDeviceConfig(port: string, baud: number, settings: DeviceSettings): Promise<NetworkStatus>;
  pollNetworkStatus(): Promise<NetworkStatus>;
}

export const tauriHost: CompanionHost = {
  listPorts: () => invoke<string[]>("list_ports"),
  captureTarget: () => invoke<TargetDraft>("capture_target_after_delay"),
  loadProfile: () => invoke<ProfileDraft | null>("load_profile"),
  saveProfile: (draft) => invoke<ProfileDraft>("save_profile", { draft }),
  startDryRun: () => invoke<RuntimeStatus>("start_dry_run"),
  enableLiveForRun: () => invoke<RuntimeStatus>("enable_live_for_run"),
  pollRuntimeEvent: () => invoke<RuntimeStatus>("poll_runtime_event"),
  stop: () => invoke<RuntimeStatus>("stop_runtime"),
  loadDeviceSettings: () => invoke<DeviceSettings>("load_device_settings"),
  saveDeviceSettings: (draft) => invoke<DeviceSettings>("save_device_settings", { draft }),
  applyDeviceConfig: (port, baud, settings) =>
    invoke<NetworkStatus>("apply_device_config", { request: { port, baud, settings } }),
  pollNetworkStatus: () => invoke<NetworkStatus>("poll_network_status")
};

export function createFixtureHost(): CompanionHost {
  const stopped = (): RuntimeStatus => ({
    state: "STOPPED",
    liveEnabled: false,
    lastEvent: "尚无事件。",
    gapMissed: null,
    network: { ...EMPTY_NETWORK }
  });
  let status = stopped();
  let saved: ProfileDraft | null = null;
  let device = { ...DEFAULT_DEVICE_SETTINGS };
  let network = { ...EMPTY_NETWORK };
  return {
    listPorts: async () => ["浏览器夹具串口"],
    captureTarget: async () => ({ processName: "Fixture.exe", processPath: "C:\\Fixture\\Fixture.exe" }),
    loadProfile: async () => saved,
    saveProfile: async (draft) => { saved = { ...draft, revision: "fixture-revision" }; return saved; },
    startDryRun: async () => {
      status = { ...stopped(), state: "DRY_RUN", lastEvent: "ENCODER_CW → CTRL+TAB · 演练模式", network };
      return status;
    },
    enableLiveForRun: async () => {
      status = { ...stopped(), state: "LIVE", liveEnabled: true, lastEvent: "本浏览器夹具会话已启用实时权限。", network };
      return status;
    },
    pollRuntimeEvent: async () => ({ ...status, network }),
    stop: async () => {
      status = { ...stopped(), network };
      return status;
    },
    loadDeviceSettings: async () => device,
    saveDeviceSettings: async (draft) => {
      device = { ...draft, version: 1 };
      return device;
    },
    applyDeviceConfig: async (port, _baud, draft) => {
      if (!port.trim()) throw new Error("a serial port is required");
      device = { ...draft, version: 1 };
      network = ssidLooksFiveG(draft.ssid)
        ? { ...EMPTY_NETWORK, state: "FAILED", ssid: draft.ssid, reason: "BAND" }
        : {
          ...EMPTY_NETWORK,
          state: "CONNECTED",
          ssid: draft.ssid,
          ip: "10.0.0.8",
          rssi: -40,
          pingHost: "8.8.8.8",
          pingOk: true,
          pingMs: 18,
          pingLost: 0,
          pingSent: 3,
          lastLog: "sta got ip=10.0.0.8",
          beats: 1
        };
      status = { ...status, network };
      return network;
    },
    pollNetworkStatus: async () => network
  };
}
