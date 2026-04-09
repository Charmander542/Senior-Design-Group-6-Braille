/*
 * UNO R4 WiFi — Braille dot hardware test + Python receiver
 *
 * Same serial protocol as arduino_receiver.ino so braille_to_arduino.py works:
 *   "DOTS:1,2,3\n"  — raise dots 1–6 (comma-separated)
 *   "DOTS:NONE\n"   — clear all
 * Baud: 115200
 *
 * Extra commands (newline-terminated):
 *   TEST   — sweep each dot, brief all-on, then clear
 *   PING   — reply PONG
 *   HELP   — list commands
 *
 * Wiring (example — change DOT_PINS to match your board):
 *   Dot 1..6 -> digital pins below -> LED/solenoid driver -> GND
 *
 * Braille layout (standard 6-dot cell):
 *   1 • • 4
 *   2 • • 5
 *   3 • • 6
 */

#include <Arduino.h>

#if __has_include(<WiFiS3.h>)
#include <WiFiS3.h>
#define HAS_WIFI_S3 1
#else
#define HAS_WIFI_S3 0
#endif

// --- Config -----------------------------------------------------------------

// Valid UNO R4 output pins (adjust for your wiring).
const int DOT_PINS[6] = {2, 3, 4, 5, 6, 7};

static const int BAUD_RATE = 115200;
static const uint16_t TCP_PORT = 3333;
static const uint32_t WIFI_CONNECT_TIMEOUT_MS = 15000;
static const uint32_t WIFI_RETRY_LOG_MS = 2000;

// Wait before first Serial.print so the monitor does not miss "READY".
static const uint32_t SERIAL_WAIT_MS = 500;

// Run a quick dot sweep once at boot (set 0 after wiring is verified).
#define ENABLE_STARTUP_SWEEP 1

String inputBuffer;
String networkInputBuffer;
static const size_t INPUT_MAX = 128;
bool wifiConnected = false;

// --- Wi-Fi config ------------------------------------------------------------
// Fill these in after flashing.
const char* WIFI_SSID = "BU Guest (unencrypted)";
const char* WIFI_PASS = "";

#if HAS_WIFI_S3
WiFiServer tcpServer(TCP_PORT);
WiFiClient tcpClient;
#endif

// --- Helpers ----------------------------------------------------------------

void clearAllDots() {
  for (int i = 0; i < 6; i++) {
    digitalWrite(DOT_PINS[i], LOW);
  }
}

void activateDot(int dotNum) {
  if (dotNum < 1 || dotNum > 6) return;
  digitalWrite(DOT_PINS[dotNum - 1], HIGH);
}

void flashAllDots() {
  for (int r = 0; r < 3; r++) {
    for (int i = 0; i < 6; i++) {
      digitalWrite(DOT_PINS[i], HIGH);
    }
    delay(200);
    clearAllDots();
    delay(200);
  }
}

void sweepDots() {
  clearAllDots();
  for (int d = 1; d <= 6; d++) {
    activateDot(d);
    delay(200);
    clearAllDots();
    delay(80);
  }
  for (int i = 0; i < 6; i++) {
    digitalWrite(DOT_PINS[i], HIGH);
  }
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
    if (n >= 1 && n <= 6) activateDot(n);
  }
}

void sendReply(const String& msg) {
  Serial.println(msg);
#if HAS_WIFI_S3
  if (tcpClient && tcpClient.connected()) {
    tcpClient.println(msg);
  }
#endif
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
    sendReply("CMD: DOTS:1,2,3 | DOTS:NONE | TEST | PING | HELP");
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
  Serial.println("BOOT:Braille dot test starting");

  for (int i = 0; i < 6; i++) {
    pinMode(DOT_PINS[i], OUTPUT);
    digitalWrite(DOT_PINS[i], LOW);
  }

#if HAS_WIFI_S3
  Serial.print("WIFI:Connecting SSID=");
  Serial.println(WIFI_SSID);
  int wifiStatus = WiFi.begin(WIFI_SSID, WIFI_PASS);
  uint32_t connectStart = millis();
  uint32_t lastLog = 0;
  while (wifiStatus != WL_CONNECTED && (millis() - connectStart) < WIFI_CONNECT_TIMEOUT_MS) {
    delay(200);
    wifiStatus = WiFi.status();
    if ((millis() - lastLog) >= WIFI_RETRY_LOG_MS) {
      lastLog = millis();
      Serial.print("WIFI:Still connecting, status=");
      Serial.println(wifiStatus);
    }
  }
  wifiConnected = (wifiStatus == WL_CONNECTED);
  if (wifiConnected) {
    Serial.print("WIFI:Connected IP=");
    Serial.println(WiFi.localIP());
    tcpServer.begin();
    Serial.print("WIFI:TCP_SERVER_PORT=");
    Serial.println(TCP_PORT);
  } else {
    Serial.println("WIFI:Connect timeout, continuing in serial-only mode");
  }
#else
  Serial.println("WIFI:Disabled (WiFiS3 not available on this board)");
#endif

  Serial.println("READY:UNO R4 WiFi Braille Dot Test");
  Serial.flush();

#if ENABLE_STARTUP_SWEEP
  sweepDots();
  Serial.println("ACK:STARTUP_SWEEP");
#endif

  flashAllDots();
}

void loop() {
#if HAS_WIFI_S3
  if (wifiConnected && (!tcpClient || !tcpClient.connected())) {
    tcpClient = tcpServer.available();
    if (tcpClient) {
      tcpClient.println("READY:UNO R4 WiFi Braille Dot Test");
    }
  }
#endif

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

  #if HAS_WIFI_S3
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
  #endif
}
