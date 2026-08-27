import { defineConfig } from "@playwright/test";

export default defineConfig({
  testDir: "./tests",
  testMatch: "visual.spec.ts",
  use: { channel: "chrome", viewport: { width: 760, height: 780 } },
  webServer: { command: "pnpm dev:fixture", url: "http://127.0.0.1:1425", reuseExistingServer: false }
});
