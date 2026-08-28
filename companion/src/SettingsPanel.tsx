import { useState } from "react";
import type { DeviceSettings, NetworkStatus } from "./types";
import { CLOUD_MODELS } from "./types";
import { CLOUD_OPTIONAL_HINT, MIC_REC_HINT, SENSOR_HINT, networkChipLabel, networkReasonLabel, recReasonLabel, recStateLabel, WIFI_BAND_HINT, WIFI_FIVE_G_ALERT } from "./ui";
import { ssidLooksFiveG } from "./validation";

function PasswordField({
  label,
  value,
  disabled,
  onChange
}: {
  label: string;
  value: string;
  disabled: boolean;
  onChange: (value: string) => void;
}) {
  const [visible, setVisible] = useState(false);
  const fieldId = "device-wifi-password";
  const toggleLabel = visible ? `隐藏${label}` : `显示${label}`;
  return <div className="settings-field">
    <label htmlFor={fieldId}>{label}</label>
    <span className="secret-field">
      <input
        id={fieldId}
        disabled={disabled}
        type={visible ? "text" : "password"}
        value={value}
        autoComplete="new-password"
        onChange={(event) => onChange(event.target.value)}
      />
      <button
        type="button"
        className="secret-toggle"
        aria-label={toggleLabel}
        aria-pressed={visible}
        title={toggleLabel}
        onClick={() => setVisible((current) => !current)}
      >
        <svg viewBox="0 0 24 24" width="18" height="18" aria-hidden="true">
          <path
            fill="none"
            stroke="currentColor"
            strokeWidth="2"
            strokeLinecap="round"
            d="M2 12s3.5-7 10-7 10 7 10 7-3.5 7-10 7-10-7-10-7Z"
          />
          <circle cx="12" cy="12" r="3" fill="none" stroke="currentColor" strokeWidth="2" />
          {visible ? <path fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" d="M4 4l16 16" /> : null}
        </svg>
        <span>{visible ? "隐藏" : "眼睛"}</span>
      </button>
    </span>
  </div>;
}

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
  const rec = recStateLabel(network.recState);
  return <section className="settings" aria-label="设备与云端设置">
    <button type="button" className="settings-toggle" aria-expanded={open} onClick={onToggle}>
      <strong>设置</strong>
      <span className={`net ${network.state.toLowerCase()}`}>{chip}</span>
    </button>
    {open && <div className="settings-panel">
      <p className="hint">{WIFI_BAND_HINT}</p>
      <label>Wi-Fi 名称<input disabled={busy} value={settings.ssid} autoComplete="off" onChange={(event) => onChange({ ssid: event.target.value })} /></label>
      <PasswordField label="Wi-Fi 密码" value={settings.password} disabled={busy} onChange={(password) => onChange({ password })} />
      <label>SiliconFlow API Key<input disabled={busy} type="password" value={settings.apiKey} autoComplete="off" onChange={(event) => onChange({ apiKey: event.target.value })} /></label>
      <label>转写模型
        <input
          disabled={busy}
          list="cloud-models"
          value={settings.model}
          autoComplete="off"
          placeholder="仅 Wi-Fi 可留空，也可填写其他模型"
          onChange={(event) => onChange({ model: event.target.value })}
        />
        <datalist id="cloud-models">
          {CLOUD_MODELS.map((model) => <option key={model} value={model} />)}
        </datalist>
      </label>
      <p className="hint">{CLOUD_OPTIONAL_HINT}</p>
      <p className="hint">{MIC_REC_HINT}</p>
      <p className="hint">{SENSOR_HINT}</p>
      <button type="button" disabled={!canApply} onClick={onApply}>保存并下发联网</button>
      {fiveG && <small role="alert" className="warn">{WIFI_FIVE_G_ALERT}</small>}
      {error && <small role="alert">{error}</small>}
      {reason && <p className="warn">{reason}</p>}
      {network.ssid && <p>当前 SSID {network.ssid}{network.rssi != null ? ` · ${network.rssi} dBm` : ""}{network.beats != null ? ` · 心跳 ${network.beats}` : ""}</p>}
      {network.pingHost && <p>连通探测 {network.pingHost}{network.pingOk ? ` 成功 ${network.pingMs ?? "-"} ms` : " 失败"}{network.pingSent != null ? ` · 丢 ${network.pingLost ?? 0}/${network.pingSent}` : ""}</p>}
      {rec && <p>{rec}{network.recMs != null ? ` · ${network.recMs} ms` : ""}{network.recRms != null ? ` · RMS ${network.recRms}` : ""}{network.recPeak != null ? ` · 峰 ${network.recPeak}` : ""}{network.recSilence ? " · 静音" : ""}{network.recReason ? ` · ${recReasonLabel(network.recReason) ?? network.recReason}` : ""}</p>}
      {network.pir != null && <p>座位 {network.pir ? "有人" : "无人"}</p>}
      {network.tofMm != null && <p>激光测距 {network.tofMm} mm</p>}
      {network.sensorState === "TOF" && <p className="warn">激光测距未就绪</p>}
      {network.sensorState === "I2C" && <p className="warn">I2C 总线未就绪</p>}
      {network.lastLog && <p>串口日志 {network.lastLog}</p>}
    </div>}
  </section>;
}
