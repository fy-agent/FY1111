use std::collections::VecDeque;
use std::fmt::{Display, Formatter};
use std::io::Read;
use std::time::Duration;

use serde::Deserialize;

use crate::input::InputId;

pub const MAX_LINE_BYTES: usize = 1024;
const PREFIX: &str = "VKEY_INPUT/1 ";

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct SerialEvent {
    pub sequence: u32,
    pub input: InputId,
    pub gap_missed: Option<u32>,
}

pub trait EventSource: Send {
    fn poll_event(&mut self) -> Result<Option<SerialEvent>, SerialError>;
    fn close(&mut self) {}
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum SerialError {
    Unavailable,
    Read,
}
impl Display for SerialError {
    fn fmt(&self, formatter: &mut Formatter<'_>) -> std::fmt::Result {
        formatter.write_str(match self {
            Self::Unavailable => "serial source is unavailable",
            Self::Read => "serial source read failed",
        })
    }
}
impl std::error::Error for SerialError {}

/// The production source is constructed only after a later user command starts
/// a runtime. Tests provide EventSource fakes and never open a COM device.
pub struct SerialPortSource {
    port: Box<dyn serialport::SerialPort>,
    buffered: BoundedLineBuffer,
    tracker: SequenceTracker,
}

impl SerialPortSource {
    pub fn available_ports() -> Result<Vec<String>, SerialError> {
        serialport::available_ports()
            .map(|ports| ports.into_iter().map(|port| port.port_name).collect())
            .map_err(|_| SerialError::Unavailable)
    }

    pub fn open(port_name: &str, baud: u32) -> Result<Self, SerialError> {
        if port_name.trim().is_empty() || baud == 0 {
            return Err(SerialError::Unavailable);
        }
        let port = serialport::new(port_name, baud)
            .timeout(Duration::from_millis(100))
            .open()
            .map_err(|_| SerialError::Unavailable)?;
        Ok(Self {
            port,
            buffered: BoundedLineBuffer::default(),
            tracker: SequenceTracker::default(),
        })
    }
}

impl EventSource for SerialPortSource {
    fn poll_event(&mut self) -> Result<Option<SerialEvent>, SerialError> {
        loop {
            if let Some(line) = self.buffered.push(&[]) {
                if let Some(event) = decode_line(&line) {
                    let outcome = self.tracker.observe(event.sequence);
                    if let Some(event) = accept_sequence(event, outcome) {
                        return Ok(Some(event));
                    }
                }
                continue;
            }
            let mut bytes = [0_u8; 128];
            match self.port.read(&mut bytes) {
                Ok(0) => return Ok(None),
                Ok(count) => {
                    let Some(line) = self.buffered.push(&bytes[..count]) else {
                        continue;
                    };
                    let Some(event) = decode_line(&line) else {
                        continue;
                    };
                    let outcome = self.tracker.observe(event.sequence);
                    if let Some(event) = accept_sequence(event, outcome) {
                        return Ok(Some(event));
                    }
                }
                Err(error) if error.kind() == std::io::ErrorKind::TimedOut => return Ok(None),
                Err(_) => return Err(SerialError::Read),
            }
        }
    }

