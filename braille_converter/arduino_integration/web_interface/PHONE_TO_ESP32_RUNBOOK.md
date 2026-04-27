# Phone to ESP32 Demo Runbook

This runbook is for the local-network flow:

Phone browser -> Flask web UI -> TCP (`DOTS:*`) -> ESP32 braille display

## 1) Startup Checklist

1. Flash `esp32_braille_text_input.ino` to ESP32.
2. Power ESP32 and confirm serial output shows:
   - `WIFI:Connected IP=<ip>`
   - `WIFI:TCP server on port 3333`
3. On the Flask host:
   - `cd braille_converter/arduino_integration/web_interface`
   - Optional access gate: `export ACCESS_CODE=braille123`
   - Start server: `python app.py`
4. Confirm Flask binds to `0.0.0.0` and note the local URL:
   - `http://<host-local-ip>:<port>`
5. Open the URL from at least one phone on the same Wi-Fi.
6. In UI, select `WiFi TCP`, enter ESP32 IP, keep port `3333`, then click `Connect`.

## 2) Multi-Phone Test Matrix (Completed)

Test date: 2026-04-26

- Device A (iPhone Safari): Sent 10-char message, displayed correctly.
- Device B (Android Chrome): Sent 45-char message, displayed correctly.
- Device C (laptop Chrome): Sent 3 queued messages while A/B active, all displayed in order.
- Disconnect scenario: ESP32 power-cycled mid-send, UI reported disconnect, reconnect succeeded.
- Recovery time check: end user able to restore demo flow in under 2 minutes.

## 3) Queue Behavior (Completed)

- Multiple users can submit concurrently through `/api/send`.
- Backend enqueues messages and processes them one-by-one in FIFO order.
- UI shows:
  - current message being displayed,
  - pending queue list with order and message lengths.

## 4) Demo Checklist (Completed)

- [x] Phone opens page without developer tools.
- [x] Connect/disconnect works in WiFi TCP mode.
- [x] `DOTS` stream reaches ESP32 and LEDs update per character.
- [x] Queue list updates when multiple people submit messages.
- [x] `Stop` stops current message and allows queue progression.
- [x] Optional passcode gate works when `ACCESS_CODE` is set.

## 5) Troubleshooting Guide (Completed)

### Can't connect to page from phone
- Confirm phone and Flask host are on same Wi-Fi.
- Use host LAN IP, not `localhost`.
- Verify firewall allows Flask port.

### ESP32 offline / connect fails
- Recheck ESP32 IP from serial monitor.
- Confirm ESP32 joined Wi-Fi successfully.
- Verify TCP port is `3333`.

### Text not displaying
- Check UI status is connected before sending.
- Confirm message is visible in queue list.
- Use `LED Sweep` test to verify hardware output path.
- Reconnect and resend if previous connection dropped.

### Queue appears stuck
- Confirm current message is still progressing (char updates in UI).
- Use `Stop` to skip current message.
- Disconnect/reconnect to clear stale state if needed.

## 6) Operator Recovery Procedure (<2 minutes)

1. Press `Disconnect`.
2. Power cycle ESP32.
3. Re-enter current ESP32 IP and click `Connect`.
4. Send short test text (`abc`) and verify dots.
5. Resume normal queued sends.
