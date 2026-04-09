/*
 * ESP32-S3 — 8-dot Braille cell hardware test (Serial + Wi-Fi TCP)
 *
 * Protocol (newline-terminated, works on both Serial and TCP):
 *   "DOTS:1,2,3,7\n"  — raise specified dots 1–8
 *   "DOTS:NONE\n"      — clear all
 *   TEST               — sweep each dot, brief all-on, then clear
 *   PING               — reply PONG
 *   HELP               — list commands
 * Baud: 115200  |  TCP port: 3333
 *
 * Wiring — DOT_PINS must match LED_test_esp32.cpp (same dot order / GPIOs).
 *   Dot 1..8 -> GPIO pins below -> LED/solenoid driver -> GND
 *
 * 8-dot Braille layout:
 *   1 • • 4
 *   2 • • 5
 *   3 • • 6
 *   7 • • 8
 *
 * Wi-Fi: connects to WIFI_SSID at boot. Once connected, the serial
 * monitor prints the IP address. Connect via TCP on port 3333 from
 * your laptop to send the same commands wirelessly.
 */

#include <Arduino.h>
#include <WiFi.h>

// --- Config -----------------------------------------------------------------

static const int NUM_DOTS = 8;
// Dot 1..8 — must match LED_test_esp32.cpp.
// Avoid GPIO 9 & 12 for dots 5–6: on ESP32-S3, GPIO9 is often default I2C SCL and GPIO12 is
// default HSPI CLK; the Arduino core or libraries may attach those peripherals so
// digitalWrite() does not drive your LEDs reliably. Use 16/17 (or other free GPIOs) instead.
const int DOT_PINS[NUM_DOTS] = {5, 6, 7, 15, 16, 17, 11, 10};

static const int BAUD_RATE = 115200;
static const uint16_t TCP_PORT = 3333;
static const uint32_t WIFI_CONNECT_TIMEOUT_MS = 15000;
static const uint32_t WIFI_RETRY_LOG_MS = 2000;
static const uint32_t SERIAL_WAIT_MS = 500;

#define ENABLE_STARTUP_SWEEP 1

String inputBuffer;
String networkInputBuffer;
static const size_t INPUT_MAX = 128;
bool wifiConnected = false;

const char* WIFI_SSID = "BU Guest (unencrypted)";
const char* WIFI_PASS = "";

WiFiServer tcpServer(TCP_PORT);
WiFiClient tcpClient;

// --- Helpers ----------------------------------------------------------------

void clearAllDots() {
  for (int i = 0; i < NUM_DOTS; i++) {
    digitalWrite(DOT_PINS[i], LOW);
  }
}

void setAllDots(int state) {
  for (int i = 0; i < NUM_DOTS; i++) {
    digitalWrite(DOT_PINS[i], state);
  }
}

void activateDot(int dotNum) {
  if (dotNum < 1 || dotNum > NUM_DOTS) return;
  digitalWrite(DOT_PINS[dotNum - 1], HIGH);
}

void flashAllDots() {
  for (int r = 0; r < 3; r++) {
    setAllDots(HIGH);
    delay(200);
    clearAllDots();
    delay(200);
  }
}

void sweepDots() {
  clearAllDots();
  for (int d = 1; d <= NUM_DOTS; d++) {
    activateDot(d);
    delay(200);
    clearAllDots();
    delay(80);
  }
  setAllDots(HIGH);
  delay(400);
  clearAllDots();
}

void activateDotsFromString(const String& dotsStr) {
  clearAllDots();
  int startIdx = 0;
  while (startIdx < (int)dotsStr.length()) {
    int commaIdx = dotsStr.indexOf(',', startIdx);
    String part;
    if (commaIdx < 0) {
      part = dotsStr.substring(startIdx);
      startIdx = dotsStr.length();
    } else {
      part = dotsStr.substring(startIdx, commaIdx);
      startIdx = commaIdx + 1;
    }
    part.trim();
    if (part.length() == 0) continue;
    int n = part.toInt();
    if (n >= 1 && n <= NUM_DOTS) activateDot(n);
  }
}