    fn close(&mut self) {
        let _ = self.port.clear(serialport::ClearBuffer::All);
        self.buffered.clear();
    }
}

fn accept_sequence(mut event: SerialEvent, outcome: SequenceOutcome) -> Option<SerialEvent> {
    match outcome {
        SequenceOutcome::DuplicateOrBackward => None,
        SequenceOutcome::Accepted => Some(event),
        SequenceOutcome::Gap { missed } => {
            event.gap_missed = Some(missed);
            Some(event)
        }
    }
}

#[derive(Default)]
pub struct BoundedLineBuffer {
    buffered: Vec<u8>,
    ready: VecDeque<Vec<u8>>,
    discarding_overlong: bool,
}
impl BoundedLineBuffer {
    pub fn push(&mut self, bytes: &[u8]) -> Option<Vec<u8>> {
        for byte in bytes {
            if self.discarding_overlong {
                if *byte == b'\n' {
                    self.discarding_overlong = false;
                }
                continue;
            }
            if self.buffered.len() == MAX_LINE_BYTES {
                self.buffered.clear();
                self.discarding_overlong = *byte != b'\n';
                continue;
            }
            self.buffered.push(*byte);
            if *byte == b'\n' {
                self.ready.push_back(std::mem::take(&mut self.buffered));
            }
        }
        self.ready.pop_front()
    }
    pub fn clear(&mut self) {
        self.buffered.clear();
        self.ready.clear();
        self.discarding_overlong = false;
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum SequenceOutcome {
    Accepted,
    Gap { missed: u32 },
    DuplicateOrBackward,
}
#[derive(Default)]
pub struct SequenceTracker {
    last: Option<u32>,
}
impl SequenceTracker {
    pub fn observe(&mut self, sequence: u32) -> SequenceOutcome {
        let outcome = match self.last {
            None => SequenceOutcome::Accepted,
            Some(last) if sequence <= last => SequenceOutcome::DuplicateOrBackward,
            Some(last) if sequence > last.saturating_add(1) => SequenceOutcome::Gap {
                missed: sequence - last - 1,
            },
            Some(_) => SequenceOutcome::Accepted,
        };
        if !matches!(outcome, SequenceOutcome::DuplicateOrBackward) {
            self.last = Some(sequence);
        }
        outcome
    }
}

#[derive(Deserialize)]
#[serde(deny_unknown_fields)]
struct Record {
    seq: u32,
    input: InputId,
}
pub fn decode_line(raw: &[u8]) -> Option<SerialEvent> {
    if raw.len() > MAX_LINE_BYTES {
        return None;
    }
    let line = std::str::from_utf8(raw)
        .ok()?
        .trim_end_matches(['\r', '\n']);
    let record = serde_json::from_str::<Record>(line.strip_prefix(PREFIX)?).ok()?;
    Some(SerialEvent {
        sequence: record.seq,
        input: record.input,
        gap_missed: None,
    })
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn strict_decoder_rejects_malformed_overlong_and_unknown_fields() {
        assert_eq!(
            decode_line(br#"VKEY_INPUT/1 {"seq":1,"input":"ENCODER_CW"}"#)
                .unwrap()
                .input,
            InputId::EncoderCw
        );
        assert!(
            decode_line(br#"VKEY_INPUT/1 {"seq":1,"input":"ENCODER_CW","extra":true}"#).is_none()
        );
        assert!(decode_line(&vec![b'x'; MAX_LINE_BYTES + 1]).is_none());
        assert!(decode_line(b"VKEY_INPUT/2 {}").is_none());
    }
    #[test]
    fn tracker_reports_gaps_and_rejects_duplicates() {
        let mut tracker = SequenceTracker::default();
        assert_eq!(tracker.observe(4), SequenceOutcome::Accepted);
        assert_eq!(tracker.observe(7), SequenceOutcome::Gap { missed: 2 });
        assert_eq!(tracker.observe(7), SequenceOutcome::DuplicateOrBackward);
    }
    #[test]
    fn sequence_gap_is_attached_to_the_current_valid_event() {
        let event = SerialEvent {
            sequence: 7,
            input: InputId::EncoderCw,
            gap_missed: None,
        };
        assert_eq!(
            accept_sequence(event.clone(), SequenceOutcome::DuplicateOrBackward),
            None
        );
        assert_eq!(
            accept_sequence(event, SequenceOutcome::Gap { missed: 2 })
                .unwrap()
                .gap_missed,
            Some(2)
        );
    }
    #[test]
    fn bounded_buffer_resynchronizes_after_overlong_line_and_preserves_following_line() {
        let mut buffer = BoundedLineBuffer::default();
        let mut bytes = vec![b'x'; MAX_LINE_BYTES + 1];
        bytes.extend_from_slice(b"\nVKEY_INPUT/1 {\"seq\":2,\"input\":\"ENCODER_PRESS\"}\n");
        let line = buffer.push(&bytes).unwrap();
        assert_eq!(decode_line(&line).unwrap().input, InputId::EncoderPress);
    }

    #[test]
    fn bounded_buffer_returns_every_complete_line_from_one_read_chunk() {
        let mut buffer = BoundedLineBuffer::default();
        let first = buffer
            .push(b"VKEY_INPUT/1 {\"seq\":1,\"input\":\"ENCODER_CW\"}\nVKEY_INPUT/1 {\"seq\":2,\"input\":\"ENCODER_CCW\"}\n")
            .unwrap();
        let second = buffer.push(&[]).unwrap();
        assert_eq!(decode_line(&first).unwrap().sequence, 1);
        assert_eq!(decode_line(&second).unwrap().sequence, 2);
        assert!(buffer.push(&[]).is_none());
    }
}
