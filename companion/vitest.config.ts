import { defineConfig } from "vitest/config";

export default defineConfig({ test: { exclude: ["tests/visual.spec.ts", "node_modules/**", "dist/**"] } });

