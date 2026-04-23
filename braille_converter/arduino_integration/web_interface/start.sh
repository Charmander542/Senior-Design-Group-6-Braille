#!/usr/bin/env bash
# Start the Braille ESP32 web interface and print the local URL phones should open.
set -e
cd "$(dirname "$0")"

# Detect local IP (macOS en0/en1, then Linux fallback)
LOCAL_IP=$(ipconfig getifaddr en0 2>/dev/null \
  || ipconfig getifaddr en1 2>/dev/null \
  || hostname -I 2>/dev/null | awk '{print $1}' \
  || echo "127.0.0.1")

PORT=${PORT:-5000}

echo "============================================================"
echo "  Braille ESP32 Web Interface"
echo "  Phones on the same Wi-Fi — open:"
echo "    http://${LOCAL_IP}:${PORT}"
echo ""
echo "  ESP32 setup:"
echo "    1. Flash esp32_braille_text_input.ino"
echo "    2. Open Serial Monitor (115200 baud)"
echo "    3. Note the IP after 'WIFI:Connected IP='"
echo "    4. Enter that IP in the web UI (WiFi TCP mode, port 3333)"
echo ""
echo "  Optional passcode: ACCESS_CODE=yourcode ./start.sh"
echo "============================================================"

python3 app.py --port "$PORT" "$@"
