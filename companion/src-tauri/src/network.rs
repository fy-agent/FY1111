use std::fmt::{Display, Formatter};

use serde::{Deserialize, Serialize};

pub const DEFAULT_CLOUD_MODEL: &str = "FunAudioLLM/SenseVoiceSmall";
pub const CLOUD_MODELS: [&str; 2] = [
    "FunAudioLLM/SenseVoiceSmall",
    "TeleAI/TeleSpeechASR",
];

#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize, Default)]
#[serde(rename_all = "SCREAMING_SNAKE_CASE")]
pub enum NetworkState {
    #[default]
    Unknown,
    Disconnected,
    Connecting,
    Connected,
    Failed,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize, Default)]
#[serde(rename_all = "camelCase", deny_unknown_fields)]
pub struct NetworkStatus {
    pub state: NetworkState,
    pub ssid: String,
    pub ip: String,
    pub rssi: Option<i32>,
    pub reason: Option<String>,
    #[serde(default)]
    pub ping_host: Option<String>,
    #[serde(default)]
    pub ping_ok: Option<bool>,
    #[serde(default)]
    pub ping_ms: Option<u32>,
    #[serde(default)]
    pub ping_lost: Option<u32>,
    #[serde(default)]
    pub ping_sent: Option<u32>,
    #[serde(default)]
    pub last_log: Option<String>,
    #[serde(default)]
    pub beats: Option<u32>,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct DeviceConfigRecord {
    pub seq: u32,
    pub ssid: String,
    pub password: String,
    pub api_key: String,
    pub model: String,
}

impl DeviceConfigRecord {
    pub fn line(&self) -> Result<String, NetworkError> {
        let json = serde_json::to_string(self).map_err(|_| NetworkError::Invalid)?;
        Ok(format!("VKEY_CONFIG/1 {json}\n"))
    }
}

pub fn model_allowed(model: &str) -> bool {
    CLOUD_MODELS.contains(&model)
}

pub fn placeholder_api_key(api_key: &str) -> String {
    if api_key.trim().is_empty() {
        "sk-debug".to_owned()
    } else {
        api_key.to_owned()
    }
}

pub fn ssid_looks_5g(ssid: &str) -> bool {
    let bytes = ssid.as_bytes();
    let mut index = 0;
    while index < bytes.len() {
        if bytes[index] == b'5' {
            let mut cursor = index + 1;
            while cursor < bytes.len() && bytes[cursor] == b' ' {
                cursor += 1;
            }
            if cursor < bytes.len() && bytes[cursor].eq_ignore_ascii_case(&b'g') {
                cursor += 1;
                if cursor + 1 < bytes.len()
                    && bytes[cursor].eq_ignore_ascii_case(&b'h')
                    && bytes[cursor + 1].eq_ignore_ascii_case(&b'z')
                {
                    cursor += 2;
                }
                if bytes.get(cursor).is_none_or(|next| !next.is_ascii_alphanumeric()) {
                    return true;
                }
            }
        }
        index += 1;
    }
    false
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum NetworkError {
    Invalid,
    Unavailable,
}
impl Display for NetworkError {
    fn fmt(&self, formatter: &mut Formatter<'_>) -> std::fmt::Result {
        formatter.write_str(match self {
            Self::Invalid => "device settings are invalid",
            Self::Unavailable => "device link is unavailable",
        })
    }
}
impl std::error::Error for NetworkError {}

#[cfg(test)]
mod tests {
    use super::ssid_looks_5g;

    #[test]
    fn ssid_5g_heuristic_matches_common_hotspot_names() {
        assert!(ssid_looks_5g("Home-5G"));
        assert!(ssid_looks_5g("office_5ghz"));
        assert!(ssid_looks_5g("5G-office"));
        assert!(!ssid_looks_5g("5guys"));
        assert!(!ssid_looks_5g("cafe"));
        assert!(!ssid_looks_5g("channel5"));
    }
}
