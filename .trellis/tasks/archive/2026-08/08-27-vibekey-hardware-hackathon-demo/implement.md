# Implementation Plan

## 1. Pre-implementation gate

- [x] Receive explicit approval of the final `prd.md`, `design.md`, and this
      plan in a message after the planning summary.
- [x] Run `task.py start` and confirm status `in_progress`.
- [x] Load `trellis-before-dev` and all relevant current-project spec indexes.
- [x] Recheck both repositories' `git status --short --branch`.
- [x] Recheck source HEAD and the eight recorded SHA-256 values.
- [x] Declare the write set as the task directory plus new `firmware/`,
      `companion/`, `protocol/`, `docs/`, root manifests, and README changes in
      `VentureD` only.

Stop if the source checkout has a different HEAD/hash or overlapping changes
that make provenance ambiguous. Do not reset or clean either repository.

## 2. Establish independent project skeleton

- [x] Add root documentation and ignore rules only as required by the demo.
- [x] Create `firmware/board-c-demo`, `companion`, `protocol`, and `docs`
      boundaries.
- [x] Add `docs/source-provenance.md` with source commit/path/hash/destination
      entries before adapting copied behavior.
- [x] Search the current project before defining every protocol token, pin, and
      shortcut constant so each has one owner.

Validation:

```powershell
rg -n "vibekey_new|realkey" . -g "!\.trellis/**"
git diff --check
```

Expected: no runtime dependency on the source path and no whitespace errors.

## 3. Implement Board C firmware baseline

- [x] Add the ESP-IDF project, target documentation, Kconfig, and defaults.
- [x] Define the Board C pin manifest with GPIO6/GPIO7 rotary encoder and an
      independent GPIO8 confirm/action button, and
      reserved documented resources.
- [x] Adapt the EC11 pure core and add provenance-preserving comments only for
      non-obvious retained safeguards.
- [x] Implement the Board C GPIO/ISR/queue adapter without importing VibeKey's
      Control Router, HID, identity, MSC, display, or BLE dependencies.
- [x] Implement the `VKEY_INPUT/1` formatter with bounded output and monotonic
      sequence values.
- [x] Add focused host tests for detents, bounce/reversal, invalid transitions,
      button debounce, pin manifest, and exact event formatting.

Offline validation:

```powershell
ceedling test:all
```

Run from `firmware/board-c-demo/host_tests` if Ceedling is available. Record
the installed Ceedling version as environment evidence because this repository
does not currently include a Gemfile or lockfile. If unavailable, report the
missing prerequisite; do not weaken the tests or claim them from the ESP-IDF
build.

ESP-IDF build validation in an exported 5.5.4 environment:

```powershell
idf.py --version
idf.py -C firmware/board-c-demo set-target esp32s3
idf.py -C firmware/board-c-demo -B <fresh-build-dir> -D SDKCONFIG=<fresh-sdkconfig> build
```

Inspect generated project description, `sdkconfig`, image size, and flash args.
Do not run `flash`, `erase-flash`, `monitor`, or open a COM port in this step.

## 4. Implement the native Companion core

- [x] Create an independent Rust/Tauri crate with pinned manifests and lockfile.
- [x] Implement strict physical input, key token, chord, target, profile, and
      runtime types.
- [x] Implement bounded serial line reading and strict `VKEY_INPUT/1` decoding.
- [x] Implement sequence duplicate/backward rejection and forward-gap status.
- [x] Implement atomic profile save, backup-on-change, revision validation, and
      reload behavior in a temporary test directory.
- [x] Adapt the foreground process probe and fail-closed decision core.
- [x] Adapt the clean-key-state guard and `SendInput` dispatcher behind an
      interface that tests and dry-run cannot reach.
- [x] Implement serial enumeration and runtime start/stop state machine.
- [x] Expose the minimal typed Tauri command surface.

Validation:

```powershell
cargo fmt --manifest-path companion/src-tauri/Cargo.toml --check
cargo clippy --manifest-path companion/src-tauri/Cargo.toml --all-targets --locked -- -D warnings
cargo test --manifest-path companion/src-tauri/Cargo.toml --all-targets --locked
```

