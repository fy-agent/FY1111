#![deny(unsafe_op_in_unsafe_fn)]

pub mod input;
pub mod profile;
pub mod runtime;
pub mod serial;
pub mod target;
pub mod windows_foreground_restore;

use serde::{Deserialize, Serialize};

pub mod commands {
    use super::*;
    use crate::profile::{ProfileDraft, ProfileStore, ProfileTarget};
    use crate::runtime::{
        RuntimeController, RuntimeError, RuntimeMode, RuntimeStatus, WindowsInputDispatcher,
        WindowsModifierState,
    };
    use crate::serial::SerialPortSource;
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
        SerialPortSource::available_ports().map_err(|_| "serial ports are unavailable".to_owned())
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
        if let Some(saved) = profile.as_ref() {
            runtime
                .set_profile(saved.clone())
                .map_err(|_| "saved profile is invalid".to_owned())?;
        }
        Ok(profile)
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
        let profile = load_profile_into_runtime(&store, &mut runtime)?;
        let source = SerialPortSource::open(&profile.serial.port, profile.serial.baud)
            .map_err(|_| "selected serial port could not be opened".to_owned())?;
        runtime
            .start_dry_run(Box::new(source))
            .map_err(|error| error.to_string())
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
        let profile = load_profile_into_runtime(&store, &mut runtime)?;
        let source = SerialPortSource::open(&profile.serial.port, profile.serial.baud)
            .map_err(|_| "selected serial port could not be opened".to_owned())?;
        runtime
            .enable_live_for_run(Box::new(source))
            .map_err(|error| error.to_string())
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
