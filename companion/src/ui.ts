import type { MappingDraft, RuntimeStatus } from "./types";

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