Required regressions include malformed/overlong records, unknown fields,
duplicates, sequence behavior, unmapped inputs, duplicate chords, stale save,
restart live-off state, wrong foreground target, dirty modifiers, and a dry-run
test proving zero dispatcher calls.

## 5. Implement the compact Companion UI

- [x] Create the independent React/Vite frontend and typed host boundary.
- [x] Implement the single-window device, target, three mapping rows, controls,
      and last-event status layout.
- [x] Add serial refresh/selection and delayed target capture interactions.
- [x] Add display-name and canonical chord validation with immediate duplicate
      explanations.
- [x] Add revision-aware save/reload and runtime controls.
- [x] Make live enable visibly distinct, session-only, and reset after restart.
- [x] Create a secret-free browser fixture whose host methods are read-only
      local fakes.

Validation:

```powershell
pnpm --dir companion install --frozen-lockfile
pnpm --dir companion check:software
pnpm --dir companion test:interaction
```

Visually inspect the committed screenshot at the target viewport and check
that there is no clipped control, horizontal overflow, duplicate-input
ambiguity, or misleading live/connected claim.

## 6. Native build and bounded runtime check

- [x] Build the Tauri application without opening a serial port or enabling
      live dispatch.
- [ ] Launch it with redirected output and a hidden helper console if needed.
- [ ] Confirm the native window title, process liveness, initial stopped state,
      and live-off state.
- [ ] Close the application normally and inspect Git status for generated
      schemas or build artifacts; do not silently stage them.

Validation commands are selected from the generated manifests, expected to be
equivalent to:

```powershell
pnpm --dir companion tauri build --debug --no-bundle
pnpm --dir companion tauri dev
```

This step does not authorize COM, foreground capture, profile writes through
the live UI, `SendInput`, or hardware interaction.

## 7. Documentation and configuration review

- [x] Document the one-screen setup flow and dry-run-first usage.
- [x] Document ESP-IDF 5.5.4 environment checks and parameterized build.
- [x] Document flash and monitor commands separately with explicit placeholders
      and the fresh authorization requirement.
- [x] Document Board C pin ownership and disabled future microphone seam.
- [x] Document known evidence boundaries and unsupported features.
- [x] Confirm no credentials, machine-specific ports, private paths, or copied
      untracked source files entered the repository.

## 8. Full quality gate

- [x] Run all firmware host tests available in the current host environment.
- [x] Run a fresh ESP-IDF build.
- [x] Run Rust format, Clippy, and all tests.
- [x] Run Development Debug frontend lint, typecheck, unit tests, and browser
      interaction/visual regression without invoking a native or firmware build.
- [x] Run `git diff --check` and inspect the full diff/stat.
- [x] Search for forbidden source paths, hard-coded COM ports, credentials,
      accidental BLE/Wi-Fi/Agent code, and generated build outputs.
- [x] Recompute source HEAD/status/hashes and compare byte-for-byte with the
      planning snapshot.

## 9. Deferred real-hardware gates

Do not execute these as part of normal implementation. Record them as pending
until the user supplies fresh authorization:

- identify the exact Board C core module and flash/PSRAM;
- enumerate and select the actual COM port;
- approve the candidate commit/image and flash write regions;
- approve reset and monitor access;
- verify encoder events on real hardware;
- authorize a specific harmless foreground target for live `SendInput`.

Offline completion must not be relabeled as flash, COM, physical encoder,
native shortcut, or competition HIL acceptance.

## 10. Rollback points

- Firmware, protocol, Companion core, and UI are separate new-directory
  boundaries and can be reverted independently before integration.
- Profile tests use temporary directories; they never touch a real user
  profile.
- If source provenance changes, stop and revise the manifest rather than
  overwriting source or forcing a copy.
- No rollback operation may reset, clean, checkout, or otherwise mutate
  user-owned changes in either repository.

## 11. Approved Development Debug refinement

- [x] Translate the Companion's visible UI, fixture runtime text, validation,
      labels, and accessibility labels to Chinese while preserving protocol IDs
      and shortcut tokens.
- [x] Project `ENCODER_PRESS` as the independent GPIO8 external confirm/action
      button; leave encoder SW unconnected in the documented topology.
- [x] Add fixture-first `dev:fixture`, `check:software`, and
      `test:interaction` commands without making native or firmware builds part
      of ordinary software development.
