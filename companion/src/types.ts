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

export interface RuntimeStatus {
  state: "STOPPED" | "DRY_RUN" | "LIVE";
  liveEnabled: boolean;
  lastEvent: string;
  gapMissed: number | null;
}
