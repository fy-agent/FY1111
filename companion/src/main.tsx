import { StrictMode } from "react";
import { createRoot } from "react-dom/client";
import { App } from "./App";
import { createFixtureHost, tauriHost } from "./host";

const useFixture = import.meta.env.VITE_COMPANION_FIXTURE === "true";
createRoot(document.getElementById("root")!).render(<StrictMode><App host={useFixture ? createFixtureHost() : tauriHost} /></StrictMode>);

