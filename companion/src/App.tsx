import { useEffect, useMemo, useState } from "react";
import type { CompanionHost } from "./host";
import { type DeviceSettings, type MappingDraft, type ProfileDraft, type RuntimeStatus } from "./types";
import { DEFAULT_DEVICE_SETTINGS, EMPTY_NETWORK } from "./types";
import { ChordField } from "./ChordField";
import { SettingsPanel } from "./SettingsPanel";
import { TranscriptPanel } from "./TranscriptPanel";
import { formatRuntimeText, INITIAL_MAPPINGS, INPUT_LABELS, RUNTIME_STATE_LABELS, WIFI_CONNECTING_STUCK_HINT, WIFI_FIVE_G_ALERT } from "./ui";
import { canonicalChord, deviceSettingsError, mappingErrors, ssidLooksFiveG } from "./validation";
import "./app.css";

export function App({ host }: { host: CompanionHost }) {
  const [ports, setPorts] = useState<string[]>([]);
  const [draft, setDraft] = useState<ProfileDraft>({ version: 1, revision: null, serial: { port: "", baud: 115200 }, target: null, mappings: INITIAL_MAPPINGS });
  const [runtime, setRuntime] = useState<RuntimeStatus>({ state: "STOPPED", liveEnabled: false, lastEvent: "尚无事件。", gapMissed: null, network: EMPTY_NETWORK });
  const [device, setDevice] = useState<DeviceSettings>(DEFAULT_DEVICE_SETTINGS);
  const [settingsOpen, setSettingsOpen] = useState(false);
  const [notice, setNotice] = useState("尚未连接。请先选择设备串口。");
  const [hydrated, setHydrated] = useState(false);
  const [dirty, setDirty] = useState(false);
  const [busy, setBusy] = useState(false);
  const errors = useMemo(() => mappingErrors(draft.mappings), [draft.mappings]);
  const deviceError = useMemo(() => deviceSettingsError(device), [device]);
  const stopped = runtime.state === "STOPPED";
  const editable = hydrated && stopped && !busy;
  const canSave = editable && errors.size === 0 && Boolean(draft.serial.port.trim()) && draft.target !== null;
  const canStart = canSave && !dirty && draft.revision !== null;
  const canApply = hydrated && !busy && Boolean(draft.serial.port.trim()) && deviceError === null;
  const visiblePorts = useMemo(
    () => draft.serial.port && !ports.includes(draft.serial.port) ? [draft.serial.port, ...ports] : ports,
    [draft.serial.port, ports]
  );

  useEffect(() => {
    let cancelled = false;
    void host.loadProfile()
      .then((profile) => {
        if (!cancelled && profile) {
          setDraft(profile);
          setDirty(false);
          setNotice("已恢复保存的配置；实时权限保持关闭。");
        }
      })
      .catch(() => {
        if (!cancelled) setNotice("保存的配置不可用，正在使用空白草稿。");
      })
      .finally(() => {
        if (!cancelled) setHydrated(true);
      });
    void host.loadDeviceSettings()
      .then((settings) => { if (!cancelled) setDevice(settings); })
      .catch(() => { if (!cancelled) setNotice("已保存的联网设置不可用，正在使用空白草稿。"); });
    return () => { cancelled = true; };
  }, [host]);

  useEffect(() => {
    if (runtime.state === "STOPPED") return undefined;
    let cancelled = false;
    let timer: number | undefined;
    const poll = async (): Promise<void> => {
      let continuePolling = true;
      try {
        const status = await host.pollRuntimeEvent();
        if (!cancelled) setRuntime(status);
        continuePolling = status.state !== "STOPPED";
      } catch {
        continuePolling = false;
        if (!cancelled) setNotice("运行状态轮询失败；实时权限已清除。");
        try {
          const status = await host.stop();
          if (!cancelled) setRuntime(status);
        } catch {
          if (!cancelled) {
            setRuntime((current) => ({ ...current, lastEvent: "无法确认运行已停止。" }));
            setNotice("运行状态轮询失败且无法确认停止；再次启用实时模式前请重启应用。");
          }
        }
      } finally {
        if (!cancelled && continuePolling) timer = window.setTimeout(() => { void poll(); }, 100);
      }
    };
    void poll();
    return () => {
      cancelled = true;
      if (timer !== undefined) window.clearTimeout(timer);
    };
  }, [host, runtime.state]);

  useEffect(() => {
    if (!hydrated) return undefined;
    let cancelled = false;
    const poll = async (): Promise<void> => {
      try {
        const network = await host.pollNetworkStatus();
        if (!cancelled) setRuntime((current) => ({ ...current, network }));
      } catch {
        if (!cancelled) setNotice("网络状态轮询失败。");
      }
    };
    const timer = window.setInterval(() => { void poll(); }, 400);
    void poll();
    return () => {
      cancelled = true;
      window.clearInterval(timer);
    };
  }, [host, hydrated]);

  useEffect(() => {
    if (runtime.network.state !== "CONNECTING") return undefined;
    const timer = window.setTimeout(() => {
      setNotice(WIFI_CONNECTING_STUCK_HINT);
    }, 8000);
    return () => window.clearTimeout(timer);
  }, [runtime.network.state, runtime.network.ssid]);

  async function refresh(): Promise<void> {
    if (!editable) return;
    setBusy(true);
    try {
      setPorts(await host.listPorts());
      setNotice("已刷新串口列表，未打开任何串口。");
    } catch {
      setNotice("刷新串口列表失败，未打开任何串口。");
    } finally {
      setBusy(false);
    }
  }
  async function capture(): Promise<void> {
    if (!editable) return;
    setBusy(true);
    setNotice("将在 3 秒后捕获前台目标，请立即切换到目标应用。");
    try {
      const target = await host.captureTarget();
      setDraft((current) => ({ ...current, target }));
      setDirty(true);
      setNotice("已捕获前台目标。实时模式下，旋钮或 GPIO8 会先唤回该窗口，再发送快捷键。");
    } catch {
      setNotice("捕获目标失败，已保留之前的目标。");
    } finally {
      setBusy(false);
    }
  }
  async function save(): Promise<void> {
    if (!canSave) return;
    setBusy(true);
    try {
      setDraft(await host.saveProfile(draft));
      setDirty(false);
      setNotice("配置已保存并生成新修订版本。");
    } catch {
      setNotice("保存配置失败；解决过期修订版本前请重新加载。");
    } finally {
      setBusy(false);
    }
  }
  function update(index: number, patch: Partial<MappingDraft>): void {
    setDraft((current) => ({ ...current, mappings: current.mappings.map((mapping, currentIndex) => currentIndex === index ? { ...mapping, ...patch } : mapping) }));
    setDirty(true);
    if (patch.keys) setNotice("快捷键已更新；保存配置后才会用于派发。");
  }
  async function startDryRun(): Promise<void> {
    if (!canStart) return;
    setBusy(true);
    try {
      setRuntime(await host.startDryRun());
      setNotice("演练模式已启动，未创建输入派发器。");
    } catch {
      setNotice("无法启动演练模式，未启用实时输入。");
    } finally {
      setBusy(false);
    }
  }
  async function startLive(): Promise<void> {
    if (!canStart) return;
    setBusy(true);
    try {
      setRuntime(await host.enableLiveForRun());
      setNotice("仅为当前进程启用了实时权限：旋钮或 GPIO8 会先唤回已捕获窗口，再发送快捷键。");
    } catch {
      setNotice("无法启动实时模式；实时权限保持关闭。");
    } finally {
      setBusy(false);
    }
  }
  async function applyNetwork(): Promise<void> {
    if (!canApply) return;
    setBusy(true);
    try {
      const payload = { ...device, apiKey: device.apiKey.trim(), model: device.model.trim() };
      const network = await host.applyDeviceConfig(draft.serial.port, draft.serial.baud, payload);
      setRuntime((current) => ({ ...current, network }));
      if (ssidLooksFiveG(device.ssid) || network.reason === "BAND") {
        setNotice(WIFI_FIVE_G_ALERT);
      } else if (network.state === "CONNECTED") {
        setNotice("设备已联网。");
      } else if (network.state === "FAILED") {
        setNotice("联网失败。开发板只支持 2.4GHz，请改用 2.4G 名称后重试。");
      } else {
        setNotice("已下发 Wi-Fi，等待设备联网。开发板只支持 2.4GHz。");
      }
    } catch {
      setNotice("下发联网配置失败，未在界面显示密钥。");
    } finally {
      setBusy(false);
    }
  }
  async function stop(): Promise<void> {
    if (busy) return;
    setBusy(true);
    try {
      setRuntime(await host.stop());
      setNotice("运行已停止，实时权限已清除。");
    } catch {
      setRuntime((current) => ({ ...current, lastEvent: "无法确认运行已停止。" }));
      setNotice("无法确认运行已停止；再次启用实时模式前请重启应用。");
    } finally {
      setBusy(false);
    }
  }

  return <main className="window">
    <header><div><h1>VentureD Companion</h1><p>Board C 快捷键演示：捕获一次前台，旋钮或 GPIO8 将其唤回后再发送映射快捷键</p></div><span className={`state ${runtime.state.toLowerCase()}`}>{RUNTIME_STATE_LABELS[runtime.state]}</span></header>
    <section className="device" aria-label="设备串口"><label>设备串口<select disabled={!editable} value={draft.serial.port} onChange={(event) => { setDraft((current) => ({ ...current, serial: { ...current.serial, port: event.target.value } })); setDirty(true); }}><option value="">请选择串口</option>{visiblePorts.map((port) => <option key={port}>{port}</option>)}</select></label><button disabled={!editable} onClick={() => void refresh()}>刷新</button><span>波特率 {draft.serial.baud}</span></section>
    <SettingsPanel settings={device} network={runtime.network} open={settingsOpen} busy={busy} canApply={canApply} error={settingsOpen ? deviceError : null} onToggle={() => setSettingsOpen((current) => !current)} onChange={(patch) => setDevice((current) => ({ ...current, ...patch }))} onApply={() => void applyNetwork()} />
    <TranscriptPanel asrState={runtime.network.asrState} asrText={runtime.network.asrText} asrReason={runtime.network.asrReason} recState={runtime.network.recState} />
    <section className="target" aria-label="前台目标"><div><strong>前台目标</strong><p>{draft.target ? `${draft.target.processName} · ${draft.target.processPath}` : "尚未捕获。请先切到目标应用，再点 3 秒后捕获。"}</p></div><button disabled={!editable} onClick={() => void capture()}>3 秒后捕获</button></section>
    <section className="mappings" aria-label="固定输入映射">{draft.mappings.map((mapping, index) => <div className="mapping" key={mapping.input}><strong>{INPUT_LABELS[mapping.input]}</strong><input disabled={!editable} aria-label={`${INPUT_LABELS[mapping.input]} 名称`} value={mapping.displayName} onChange={(event) => update(index, { displayName: event.target.value })} /><ChordField label={`${INPUT_LABELS[mapping.input]} 快捷键`} keys={mapping.keys} disabled={!editable} onChange={(keys) => update(index, { keys })} /><span className="canonical">{canonicalChord(mapping.keys) ?? "点击后按下快捷键"}</span>{errors.get(mapping.input) && <small role="alert">{errors.get(mapping.input)}</small>}</div>)}</section>
    <section className="controls"><button disabled={!canSave} onClick={() => void save()}>保存配置</button><button disabled={!canStart} onClick={() => void startDryRun()}>启动演练模式</button><button className="live" disabled={!canStart} onClick={() => void startLive()}>为本次运行启用实时模式</button><button disabled={stopped || busy} onClick={() => void stop()}>停止运行</button></section>
    <footer><strong>最后事件：</strong> {formatRuntimeText(runtime.lastEvent)}<br /><span>{notice}</span></footer>
  </main>;
}