void sendReply(const String& msg) {
  Serial.println(msg);
  if (tcpClient && tcpClient.connected()) {
    tcpClient.println(msg);
  }
}

void processLine(const String& lineIn) {
  String message = lineIn;
  message.trim();
  if (message.length() == 0) return;

  if (message.equalsIgnoreCase("PING")) {
    sendReply("PONG");
    return;
  }
  if (message.equalsIgnoreCase("HELP")) {
    sendReply("CMD: DOTS:1,2,3,7 | DOTS:NONE | TEST | PING | HELP");
    return;
  }
  if (message.equalsIgnoreCase("TEST")) {
    sweepDots();
    sendReply("ACK:TEST");
    return;
  }

  if (message.startsWith("DOTS:")) {
    String dotsStr = message.substring(5);
    dotsStr.trim();
    if (dotsStr.length() == 0 || dotsStr.equalsIgnoreCase("NONE")) {
      clearAllDots();
      sendReply("ACK:CLEARED");
    } else {
      activateDotsFromString(dotsStr);
      sendReply("ACK:DOTS:" + dotsStr);
    }
    return;
  }

  sendReply("ERROR:Unknown command (send HELP)");
}

// --- Arduino ----------------------------------------------------------------

void setup() {
  Serial.begin(BAUD_RATE);
  delay(SERIAL_WAIT_MS);
  Serial.println("BOOT:8-dot Braille cell test starting");

  for (int i = 0; i < NUM_DOTS; i++) {
    pinMode(DOT_PINS[i], OUTPUT);
    digitalWrite(DOT_PINS[i], LOW);
  }

  // --- ESP32 Wi-Fi ---
  Serial.print("WIFI:Connecting SSID=");
  Serial.println(WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  uint32_t connectStart = millis();
  uint32_t lastLog = 0;
  while (WiFi.status() != WL_CONNECTED &&
         (millis() - connectStart) < WIFI_CONNECT_TIMEOUT_MS) {
    delay(200);
    if ((millis() - lastLog) >= WIFI_RETRY_LOG_MS) {
      lastLog = millis();
      Serial.print("WIFI:Still connecting, status=");
      Serial.println(WiFi.status());
    }
  }

  wifiConnected = (WiFi.status() == WL_CONNECTED);
  if (wifiConnected) {
    Serial.print("WIFI:Connected IP=");
    Serial.println(WiFi.localIP());
    tcpServer.begin();
    Serial.print("WIFI:TCP_SERVER_PORT=");
    Serial.println(TCP_PORT);
  } else {
    Serial.println("WIFI:Connect timeout, continuing in serial-only mode");
  }

  Serial.println("READY:ESP32-S3 8-Dot Braille Cell Test");
  Serial.flush();

#if ENABLE_STARTUP_SWEEP
  sweepDots();
  Serial.println("ACK:STARTUP_SWEEP");
#endif

  flashAllDots();
}

void loop() {
  if (wifiConnected && (!tcpClient || !tcpClient.connected())) {
    tcpClient = tcpServer.available();
    if (tcpClient) {
      Serial.println("WIFI:Client connected");
      tcpClient.println("READY:ESP32-S3 8-Dot Braille Cell Test");
    }
  }

  while (Serial.available() > 0) {
    char c = static_cast<char>(Serial.read());
    if (c == '\n') {
      processLine(inputBuffer);
      inputBuffer = "";
    } else if (c != '\r') {
      if (inputBuffer.length() < INPUT_MAX) {
        inputBuffer += c;
      }
    }
  }

  if (wifiConnected && tcpClient && tcpClient.connected()) {
    while (tcpClient.available() > 0) {
      char c = static_cast<char>(tcpClient.read());
      if (c == '\n') {
        processLine(networkInputBuffer);
        networkInputBuffer = "";
      } else if (c != '\r') {
        if (networkInputBuffer.length() < INPUT_MAX) {
          networkInputBuffer += c;
        }
      }
    }
  }
}
