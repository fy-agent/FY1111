import type { MappingDraft, NetworkState, RuntimeStatus } from "./types";

export const INPUT_LABELS = {
  ENCODER_CW: "顺时针旋转",
  ENCODER_CCW: "逆时针旋转",
  // ENCODER_PRESS remains the wire contract even though GPIO8 is an external button.
  ENCODER_PRESS: "GPIO8 外接确认/动作按钮"
} as const;

export const INITIAL_MAPPINGS: MappingDraft[] = [
  { input: "ENCODER_CW", displayName: "上一项", keys: ["CTRL", "TAB"] },
  { input: "ENCODER_CCW", displayName: "下一项", keys: ["CTRL", "SHIFT", "TAB"] },
  { input: "ENCODER_PRESS", displayName: "确认动作", keys: ["ENTER"] }
];

export const RUNTIME_STATE_LABELS: Record<RuntimeStatus["state"], string> = {
  STOPPED: "已停止",
  DRY_RUN: "演练模式",
  LIVE: "实时模式"
};

export const NETWORK_STATE_LABELS: Record<NetworkState, string> = {
  UNKNOWN: "未连接",
  DISCONNECTED: "未连接",
  CONNECTING: "连接中",
  CONNECTED: "已连接",
  FAILED: "失败"
};

export const WIFI_BAND_HINT = "开发板只支持 2.4GHz Wi-Fi。5G 热点搜不到，也不会变成已连接。";
export const WIFI_FIVE_G_ALERT = "当前名称像 5G 热点，开发板连不上。请改用 2.4GHz 名称。";
export const WIFI_CONNECTING_STUCK_HINT = "仍在连接中：若这是 5G 热点，开发板搜不到，请改用 2.4GHz 名称。";
export const MIC_REC_HINT = "按住 GPIO9 录音，松开结束；有外部内存时最长约数分钟，存满会自动停。已联网且填写 Key 后，板子会把 16 kHz WAV 传到硅基，再把转写回传到这里。";
export const CLOUD_OPTIONAL_HINT = "API Key 可留空只测 Wi-Fi。转写默认 XingChenAGI/XingChenASR-V3.2-Ultra，也可自行填写其他模型。";

export function recStateLabel(state: string | null): string | null {
  switch (state) {
    case "START":
    case "ACTIVE": return "录音中";
    case "DONE": return "录音完成";
    case "FAIL": return "录音失败";
    default: return null;
  }
}

export function networkReasonLabel(reason: string | null): string | null {
  switch (reason) {
    case "BAND": return "开发板仅支持 2.4GHz，当前名称像 5G 热点";
    case "NO_AP": return "找不到这个热点；请确认 2.4GHz 名称";
    case "AUTH": return "认证失败，请检查密码";
    case "TIMEOUT": return "连接超时";
    case "UNKNOWN": return "联网失败";
    default: return null;
  }
}

export function networkChipLabel(state: NetworkState, ip: string, reason: string | null, ssidLooksFiveG: boolean): string {
  if (state === "CONNECTED" && ip) return `${NETWORK_STATE_LABELS[state]} ${ip}`;
  if (reason === "BAND" || (ssidLooksFiveG && state === "FAILED")) return "失败 · 仅2.4G";
  if (ssidLooksFiveG && state !== "CONNECTED") return `${NETWORK_STATE_LABELS[state]} · 疑似5G`;
  return NETWORK_STATE_LABELS[state];
}

const RUNTIME_PHRASES: ReadonlyArray<readonly [string, string]> = [
  ["Dry-run started. No dispatcher constructed.", "已启动演练模式；未创建输入派发器。"],
  ["Live enabled for this process only.", "仅为当前进程启用实时权限。"],
  ["No event yet.", "尚无事件。"],
  ["runtime is stopped", "运行已停止"],
  ["stop the active runtime before changing configuration", "请先停止当前运行，再修改配置"],
  ["a valid saved profile is required", "需要有效且已保存的配置"],
  ["input is unmapped", "输入未映射"],
  ["profile mapping is invalid", "映射配置无效"],
  ["serial input stopped", "串口输入已停止"],
  ["foreground target did not match", "前台目标不匹配"],
  ["foreground restore target is missing", "前台恢复目标不存在"],
  ["foreground restore was rejected", "前台恢复被拒绝"],
  ["keyboard state is not clear", "键盘按键状态不干净"],
  ["input dispatch rejected", "输入派发被拒绝"],
  [" · dry-run", " · 演练模式"],
  [" · live", " · 实时模式"],
  [" · dispatched", " · 已派发"],
  [" · rejected", " · 已拒绝"]
];

export function formatRuntimeText(value: string): string {
  return RUNTIME_PHRASES.reduce(
    (formatted, [english, chinese]) => formatted.replaceAll(english, chinese),
    value
  );
}
