/*
 * ESP32 — Braille Text Input (Serial + Wi-Fi TCP)
 *
 * Commands (newline-terminated, accepted on both Serial and TCP):
 *   TEXT:<string>        — convert text → braille, display char by char
 *   DELAY:<ms>           — set display time per character (default 1000 ms)
 *   DOTS:<1,2,...>       — raise specific dots 1–8
 *   DOTS:NONE            — clear all LEDs
 *   TEST                 — sweep all 8 LEDs one by one
 *   PING                 — reply PONG
 *   HELP                 — list all commands
 *
 * Wi-Fi: connects to WIFI_SSID at boot, then starts a TCP server on TCP_PORT.
 * The serial monitor prints "WIFI:Connected IP=<addr>" once online.
 * Enter that IP in the Flask web interface (WiFi TCP mode, port 3333).
 *
 * 8-dot Braille layout:
 *   Dot 1 • • Dot 4
 *   Dot 2 • • Dot 5
 *   Dot 3 • • Dot 6
 *   Dot 7 • • Dot 8   (dot 7 = capital indicator, dot 8 = number indicator)
 *
 * Wiring (DOT_PINS — adjust for your board):
 *   DOT_PINS[0] → dot 1 → LED + resistor → GND
 *   ...
 *   DOT_PINS[7] → dot 8 → LED + resistor → GND
 * Avoid GPIO 6-11 (SPI flash) and 34-39 (input-only) on ESP32-WROOM.
 */

#include <Arduino.h>
#include <WiFi.h>

// ── Wi-Fi credentials — update before flashing ──────────────────────────────
const char* WIFI_SSID = "BU Guest (unencrypted)";
const char* WIFI_PASS = "";

// ── Network config ───────────────────────────────────────────────────────────
static const uint16_t TCP_PORT                = 3333;
static const uint32_t WIFI_CONNECT_TIMEOUT_MS = 15000;
static const uint32_t WIFI_RETRY_LOG_MS       = 2000;

// ── Pin configuration ────────────────────────────────────────────────────────
//   Dots:   1    2    3    4    5    6    7    8
const int DOT_PINS[8] = {18, 19, 21, 22, 23, 25, 26, 27};

static const uint32_t BAUD_RATE      = 115200;
static const uint32_t SERIAL_WAIT_MS = 500;
static uint32_t       charDelayMs    = 1000;

// ── TCP server / client ──────────────────────────────────────────────────────
WiFiServer tcpServer(TCP_PORT);
WiFiClient tcpClient;
bool       wifiConnected = false;

// ── Input buffers ─────────────────────────────────────────────────────────────
String     serialBuf;
String     tcpBuf;
static const size_t INPUT_MAX = 256;

// ── Braille lookup table ─────────────────────────────────────────────────────
// Bit positions: bit0=dot1  bit1=dot2 ... bit7=dot8
// Dot 7 (0x40) = capital indicator, Dot 8 (0x80) = number indicator.
// Unknown characters fall through to 0xFF (all dots on).

struct BrailleEntry { char ch; uint8_t dots; };

static const BrailleEntry BRAILLE_MAP[] = {
  // Lowercase
  {'a',0x01},{'b',0x03},{'c',0x09},{'d',0x19},{'e',0x11},
  {'f',0x0B},{'g',0x1B},{'h',0x13},{'i',0x0A},{'j',0x1A},
  {'k',0x05},{'l',0x07},{'m',0x0D},{'n',0x1D},{'o',0x15},
  {'p',0x0F},{'q',0x1F},{'r',0x17},{'s',0x0E},{'t',0x1E},
  {'u',0x25},{'v',0x27},{'w',0x3A},{'x',0x2D},{'y',0x3D},{'z',0x35},
  // Uppercase (same braille + dot 7)
  {'A',0x01|0x40},{'B',0x03|0x40},{'C',0x09|0x40},{'D',0x19|0x40},{'E',0x11|0x40},
  {'F',0x0B|0x40},{'G',0x1B|0x40},{'H',0x13|0x40},{'I',0x0A|0x40},{'J',0x1A|0x40},
  {'K',0x05|0x40},{'L',0x07|0x40},{'M',0x0D|0x40},{'N',0x1D|0x40},{'O',0x15|0x40},
  {'P',0x0F|0x40},{'Q',0x1F|0x40},{'R',0x17|0x40},{'S',0x0E|0x40},{'T',0x1E|0x40},
  {'U',0x25|0x40},{'V',0x27|0x40},{'W',0x3A|0x40},{'X',0x2D|0x40},{'Y',0x3D|0x40},
  {'Z',0x35|0x40},
  // Digits (same braille as a-j + dot 8)
  {'1',0x01|0x80},{'2',0x03|0x80},{'3',0x09|0x80},{'4',0x19|0x80},{'5',0x11|0x80},
  {'6',0x0B|0x80},{'7',0x1B|0x80},{'8',0x13|0x80},{'9',0x0A|0x80},{'0',0x1A|0x80},
  // Punctuation & space
  {' ',0x00},
  {',',0x02},{';',0x06},{':',0x12},{'.',0x32},{'?',0x26},
  {'!',0x16},{'-',0x24},{'\'',0x04},{'"',0x36},{'(',0x13},{')',0x2C},
};

static const int BRAILLE_MAP_LEN =
    (int)(sizeof(BRAILLE_MAP) / sizeof(BRAILLE_MAP[0]));

// ── LED helpers ──────────────────────────────────────────────────────────────

void clearAll() {
  for (int i = 0; i < 8; i++) digitalWrite(DOT_PINS[i], LOW);
}

void showPattern(uint8_t dots) {
  for (int i = 0; i < 8; i++)
    digitalWrite(DOT_PINS[i], (dots >> i) & 1 ? HIGH : LOW);
}

