import type { DeviceSettings, NetworkStatus } from "./types";
// API key / model fields are hidden for this Wi-Fi-only debug pass.
import { networkChipLabel, networkReasonLabel, WIFI_BAND_HINT, WIFI_FIVE_G_ALERT } from "./ui";
import { ssidLooksFiveG } from "./validation";

export function SettingsPanel({
  settings,
  network,
  open,
  busy,
  canApply,
  error,
  onToggle,
  onChange,
  onApply
}: {
  settings: DeviceSettings;
  network: NetworkStatus;
  open: boolean;
  busy: boolean;
  canApply: boolean;
  error: string | null;
  onToggle: () => void;
  onChange: (patch: Partial<DeviceSettings>) => void;
  onApply: () => void;
}) {
  const fiveG = ssidLooksFiveG(settings.ssid);
  const chip = networkChipLabel(network.state, network.ip, network.reason, fiveG);
  const reason = networkReasonLabel(network.reason);
  return <section className="settings" aria-label="设备与云端设置">
    <button type="button" className="settings-toggle" aria-expanded={open} onClick={onToggle}>
      <strong>设置</strong>
      <span className={`net ${network.state.toLowerCase()}`}>{chip}</span>
    </button>
    {open && <div className="settings-panel">
      <p className="hint">{WIFI_BAND_HINT}</p>
      <label>Wi-Fi 名称<input disabled={busy} value={settings.ssid} autoComplete="off" onChange={(event) => onChange({ ssid: event.target.value })} /></label>
      <label>Wi-Fi 密码<input disabled={busy} type="password" value={settings.password} autoComplete="new-password" onChange={(event) => onChange({ password: event.target.value })} /></label>
      <p>本次只调试 Wi-Fi。SiliconFlow API Key 与转写模型已屏蔽。</p>
      <button type="button" disabled={!canApply} onClick={onApply}>保存并下发联网</button>
      {fiveG && <small role="alert" className="warn">{WIFI_FIVE_G_ALERT}</small>}
      {error && <small role="alert">{error}</small>}
      {reason && <p className="warn">{reason}</p>}
      {network.ssid && <p>当前 SSID {network.ssid}{network.rssi != null ? ` · ${network.rssi} dBm` : ""}{network.beats != null ? ` · 心跳 ${network.beats}` : ""}</p>}
      {network.pingHost && <p>连通探测 {network.pingHost}{network.pingOk ? ` 成功 ${network.pingMs ?? "-"} ms` : " 失败"}{network.pingSent != null ? ` · 丢 ${network.pingLost ?? 0}/${network.pingSent}` : ""}</p>}
      {network.lastLog && <p>串口日志 {network.lastLog}</p>}
    </div>}
  </section>;
}
