import { expect, test } from "@playwright/test";

test("中文夹具窗口无横向溢出，并展示三个固定输入行", async ({ page }) => {
  await page.goto("http://127.0.0.1:1425");
  await expect(page.locator("html")).toHaveAttribute("lang", "zh-CN");
  await expect(page.getByRole("heading", { name: "VentureD Companion" })).toBeVisible();
  await expect(page.getByLabel("固定输入映射").locator(".mapping")).toHaveCount(3);
  await expect(page.getByText("GPIO8 外接确认/动作按钮", { exact: true })).toBeVisible();
  await expect(page.locator(".state")).toHaveText("已停止");
  await expect(page.getByRole("button", { name: "为本次运行启用实时模式" })).toBeDisabled();
  await expect(page.getByText("尚未捕获。请先切到目标应用，再点 3 秒后捕获。")).toBeVisible();
  await expect(page.getByRole("button", { name: "设置 未连接" })).toBeVisible();
  await expect(page.locator(".net")).toHaveText("未连接");
  await expect(page.getByText("Wi-Fi 名称")).toHaveCount(0);
  const initialText = await page.locator("body").innerText();
  expect(initialText).not.toMatch(/Hook|Codex 项目目录|检查 Hook/);
  for (const formerEnglishText of ["Disconnected", "Select a port", "Clockwise", "Counterclockwise", "Encoder press", "Last event"]) {
    expect(initialText).not.toContain(formerEnglishText);
  }
  await expect(page).toHaveScreenshot("companion-fixture.png", { fullPage: true });
  expect(await page.evaluate(() => document.documentElement.scrollWidth <= document.documentElement.clientWidth)).toBeTruthy();

  await page.getByRole("button", { name: "刷新" }).click();
  await expect(page.getByText("已刷新串口列表，未打开任何串口。")).toBeVisible();
  await page.getByRole("combobox", { name: "设备串口" }).selectOption("浏览器夹具串口");
  await page.getByRole("button", { name: "3 秒后捕获" }).click();
  await page.getByRole("button", { name: "保存配置" }).click();
  await expect(page.getByRole("button", { name: "启动演练模式" })).toBeEnabled();
  await page.getByRole("button", { name: "启动演练模式" }).click();
  await expect(page.locator(".state")).toHaveText("演练模式");
  await expect(page.getByText("ENCODER_CW → CTRL+TAB · 演练模式")).toBeVisible();
  await expect(page.getByRole("button", { name: "为本次运行启用实时模式" })).toBeDisabled();
  await page.getByRole("button", { name: "停止运行" }).click();
  await expect(page.locator(".state")).toHaveText("已停止");
  await page.getByRole("button", { name: "顺时针旋转 快捷键" }).click();
  await expect(page.getByRole("button", { name: "顺时针旋转 快捷键" })).toHaveAttribute("aria-pressed", "true");
  await page.keyboard.down("Control");
  await page.keyboard.down("Tab");
  await page.keyboard.down("1");
  await page.keyboard.up("1");
  await page.keyboard.up("Tab");
  await page.keyboard.up("Control");
  await expect(page.getByRole("button", { name: "顺时针旋转 快捷键" })).toHaveText("CTRL+TAB+1");
  await expect(page.getByText("快捷键已更新；保存配置后才会用于派发。")).toBeVisible();

  await page.getByRole("button", { name: /设置/ }).click();
  await expect(page.getByText("开发板只支持 2.4GHz Wi-Fi")).toBeVisible();
  await page.getByLabel("Wi-Fi 名称").fill("cafe");
  await page.getByLabel("Wi-Fi 密码").fill("secret");
  await expect(page.getByText("SiliconFlow API Key 与转写模型已屏蔽")).toBeVisible();
  await page.getByRole("button", { name: "保存并下发联网" }).click();
  await expect(page.getByText("设备已联网。")).toBeVisible();
  await expect(page.getByText("已连接 10.0.0.8")).toBeVisible();
  await page.getByLabel("Wi-Fi 名称").fill("Home-5G");
  await expect(page.getByRole("alert").filter({ hasText: "当前名称像 5G 热点" })).toBeVisible();
  await page.getByRole("button", { name: "保存并下发联网" }).click();
  await expect(page.locator(".net")).toHaveText("失败 · 仅2.4G");
});