uint8_t lookupChar(char c) {
  for (int i = 0; i < BRAILLE_MAP_LEN; i++)
    if (BRAILLE_MAP[i].ch == c) return BRAILLE_MAP[i].dots;
  return 0xFF;
}

void sweepDots() {
  clearAll();
  for (int d = 0; d < 8; d++) {
    digitalWrite(DOT_PINS[d], HIGH);
    delay(150);
    digitalWrite(DOT_PINS[d], LOW);
    delay(60);
  }
  for (int i = 0; i < 8; i++) digitalWrite(DOT_PINS[i], HIGH);
  delay(400);
  clearAll();
}

// ── Reply helper — sends to Serial and active TCP client ─────────────────────

void sendReply(const String& msg) {
  Serial.println(msg);
  if (tcpClient && tcpClient.connected()) {
    tcpClient.println(msg);
  }
}

// ── TEXT command ─────────────────────────────────────────────────────────────

void displayText(const String& text) {
  sendReply("DISPLAYING:" + text);

  for (int idx = 0; idx < (int)text.length(); idx++) {
    char    c    = text[idx];
    uint8_t dots = lookupChar(c);
    showPattern(dots);

    String dotBin = "";
    for (int b = 7; b >= 0; b--) dotBin += String((dots >> b) & 1);
    sendReply(String("CHAR:") + c + "=DOTS_BIN:" + dotBin);

    delay(charDelayMs);
  }

  clearAll();
  sendReply("ACK:TEXT_DONE");
}

// ── DOTS command ─────────────────────────────────────────────────────────────

void activateDotsFromString(const String& dotsStr) {
  uint8_t pattern = 0;
  int startIdx = 0;

  while (startIdx < (int)dotsStr.length()) {
    int comma = dotsStr.indexOf(',', startIdx);
    String part = (comma < 0) ? dotsStr.substring(startIdx)
                               : dotsStr.substring(startIdx, comma);
    part.trim();
    if (part.length() > 0) {
      int n = part.toInt();
      if (n >= 1 && n <= 8) pattern |= (uint8_t)(1 << (n - 1));
    }
    if (comma < 0) break;
    startIdx = comma + 1;
  }

  showPattern(pattern);
}

// ── Command dispatcher ───────────────────────────────────────────────────────

void processLine(const String& lineIn) {
  String msg = lineIn;
  msg.trim();
  if (msg.length() == 0) return;

  if (msg.equalsIgnoreCase("PING")) {
    sendReply("PONG");

  } else if (msg.equalsIgnoreCase("TEST")) {
    sweepDots();
    sendReply("ACK:TEST");

  } else if (msg.equalsIgnoreCase("HELP")) {
    sendReply("CMDS: TEXT:<str> | DELAY:<ms> | DOTS:<1..8,...> | DOTS:NONE | TEST | PING | HELP");

  } else if (msg.startsWith("DELAY:")) {
    int ms = msg.substring(6).toInt();
    if (ms > 0) {
      charDelayMs = (uint32_t)ms;
      sendReply("ACK:DELAY:" + String(charDelayMs));
    } else {
      sendReply("ERROR:Invalid delay value");
    }

  } else if (msg.startsWith("TEXT:")) {
    displayText(msg.substring(5));

  } else if (msg.startsWith("DOTS:")) {
    String dotsStr = msg.substring(5);
    dotsStr.trim();
    if (dotsStr.equalsIgnoreCase("NONE") || dotsStr.length() == 0) {
      clearAll();
      sendReply("ACK:CLEARED");
    } else {
      activateDotsFromString(dotsStr);
      sendReply("ACK:DOTS:" + dotsStr);
    }

  } else {
    sendReply("ERROR:Unknown command — send HELP for list");
  }
}

// ── Arduino entry points ─────────────────────────────────────────────────────

void setup() {
  Serial.begin(BAUD_RATE);
  delay(SERIAL_WAIT_MS);
  Serial.println("BOOT:ESP32 Braille starting");

  for (int i = 0; i < 8; i++) {
    pinMode(DOT_PINS[i], OUTPUT);
    digitalWrite(DOT_PINS[i], LOW);
  }

  // Connect to Wi-Fi
  Serial.print("WIFI:Connecting SSID=");
  Serial.println(WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  uint32_t connectStart = millis();
  uint32_t lastLog      = 0;
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
    Serial.print("WIFI:TCP server on port ");
    Serial.println(TCP_PORT);
  } else {
    Serial.println("WIFI:Timeout — continuing in serial-only mode");
  }

  Serial.println("READY:ESP32 Braille 8-LED Text Input");
  Serial.flush();

  sweepDots();
  Serial.println("ACK:STARTUP_SWEEP_DONE");
}

void loop() {
  // Accept a new TCP client whenever none is connected
  if (wifiConnected && (!tcpClient || !tcpClient.connected())) {
    tcpClient = tcpServer.available();
    if (tcpClient) {
      Serial.println("WIFI:Client connected");
      tcpClient.println("READY:ESP32 Braille 8-LED Text Input");
    }
  }

  // Read from Serial
  while (Serial.available() > 0) {
    char c = static_cast<char>(Serial.read());
    if (c == '\n') {
      processLine(serialBuf);
      serialBuf = "";
    } else if (c != '\r') {
      if (serialBuf.length() < INPUT_MAX) serialBuf += c;
    }
  }

  // Read from TCP client
  if (wifiConnected && tcpClient && tcpClient.connected()) {
    while (tcpClient.available() > 0) {
      char c = static_cast<char>(tcpClient.read());
      if (c == '\n') {
        processLine(tcpBuf);
        tcpBuf = "";
      } else if (c != '\r') {
        if (tcpBuf.length() < INPUT_MAX) tcpBuf += c;
      }
    }
  }
}
