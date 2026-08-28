import { describe, expect, it } from "vitest";
import { canonicalChord, chordIdentity, deviceSettingsError, displayNameError, mappingErrors, modifiersFromKeyboardEvent, primaryFromKeyboardEvent, ssidLooksFiveG } from "../src/validation";
import { createFixtureHost } from "../src/host";
import { asrHeadline, asrReasonLabel, formatRuntimeText, INITIAL_MAPPINGS, INPUT_LABELS, MIC_REC_HINT, NETWORK_STATE_LABELS, rememberTranscript, RUNTIME_STATE_LABELS, SENSOR_HINT, WIFI_BAND_HINT, networkChipLabel, networkReasonLabel, recReasonLabel, recStateLabel } from "../src/ui";
import packageJson from "../package.json";
import tauriConfig from "../src-tauri/tauri.conf.json";

describe("mapping validation", () => {
  it("canonicalizes modifier order and rejects invalid chord shapes", () => {
    expect(canonicalChord(["shift", "ctrl", "tab"])).toBe("CTRL+SHIFT+TAB");
    expect(canonicalChord(["ctrl", "tab", "1"])).toBe("CTRL+TAB+1");
    expect(canonicalChord(["ctrl", "shift", "["])).toBe("CTRL+SHIFT+[");
    expect(canonicalChord(["CTRL", "ALT"])).toBeNull();
    expect(canonicalChord(["CTRL", "CTRL", "A"])).toBeNull();
    expect(canonicalChord(["CTRL", "", "A"])).toBeNull();
  });
  it("accepts bounded Unicode names and rejects controls", () => {
    expect(displayNameError("上一项")).toBeNull();
    expect(displayNameError("bad\nname")).not.toBeNull();
    expect(displayNameError("😀".repeat(40))).toBeNull();
    expect(displayNameError("😀".repeat(41))).not.toBeNull();
  });
  it("reports duplicate canonical chords for both fixed rows", () => {
    const errors = mappingErrors([
      { input: "ENCODER_CW", displayName: "A", keys: ["CTRL", "TAB"] },
      { input: "ENCODER_CCW", displayName: "B", keys: ["TAB", "CTRL"] },
      { input: "ENCODER_PRESS", displayName: "C", keys: ["ENTER"] }
    ]);
    expect(errors.get("ENCODER_CW")).toContain("与 ENCODER_CCW 重复");
    expect(errors.get("ENCODER_CCW")).toContain("与 ENCODER_CW 重复");
  });
  it("projects the independent GPIO8 action button and Chinese runtime controls without changing protocol IDs", () => {
    expect(INPUT_LABELS.ENCODER_PRESS).toBe("GPIO8 外接确认/动作按钮");
    expect(INPUT_LABELS.BUTTON_A).toBe("GPIO10 下拉按键");
    expect(INPUT_LABELS.BUTTON_B).toBe("GPIO11 下拉按键");
    expect(INITIAL_MAPPINGS.find((mapping) => mapping.input === "BUTTON_B")?.displayName).toBe("确认动作");
    expect(INITIAL_MAPPINGS.find((mapping) => mapping.input === "BUTTON_B")?.keys).toEqual(["ENTER"]);
    expect(INITIAL_MAPPINGS.find((mapping) => mapping.input === "BUTTON_A")?.keys).toEqual(["CTRL", "N"]);
    expect(INITIAL_MAPPINGS.find((mapping) => mapping.input === "ENCODER_PRESS")?.keys).toEqual(["CTRL", "SHIFT", "N"]);
    expect(INITIAL_MAPPINGS).toHaveLength(5);
    expect(RUNTIME_STATE_LABELS.STOPPED).toBe("已停止");
    expect(RUNTIME_STATE_LABELS.DRY_RUN).toBe("演练模式");
    expect(formatRuntimeText("SERIAL_GAP/2: ENCODER_CW → CTRL+TAB · dry-run")).toBe("SERIAL_GAP/2: ENCODER_CW → CTRL+TAB · 演练模式");
    expect(formatRuntimeText("ENCODER_PRESS → ENTER · dispatched")).toBe("ENCODER_PRESS → ENTER · 已派发");
    expect(formatRuntimeText("input is unmapped · rejected")).toBe("输入未映射 · 已拒绝");
    expect(formatRuntimeText("input dispatch rejected · rejected")).toBe("输入派发被拒绝 · 已拒绝");
    expect(formatRuntimeText("foreground restore target is missing · rejected")).toBe("前台恢复目标不存在 · 已拒绝");
    expect(formatRuntimeText("foreground restore was rejected · rejected")).toBe("前台恢复被拒绝 · 已拒绝");
    expect(MIC_REC_HINT).toContain("GPIO9");
    expect(SENSOR_HINT).toContain("GPIO16");
    expect(SENSOR_HINT).toContain("GPIO4");
    expect(MIC_REC_HINT).toContain("远");
    expect(MIC_REC_HINT).toContain("停止转写");
    expect(MIC_REC_HINT).toContain("未联网");
    expect(asrReasonLabel("CANCEL")).toBe("已取消");
    expect(asrHeadline(null, null, null)).toBe("可录音");
    expect(asrHeadline("START", null, "DONE")).toBe("正在转写…");
    expect(asrHeadline("DONE", null, "DONE")).toBe("转写完成");
    expect(asrHeadline("FAIL", "CANCEL", null)).toBe("转写已停止");
    expect(asrHeadline("FAIL", "HTTP", null)).toBe("转写失败 · 上传失败");
    expect(asrHeadline(null, null, "ACTIVE")).toBe("录音中");
    expect(rememberTranscript([], "今天天气不错", "DONE")).toEqual(["今天天气不错"]);
    expect(rememberTranscript(["今天天气不错"], null, "START")).toEqual(["今天天气不错"]);
    expect(rememberTranscript(["上一句"], "今天天气不错", "DONE")).toEqual(["今天天气不错", "上一句"]);
    expect(recReasonLabel("WIFI")).toBe("未联网");
    expect(recStateLabel("ACTIVE")).toBe("录音中");
    expect(recStateLabel("DONE")).toBe("录音完成");
    expect(recStateLabel("FAIL")).toBe("录音失败");
  });
  it("keeps daily development on the fixture while reserving a native Vite bridge for Tauri", () => {
    expect(packageJson.scripts.dev).toBe("pnpm dev:fixture");
    expect(packageJson.scripts["check:software"]).toBe("pnpm lint && pnpm typecheck && pnpm test");
    expect(packageJson.scripts["test:interaction"]).toBe("playwright test");
    expect([
      packageJson.scripts.dev,
      packageJson.scripts["check:software"],
      packageJson.scripts["test:interaction"]
    ].join(" ")).not.toMatch(/tauri|cargo|idf|ceedling|build/i);
    expect(packageJson.scripts["dev:native"]).not.toContain("fixture");
    expect(tauriConfig.build.beforeDevCommand).toBe("pnpm dev:native");
  });
  it("fixture host restores a saved versioned profile with live permission off", async () => {
    const host = createFixtureHost();
    await expect(host.loadProfile()).resolves.toBeNull();
    const saved = await host.saveProfile({ version: 1, revision: null, serial: { port: "fixture", baud: 115200 }, target: { processName: "Fixture.exe", processPath: "C:\\Fixture\\Fixture.exe" }, mappings: [
      { input: "ENCODER_CW", displayName: "上一项", keys: ["CTRL", "TAB"] },
      { input: "ENCODER_CCW", displayName: "下一项", keys: ["CTRL", "SHIFT", "TAB"] },
      { input: "ENCODER_PRESS", displayName: "新建窗口", keys: ["CTRL", "SHIFT", "N"] },
      { input: "BUTTON_A", displayName: "新建", keys: ["CTRL", "N"] },
      { input: "BUTTON_B", displayName: "确认动作", keys: ["ENTER"] }
    ] });
    expect((await host.loadProfile())?.revision).toBe(saved.revision);
    expect((await host.loadProfile())?.serial).toEqual({ port: "fixture", baud: 115200 });
    expect((await host.enableLiveForRun()).state).toBe("LIVE");
    await expect(host.pollRuntimeEvent()).resolves.toMatchObject({ lastEvent: "本浏览器夹具会话已启用实时权限。" });
    expect((await host.pollRuntimeEvent()).state).toBe("LIVE");
    expect((await host.stop()).liveEnabled).toBe(false);
    expect((await host.pollRuntimeEvent()).state).toBe("STOPPED");
    expect((await host.pollRuntimeEvent()).network.state).toBe("UNKNOWN");
    const network = await host.applyDeviceConfig("浏览器夹具串口", 115200, {
      version: 1,
      ssid: "cafe",
      password: "secret",
      apiKey: "sk-demo",
      model: "XingChenAGI/XingChenASR-V3.2-Ultra"
    });
    expect(network).toMatchObject({ state: "CONNECTED", ip: "10.0.0.8", ssid: "cafe" });
    expect((await host.loadDeviceSettings()).apiKey).toBe("sk-demo");
    expect(NETWORK_STATE_LABELS.CONNECTED).toBe("已连接");
    expect(deviceSettingsError({ version: 1, ssid: "", password: "", apiKey: "", model: "XingChenAGI/XingChenASR-V3.2-Ultra" })).toContain("Wi-Fi 名称");
    expect(deviceSettingsError({ version: 1, ssid: "cafe", password: "secret", apiKey: "", model: "" })).toBeNull();
    expect(deviceSettingsError({ version: 1, ssid: "cafe", password: "secret", apiKey: "", model: "XingChenAGI/XingChenASR-V3.2-Ultra" })).toBeNull();
    expect(deviceSettingsError({ version: 1, ssid: "cafe", password: "", apiKey: "", model: "x".repeat(65) })).toContain("转写模型");
    expect(ssidLooksFiveG("Home-5G")).toBe(true);
    expect(ssidLooksFiveG("5guys")).toBe(false);
    expect(networkChipLabel("FAILED", "", "BAND", true)).toBe("失败 · 仅2.4G");
    expect(networkReasonLabel("BAND")).toContain("5G");
    expect(WIFI_BAND_HINT).toContain("2.4GHz");
    await expect(host.applyDeviceConfig("浏览器夹具串口", 115200, {
      version: 1,
      ssid: "Home-5G",
      password: "secret",
      apiKey: "sk-demo",
      model: "XingChenAGI/XingChenASR-V3.2-Ultra"
    })).resolves.toMatchObject({ state: "FAILED", reason: "BAND" });
  });
  it("maps a recorded keyboard event to the same chord tokens the host persists", () => {
    expect(primaryFromKeyboardEvent({ code: "Tab", key: "Tab" })).toBe("TAB");
    expect(primaryFromKeyboardEvent({ code: "Digit1", key: "1" })).toBe("1");
    expect(primaryFromKeyboardEvent({ code: "BracketLeft", key: "{" })).toBe("[");
    expect(modifiersFromKeyboardEvent({ ctrlKey: true, altKey: false, shiftKey: false })).toEqual(["CTRL"]);
    expect(chordIdentity(["CTRL", "TAB", "1"])).toBe(chordIdentity(["1", "TAB", "CTRL"]));
    expect(canonicalChord(["CTRL", "TAB", "1"])).toBe("CTRL+TAB+1");
  });
});
