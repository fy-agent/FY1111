#![cfg_attr(not(debug_assertions), windows_subsystem = "windows")]

fn main() {
    tauri::Builder::default()
        .manage(ventured_companion::commands::AppState::default())
        .invoke_handler(tauri::generate_handler![
            ventured_companion::commands::list_ports,
            ventured_companion::commands::capture_target_after_delay,
            ventured_companion::commands::load_profile,
            ventured_companion::commands::save_profile,
            ventured_companion::commands::start_dry_run,
            ventured_companion::commands::enable_live_for_run,
            ventured_companion::commands::poll_runtime_event,
            ventured_companion::commands::stop_runtime,
        ])
        .run(tauri::generate_context!())
        .expect("failed to start VentureD Companion");
}
