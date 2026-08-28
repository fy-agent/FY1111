use hidapi::{HidApi, HidDevice};

use crate::serial::{BoundedLineBuffer, EventSource, LinkDecoder, SerialError, SerialEvent};

pub const USB_VID: u16 = 0x303A;
pub const USB_PID: u16 = 0x82D0;
pub const USB_LINK_ID: &str = "usb:ventured";
pub const USB_LINK_BAUD: u32 = 115200;

const HID_REPORT_LEN: usize = 64;
const HID_PAYLOAD_MAX: usize = 63;

pub struct UsbLinkSource {
    _api: HidApi,
    device: HidDevice,
    buffered: BoundedLineBuffer,
    decoder: LinkDecoder,
}

impl UsbLinkSource {
    pub fn present() -> bool {
        HidApi::new()
            .ok()
            .map(|api| {
                api.device_list()
                    .any(|device| device.vendor_id() == USB_VID && device.product_id() == USB_PID)
            })
            .unwrap_or(false)
    }

    pub fn open() -> Result<Self, SerialError> {
        let api = HidApi::new().map_err(|_| SerialError::Unavailable)?;
        let device = api
            .open(USB_VID, USB_PID)
            .map_err(|_| SerialError::Unavailable)?;
        let _ = device.set_blocking_mode(false);
        Ok(Self {
            _api: api,
            device,
            buffered: BoundedLineBuffer::default(),
            decoder: LinkDecoder::default(),
        })
    }
}

impl EventSource for UsbLinkSource {
    fn poll_event(&mut self) -> Result<Option<SerialEvent>, SerialError> {
        loop {
            if let Some(line) = self.buffered.push(&[]) {
                if let Some(event) = self.decoder.accept_decoded(&line) {
                    return Ok(Some(event));
                }
                continue;
            }
            let mut report = [0_u8; HID_REPORT_LEN + 1];
            match self.device.read_timeout(&mut report, 10) {
                Ok(0) => return Ok(None),
                Ok(count) => {
                    let mut payload = [0_u8; HID_PAYLOAD_MAX];
                    let n = hid_unpack(&report[..count], &mut payload);
                    if n == 0 {
                        continue;
                    }
                    let Some(line) = self.buffered.push(&payload[..n]) else {
                        continue;
                    };
                    if let Some(event) = self.decoder.accept_decoded(&line) {
                        return Ok(Some(event));
                    }
                }
                Err(_) => return Err(SerialError::Read),
            }
        }
    }

    fn write_line(&mut self, line: &str) -> Result<(), SerialError> {
        let mut bytes = line.as_bytes().to_vec();
        if !line.ends_with('\n') {
            bytes.push(b'\n');
        }
        let mut rest = bytes.as_slice();
        while !rest.is_empty() {
            let mut report = [0_u8; HID_REPORT_LEN];
            let n = hid_pack(&mut report, rest);
            if n == 0 {
                return Err(SerialError::Write);
            }
            write_hid_report(&self.device, &report)?;
            rest = &rest[n..];
        }
        Ok(())
    }

    fn last_network_status(&self) -> Option<crate::network::NetworkStatus> {
        self.decoder.last_network_status()
    }

    fn close(&mut self) {
        self.buffered.clear();
    }
}

fn hid_pack(report: &mut [u8; HID_REPORT_LEN], src: &[u8]) -> usize {
    if src.is_empty() {
        return 0;
    }
    let chunk = src.len().min(HID_PAYLOAD_MAX);
    report.fill(0);
    report[0] = chunk as u8;
    report[1..1 + chunk].copy_from_slice(&src[..chunk]);
    chunk
}

fn hid_unpack(report: &[u8], dst: &mut [u8]) -> usize {
    if report.is_empty() || dst.is_empty() {
        return 0;
    }
    let mut offset = 0;
    if report.len() >= 2 && report[0] == 0 {
        offset = 1;
    }
    if offset >= report.len() {
        return 0;
    }
    let payload = report[offset] as usize;
    if payload == 0 || payload > HID_PAYLOAD_MAX {
        return 0;
    }
    if offset + 1 + payload > report.len() {
        return 0;
    }
    let n = payload.min(dst.len());
    dst[..n].copy_from_slice(&report[offset + 1..offset + 1 + n]);
    n
}

fn write_hid_report(device: &HidDevice, report: &[u8; HID_REPORT_LEN]) -> Result<(), SerialError> {
    let mut framed = [0_u8; HID_REPORT_LEN + 1];
    framed[1..].copy_from_slice(report);
    if device.write(&framed).is_ok() {
        return Ok(());
    }
    device
        .write(report)
        .map(|_| ())
        .map_err(|_| SerialError::Write)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn hid_frame_matches_teensy_style_length_prefix() {
        let src = b"VKEY_INPUT/1 {\"seq\":1,\"input\":\"ENCODER_CW\"}\n";
        let mut report = [0_u8; HID_REPORT_LEN];
        let n = hid_pack(&mut report, src);
        assert_eq!(n, src.len());
        assert_eq!(report[0] as usize, src.len());
        let mut out = [0_u8; HID_PAYLOAD_MAX];
        assert_eq!(hid_unpack(&report, &mut out), src.len());
        assert_eq!(&out[..src.len()], src);

        let mut prefixed = [0_u8; HID_REPORT_LEN + 1];
        prefixed[1..].copy_from_slice(&report);
        assert_eq!(hid_unpack(&prefixed, &mut out), src.len());
        assert_eq!(&out[..src.len()], src);
        assert_eq!(hid_unpack(&[0, 3, b'a'], &mut out), 0);
    }
}