- [x] Document Development Debug, Native Integration, and Hardware Acceptance
      as separate command/authorization stages, including GPIO6/7 camera and
      GPIO8 DHT11 conflicts.

## 12. Remaining reuse-first Companion increment

Copy or narrowly adapt proven tracked VibeKey modules. Do not redesign
equivalent restore or Hook inspection. Keep the existing compact three-row
Companion. Do not import Setup UI, MappingEditor, multi-application catalogs,
proposal/install/replace/removal Hook builders, Observer execution, or
application launching.

Source checkout `C:\Users\xk\Desktop\realkey\vibekey_new` remains read-only.

- [x] Copy `experiments/desktop-control-agent/src/windows_foreground_restore.rs`
      almost verbatim into `companion/src-tauri/src/`, adapting only local
      target/trait imports. Preserve exact-path window enumeration, minimized
      restore, bounded input-queue attachment, `SetForegroundWindow`, and
      post-focus verification.
- [x] Extend the existing VentureD target decision with source restore
      outcomes (`Unchanged`, `Restored`, `Missing`, `Rejected`). Live dispatch
      may restore an already-running exact-path window, then must recheck
      exact foreground identity before `SendInput`. Missing/Rejected yields
      zero dispatch and never starts an executable.
- [x] Copy only the read-only managed-Observer classifier and its parsing
      tests from `experiments/codex-desktop-observer/src/hook_config.rs` into
      a small local `hook_status` module. Do not copy proposal, install,
      replace, removal, backup, or restore builders.
- [x] Reuse the redacted projection from
      `experiments/desktop-control-agent/setup-ui/src-tauri/src/hook_setup.rs`
      lines 1641-1697: React may see only `missing`, `not_installed`,
      `installed_unverified`, or `blocked`, plus bounded event-group/handler
      counts. No Hook commands or private paths in the React DTO.
- [x] Add a compact Hook status card: user supplies an explicit Codex project
      directory (absolute path text field; no new Tauri dialog plugin unless
      already present), native host reads only that project's
      `.codex/hooks.json`.
- [x] Update `docs/source-provenance.md` with the additional source paths,
      commit, SHA-256 values, and adaptation boundary.
- [x] Fixture host remains fake: it cannot open serial, write the real
      profile, capture a real target, call `SendInput`, or read a real
      `hooks.json`.
- [x] Keep dry-run free of restorer and dispatcher construction.

Validation:

```powershell
cargo fmt --manifest-path companion/src-tauri/Cargo.toml --check
cargo clippy --manifest-path companion/src-tauri/Cargo.toml --all-targets --locked -- -D warnings
cargo test --manifest-path companion/src-tauri/Cargo.toml --all-targets --locked
pnpm --dir companion check:software
pnpm --dir companion test:interaction
```

Required regressions: restore missing/rejected with zero dispatch; restore then
exact recheck then one dispatch; already-matching foreground may leave restore
Unchanged and still dispatch; dry-run never restores or dispatches; source
HEAD/status/hashes unchanged.

Do not run native GUI launch, COM, flash, monitor, or `SendInput` in this
increment.

## 13. First-demo scope correction: drop Hook, keep restore + shortcuts

The first Board C demo is only:

```text
capture one foreground target
  -> encoder CW/CCW or GPIO8 press
  -> restore that already-running window
  -> send one of three locally mapped shortcuts
```

Codex Hook/Observer inspection is out of scope. Delete the copied
`hook_status` module, Tauri command, React card, and fixture fake.

- [x] Remove `inspect_hook_status` and all Hook DTOs from the Companion host,
      UI, fixture, visual regression, and runtime contracts.
- [x] Keep delayed foreground capture, exact-path restore, and three mapping
      rows. Live dispatch restores first, rechecks identity, then sends the
      chord. Dry-run still never constructs a restorer or dispatcher.
- [x] Firmware: GPIO8 is input-only (no interrupt); encoder ISR is attached
      before ANYEDGE; button scans at 10 ms; `VKEY_INPUT/1` is unbuffered on
      ESP32-S3 USB-Serial-JTAG. Do not import Control Router, HID, or BLE.
- [x] Build firmware, flash the enumerated Board C port after the code change,
      then launch native Companion for debug. Do not enable live `SendInput`
      until the user captures a harmless target.
