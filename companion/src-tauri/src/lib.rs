#![deny(unsafe_op_in_unsafe_fn)]

pub mod device_settings;
pub mod input;
pub mod network;
pub mod profile;
pub mod runtime;
pub mod serial;
pub mod target;
pub mod usb_link;
pub mod windows_foreground_restore;

use serde::{Deserialize, Serialize};

pub mod commands {
    use super::*;
    use crate::device_settings::{DeviceSettings, DeviceSettingsStore};
    use crate::network::NetworkStatus;
    use crate::profile::{ProfileDraft, ProfileStore, ProfileTarget};
    use crate::runtime::{
        RuntimeController, RuntimeError, RuntimeMode, RuntimeStatus, WindowsInputDispatcher,
        WindowsModifierState,
    };
    use crate::usb_link::{UsbLinkSource, USB_LINK_BAUD, USB_LINK_ID};
    use crate::target::{ForegroundProbe, WindowsForegroundProbe};
    use crate::windows_foreground_restore::WindowsForegroundTargetRestorer;
    use tauri::Manager;

    #[derive(Default)]
    pub struct AppState {
        runtime: std::sync::Mutex<RuntimeController>,
    }
    impl AppState {
        pub fn runtime(&self) -> Result<std::sync::MutexGuard<'_, RuntimeController>, String> {
            self.runtime
                .lock()
                .map_err(|_| "runtime state is unavailable".to_owned())
        }
    }

    #[derive(Debug, Clone, Serialize, Deserialize)]
    #[serde(rename_all = "camelCase", deny_unknown_fields)]
    pub struct TargetDraft {
        pub process_name: String,
        pub process_path: String,
    }

    fn profile_store(app: &tauri::AppHandle) -> Result<ProfileStore, String> {
        let directory = app
            .path()
            .app_config_dir()
            .map_err(|_| "profile storage is unavailable".to_owned())?;
        Ok(ProfileStore::new(directory.join("profile.json")))
    }

    fn device_store(app: &tauri::AppHandle) -> Result<DeviceSettingsStore, String> {
        let directory = app
            .path()
            .app_config_dir()
            .map_err(|_| "device settings storage is unavailable".to_owned())?;
        Ok(DeviceSettingsStore::new(directory.join("device.json")))
    }

    fn normalize_link(draft: &mut ProfileDraft) {
        draft.serial.port = USB_LINK_ID.to_owned();
        if draft.serial.baud == 0 {
            draft.serial.baud = USB_LINK_BAUD;
        }
    }

    fn attach_usb(runtime: &mut RuntimeController) -> Result<(), String> {
        if runtime.source_matches(USB_LINK_ID, USB_LINK_BAUD) {
            return Ok(());
        }
        runtime
            .ensure_stopped()
            .map_err(|error| error.to_string())?;
        runtime.close_source();
        let source = UsbLinkSource::open().map_err(|_| "未插入 Board C USB 设备".to_owned())?;
        runtime.attach_source(USB_LINK_ID.to_owned(), USB_LINK_BAUD, Box::new(source));
        Ok(())
    }

    fn start_shortcut(
        runtime: &mut RuntimeController,
        mode: RuntimeMode,
    ) -> Result<RuntimeStatus, String> {
        attach_usb(runtime)?;
        runtime.start_existing(mode).map_err(|error| error.to_string())
    }

    pub fn save_profile_to_store(
        store: &ProfileStore,
        draft: ProfileDraft,
    ) -> Result<ProfileDraft, String> {
        let expected = draft.revision.clone();
        store
            .save(draft, expected.as_deref())
            .map_err(|error| error.to_string())
    }

    pub fn load_profile_from_store(store: &ProfileStore) -> Result<Option<ProfileDraft>, String> {
        store
            .load()
            .map_err(|_| "saved profile is invalid".to_owned())
    }

    fn load_profile_into_runtime(
        store: &ProfileStore,
        runtime: &mut RuntimeController,
    ) -> Result<ProfileDraft, String> {
        let profile = store
            .load()
            .map_err(|_| "profile is unavailable".to_owned())?
            .ok_or_else(|| "a saved profile is required".to_owned())?;
        runtime
            .set_profile(profile.clone())
            .map_err(|error| error.to_string())?;
        Ok(profile)
    }

    #[tauri::command]
    pub fn list_ports() -> Result<Vec<String>, String> {
        Ok(if UsbLinkSource::present() {
            vec![USB_LINK_ID.to_owned()]
        } else {
            Vec::new()
        })
    }

    #[tauri::command]
    pub fn capture_target_after_delay(
        state: tauri::State<'_, AppState>,
    ) -> Result<TargetDraft, String> {
        state
            .runtime()?
            .ensure_stopped()
            .map_err(|error| error.to_string())?;
        std::thread::sleep(std::time::Duration::from_secs(3));
        // Hold the guard through the probe so another command cannot start the
        // runtime between the post-delay check and foreground capture.
        let runtime_guard = state.runtime()?;
        runtime_guard
            .ensure_stopped()
            .map_err(|error| error.to_string())?;
        let identity = WindowsForegroundProbe
            .foreground_identity()
            .map_err(|_| "foreground target is unavailable".to_owned())?
            .ok_or_else(|| "no foreground target is available".to_owned())?;
        drop(runtime_guard);
        Ok(TargetDraft {
            process_name: identity.process_name,
            process_path: identity.process_path,
        })
    }

    #[tauri::command]
    pub fn load_profile(
        app: tauri::AppHandle,
        state: tauri::State<'_, AppState>,
    ) -> Result<Option<ProfileDraft>, String> {
        let profile = load_profile_from_store(&profile_store(&app)?)?;
        let mut runtime = state.runtime()?;
        runtime
            .ensure_stopped()
            .map_err(|error| error.to_string())?;
        if let Some(mut saved) = profile {
            normalize_link(&mut saved);
            runtime
                .set_profile(saved.clone())
                .map_err(|_| "saved profile is invalid".to_owned())?;
            return Ok(Some(saved));
        }
        Ok(None)
    }

    #[tauri::command]
    pub fn save_profile(
        app: tauri::AppHandle,
        state: tauri::State<'_, AppState>,
        draft: ProfileDraft,
    ) -> Result<ProfileDraft, String> {
        let mut runtime = state.runtime()?;
        runtime
            .ensure_stopped()
            .map_err(|error| error.to_string())?;
        let mut draft = draft;
        normalize_link(&mut draft);
        let saved = save_profile_to_store(&profile_store(&app)?, draft)?;
        runtime
            .set_profile(saved.clone())
            .map_err(|error| error.to_string())?;
        Ok(saved)
    }

    #[tauri::command]
    pub fn start_dry_run(
        app: tauri::AppHandle,
        state: tauri::State<'_, AppState>,
    ) -> Result<RuntimeStatus, String> {
        let store = profile_store(&app)?;
        let mut runtime = state.runtime()?;
        runtime
            .ensure_stopped()
            .map_err(|error| error.to_string())?;
        let _profile = load_profile_into_runtime(&store, &mut runtime)?;
        start_shortcut(&mut runtime, RuntimeMode::DryRun)
    }

    #[tauri::command]
    pub fn enable_live_for_run(
        app: tauri::AppHandle,
        state: tauri::State<'_, AppState>,
    ) -> Result<RuntimeStatus, String> {
        let store = profile_store(&app)?;
        let mut runtime = state.runtime()?;
        runtime
            .ensure_stopped()
            .map_err(|error| error.to_string())?;
        let _profile = load_profile_into_runtime(&store, &mut runtime)?;
        start_shortcut(&mut runtime, RuntimeMode::Live)
    }

    #[tauri::command]
    pub fn poll_runtime_event(state: tauri::State<'_, AppState>) -> Result<RuntimeStatus, String> {
        let mut runtime = state.runtime()?;
        match runtime.status().state {
            RuntimeMode::DryRun => runtime.poll_dry_run().map_err(|error| error.to_string()),
            RuntimeMode::Live => {
                let probe = WindowsForegroundProbe;
                let restorer = WindowsForegroundTargetRestorer;
                let modifiers = WindowsModifierState;
                let mut dispatcher = WindowsInputDispatcher;
                runtime
                    .poll_live(&probe, &restorer, &modifiers, &mut dispatcher)
                    .map_err(|error| error.to_string())
            }
            RuntimeMode::Stopped => Err(RuntimeError::Stopped.to_string()),
        }
    }

    #[tauri::command]
    pub fn stop_runtime(state: tauri::State<'_, AppState>) -> Result<RuntimeStatus, String> {
        Ok(state.runtime()?.stop())
    }

    #[tauri::command]
    pub fn load_device_settings(
        app: tauri::AppHandle,
    ) -> Result<DeviceSettings, String> {
        Ok(device_store(&app)?
            .load()
            .map_err(|_| "saved device settings are invalid".to_owned())?
            .unwrap_or_default())
    }

    #[tauri::command]
    pub fn save_device_settings(
        app: tauri::AppHandle,
        draft: DeviceSettings,
    ) -> Result<DeviceSettings, String> {
        device_store(&app)?
            .save(draft)
            .map_err(|error| error.to_string())
    }

    #[derive(Debug, Clone, serde::Deserialize)]
    #[serde(rename_all = "camelCase", deny_unknown_fields)]
    pub struct ApplyDeviceConfigRequest {
        pub port: String,
        pub baud: u32,
        pub settings: DeviceSettings,
    }

    #[tauri::command]
    pub fn apply_device_config(
        app: tauri::AppHandle,
        state: tauri::State<'_, AppState>,
        request: ApplyDeviceConfigRequest,
    ) -> Result<NetworkStatus, String> {
        let _ = (request.port.as_str(), request.baud);
        request
            .settings
            .validate()
            .map_err(|error| error.to_string())?;
        let saved = device_store(&app)?
            .save(request.settings)
            .map_err(|error| error.to_string())?;
        let mut runtime = state.runtime()?;
        attach_usb(&mut runtime)?;
        runtime
            .apply_config(&saved)
            .map_err(|error| error.to_string())
    }

    #[tauri::command]
    pub fn poll_network_status(
        state: tauri::State<'_, AppState>,
    ) -> Result<NetworkStatus, String> {
        let mut runtime = state.runtime()?;
        if !runtime.has_source() {
            let _ = attach_usb(&mut runtime);
        }
        Ok(runtime.poll_network())
    }

    pub fn target_from_draft(draft: TargetDraft) -> Result<ProfileTarget, String> {
        if draft.process_name.trim().is_empty() || draft.process_path.trim().is_empty() {
            return Err("invalid target".to_owned());
        }
        Ok(ProfileTarget {
            process_name: draft.process_name,
            process_path: draft.process_path,
        })
    }

    #[cfg(test)]
    mod tests {
        use super::*;
        use crate::input::InputId;
        use crate::profile::PROFILE_VERSION;
        use crate::profile::{MappingDraft, ProfileSerial};
        use tempfile::tempdir;
        fn draft() -> ProfileDraft {
            ProfileDraft {
                version: PROFILE_VERSION,
                revision: None,
                serial: ProfileSerial {
                    port: "fixture".into(),
                    baud: 115200,
                },
                target: Some(ProfileTarget {
                    process_name: "Fixture.exe".into(),
                    process_path: r"C:\Fixture.exe".into(),
                }),
                mappings: vec![
                    MappingDraft {
                        input: InputId::EncoderCw,
                        display_name: "Previous".into(),
                        keys: vec!["CTRL".into(), "TAB".into()],
                    },
                    MappingDraft {
                        input: InputId::EncoderCcw,
                        display_name: "Next".into(),
                        keys: vec!["CTRL".into(), "SHIFT".into(), "TAB".into()],
                    },
                    MappingDraft {
                        input: InputId::EncoderPress,
                        display_name: "New window".into(),
                        keys: vec!["CTRL".into(), "SHIFT".into(), "N".into()],
                    },
                    MappingDraft {
                        input: InputId::ButtonA,
                        display_name: "New".into(),
                        keys: vec!["CTRL".into(), "N".into()],
                    },
                    MappingDraft {
                        input: InputId::ButtonB,
                        display_name: "Confirm".into(),
                        keys: vec!["ENTER".into()],
                    },
                ],
            }
        }
        #[test]
        fn save_command_core_persists_to_an_injected_temporary_store() {
            let directory = tempdir().unwrap();
            let store = ProfileStore::new(directory.path().join("profile.json"));
            let saved = save_profile_to_store(&store, draft()).unwrap();
            assert!(store.path().exists());
            assert_eq!(store.load().unwrap(), Some(saved));
        }
        #[test]
        fn saved_profile_round_trips_into_a_restarted_live_off_runtime() {
            let directory = tempdir().unwrap();
            let store = ProfileStore::new(directory.path().join("profile.json"));
            let saved = save_profile_to_store(&store, draft()).unwrap();
            let loaded = load_profile_from_store(&store).unwrap().unwrap();
            assert_eq!(loaded, saved);
            let mut restarted = RuntimeController::default();
            restarted.set_profile(loaded).unwrap();
            assert_eq!(restarted.status().state, RuntimeMode::Stopped);
            assert!(!restarted.status().live_enabled);
        }
    }
}
