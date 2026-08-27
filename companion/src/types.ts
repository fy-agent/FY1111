export const INPUT_IDS = ["ENCODER_CW", "ENCODER_CCW", "ENCODER_PRESS"] as const;
export type InputId = (typeof INPUT_IDS)[number];
export type Modifier = "CTRL" | "ALT" | "SHIFT";
export type PrimaryKey = string;

export interface MappingDraft {
  input: InputId;
  displayName: string;
  keys: string[];
}

export interface TargetDraft {
  processName: string;
  processPath: string;
}

export interface ProfileDraft {
  version: 1;
  revision: string | null;
  serial: SerialDraft;
  target: TargetDraft | null;
  mappings: MappingDraft[];
}
export interface SerialDraft {
  port: string;
  baud: number;
}

export type NetworkState = "UNKNOWN" | "DISCONNECTED" | "CONNECTING" | "CONNECTED" | "FAILED";

export interface NetworkStatus {
  state: NetworkState;
  ssid: string;
  ip: string;
  rssi: number | null;
  reason: string | null;
  pingHost: string | null;
  pingOk: boolean | null;
  pingMs: number | null;
  pingLost: number | null;
  pingSent: number | null;
  lastLog: string | null;
  beats: number | null;
}

export interface DeviceSettings {
  version: 1;
  ssid: string;
  password: string;
  apiKey: string;
  model: string;
}

export interface RuntimeStatus {
  state: "STOPPED" | "DRY_RUN" | "LIVE";
  liveEnabled: boolean;
  lastEvent: string;
  gapMissed: number | null;
  network: NetworkStatus;
}

export const EMPTY_NETWORK: NetworkStatus = {
  state: "UNKNOWN",
  ssid: "",
  ip: "",
  rssi: null,
  reason: null,
  pingHost: null,
  pingOk: null,
  pingMs: null,
  pingLost: null,
  pingSent: null,
  lastLog: null,
  beats: null
};

export const DEFAULT_DEVICE_SETTINGS: DeviceSettings = {
  version: 1,
  ssid: "",
  password: "",
  apiKey: "",
  model: "FunAudioLLM/SenseVoiceSmall"
};

export const CLOUD_MODELS = ["FunAudioLLM/SenseVoiceSmall", "TeleAI/TeleSpeechASR"] as const;
