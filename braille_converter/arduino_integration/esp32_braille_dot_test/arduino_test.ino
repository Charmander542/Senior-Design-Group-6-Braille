// braille_sr.ino
// N-tile 6-bit braille display via chained SN74HC595 shift registers.
//
// ── Configuration (only things you should need to change) ───────────────────
//
#define TILE_COUNT       4    // ← set to however many 6-dot tiles you have
#define WINDOW_TIMEOUT_MS 800 // ms of serial silence before partial window flushes
//
// Chip count is derived automatically: ceil(TILE_COUNT * 6 / 8) chips needed.
// e.g.  4 tiles →  3 chips (24 bits)

// ── Manual / debug protocol ──────────────────────────────────────────────────
//   "P:<t>:<XX>\n"        — set tile t (decimal) directly, bypass window.
//   "PA:<XX...XX>\n"      — set all TILE_COUNT tiles at once (TILE_COUNT*2
//                           hex chars), bypass window.

#include <Arduino.h>
#include <WiFi.h>

// ── WiFi / TCP configuration ─────────────────────────────────────────────────
// Set these to your WiFi network. DHCP is the safest default on guest WiFi:
// use the IP printed in Serial Monitor after the ESP32 connects.
const char* WIFI_SSID = "BU Guest (unencrypted)";
const char* WIFI_PASS = "";

static const uint16_t TCP_PORT = 3333;
static const bool USE_STATIC_IP = false;
static const char* FIRMWARE_BUILD = "arduino_test DHCP 2026-04-30";

// Only enable USE_STATIC_IP when you know these values match the current network.
IPAddress LOCAL_IP(10, 193, 237, 143);
IPAddress GATEWAY(10, 193, 237, 1);
IPAddress SUBNET(255, 255, 255, 0);
IPAddress DNS_PRIMARY(8, 8, 8, 8);
IPAddress DNS_SECONDARY(1, 1, 1, 1);

static const uint32_t WIFI_CONNECT_TIMEOUT_MS = 15000;
static const uint32_t WIFI_RETRY_LOG_MS = 2000;

WiFiServer tcpServer(TCP_PORT);
WiFiClient tcpClient;
bool wifiConnected = false;

// ── Derived constants (do not edit) ─────────────────────────────────────────
#define BITS_PER_TILE    6
#define TOTAL_BITS       (TILE_COUNT * BITS_PER_TILE)
// Holds manual commands, PA commands, and web commands like "DOTS:1,2,3,4,5,6".
#define INPUT_BUF_SIZE   64

// ── Pin assignments ──────────────────────────────────────────────────────────
const uint8_t SER_DATA_PIN = 11;
const uint8_t SR_CLOCK_PIN = 12;
const uint8_t R_CLOCK_PIN  = 13;
const uint8_t SR_CLEAR_PIN = 8;   // active LOW — hold HIGH during normal use
const uint8_t OE_PIN       = 9;   // active LOW — pull LOW to enable outputs

// ── Display state ────────────────────────────────────────────────────────────
uint8_t tilePatterns[TILE_COUNT] = {};   // zero-initialised

// ── Window buffer ────────────────────────────────────────────────────────────
uint8_t  windowBuf[TILE_COUNT]   = {};
uint8_t  windowCount             = 0;
uint32_t windowLastMs            = 0;

Print* commandOut = &Serial;

// ── Shift-register helpers ───────────────────────────────────────────────────

inline void latchOutput() {
  digitalWrite(R_CLOCK_PIN, HIGH);
  delayMicroseconds(1);
  digitalWrite(R_CLOCK_PIN, LOW);
}

inline void shiftBit(bool bitVal) {
  digitalWrite(SER_DATA_PIN, bitVal ? HIGH : LOW);
  digitalWrite(SR_CLOCK_PIN, HIGH);
  delayMicroseconds(1);
  digitalWrite(SR_CLOCK_PIN, LOW);
}

// Push TILE_COUNT * BITS_PER_TILE bits into the chain, MSB of the last tile
// first so tile 0 / dot 1 lands on the first output of the first chip.
// Any remainder bits to fill the final chip's unused outputs are shifted as 0.
void flushDisplay() {
  for (int tile = TILE_COUNT - 1; tile >= 0; tile--) {
    for (int bit = BITS_PER_TILE - 1; bit >= 0; bit--) {
      shiftBit((tilePatterns[tile] >> bit) & 0x01);
    }
  }
  // Pad leftover bits in the last chip (0 if TOTAL_BITS % 8 == 0)
  uint8_t remainder = (8 - (TOTAL_BITS % 8)) % 8;
  for (uint8_t i = 0; i < remainder; i++) {
    shiftBit(false);
  }
  latchOutput();
}

// ── Display API ──────────────────────────────────────────────────────────────

inline uint8_t truncate6(uint8_t pattern) {
  return pattern & 0x3F;  // drop dots 7 & 8 (bits 6-7)
}

void setTilePattern(uint8_t tile, uint8_t pattern) {
  if (tile >= TILE_COUNT) return;
  tilePatterns[tile] = truncate6(pattern);
  flushDisplay();
}

void setAllTiles(const uint8_t* patterns) {
  for (uint8_t i = 0; i < TILE_COUNT; i++) {
    tilePatterns[i] = truncate6(patterns[i]);
  }
  flushDisplay();
}

void clearAll() {
  memset(tilePatterns, 0, sizeof(tilePatterns));
  flushDisplay();
}

// ── Window buffer logic ──────────────────────────────────────────────────────

void flushWindow() {
  for (uint8_t i = windowCount; i < TILE_COUNT; i++) {
    windowBuf[i] = 0x00;   // pad unfilled slots
  }
  setAllTiles(windowBuf);
  memset(windowBuf, 0, sizeof(windowBuf));
  windowCount  = 0;
  windowLastMs = 0;
}

// Called from loop() after a timeout, or from windowAccept() when full.
// Sends one "OK" per character that was buffered (host blocks on each one).
void flushWindowWithOK(uint8_t count) {
  flushWindow();
  for (uint8_t i = 0; i < count; i++) {
    commandOut->println("OK");
  }
}

void windowAccept(uint8_t pattern) {
  windowBuf[windowCount++] = truncate6(pattern);
  windowLastMs = millis();
  if (windowCount >= TILE_COUNT) {
    flushWindowWithOK(TILE_COUNT);
  }
}

// ── Visualisation ────────────────────────────────────────────────────────────

void printAllVisualizations() {
  commandOut->print("Tiles: ");
  for (uint8_t t = 0; t < TILE_COUNT; t++) {
    commandOut->print("[");
    uint8_t p = tilePatterns[t];
    for (int row = 0; row < 3; row++) {
      commandOut->print((p >> row)       & 1 ? "o" : ".");   // dot1/2/3
      commandOut->print((p >> (row + 3)) & 1 ? "o" : ".");   // dot4/5/6
      if (row < 2) commandOut->print("|");
    }
    commandOut->print("]");
    if (t < TILE_COUNT - 1) commandOut->print(" ");
  }
  commandOut->println();
}

// ── Serial command parser ────────────────────────────────────────────────────

char inputBuffer[INPUT_BUF_SIZE];
int  bufferIndex = 0;
char networkInputBuffer[INPUT_BUF_SIZE];
int  networkBufferIndex = 0;

void printLine(const char* msg) {
  commandOut->println(msg);
}

void setDotsFromCsv(const char* dotsStr) {
  uint8_t pattern = 0;
  const char* p = dotsStr;

  while (*p != '\0') {
    while (*p == ' ' || *p == ',') p++;
    if (*p == '\0') break;

    char* endPtr = nullptr;
    long dot = strtol(p, &endPtr, 10);
    if (endPtr == p) {
      p++;
      continue;
    }

    if (dot >= 1 && dot <= BITS_PER_TILE) {
      pattern |= (1 << (dot - 1));
    } else if (dot >= 7 && dot <= 8) {
      commandOut->print("WARN:dot ");
      commandOut->print(dot);
      commandOut->println(" ignored on 6-dot hardware");
    }
    p = endPtr;
  }

  uint8_t patterns[TILE_COUNT] = {};
  patterns[0] = pattern;
  setAllTiles(patterns);
  printAllVisualizations();
  printLine("OK");
}

void processCommand(const char* cmd) {

  // ── DOTS:1,2,3 — web interface command, shown on tile 0 ───────────────────
  if (strncmp(cmd, "DOTS:", 5) == 0) {
    const char* dotsStr = cmd + 5;
    if (strcmp(dotsStr, "NONE") == 0 || dotsStr[0] == '\0') {
      clearAll();
      printAllVisualizations();
      printLine("OK");
    } else {
      setDotsFromCsv(dotsStr);
    }

  // ── PING ──────────────────────────────────────────────────────────────────
  } else if (strcmp(cmd, "PING") == 0) {
    printLine("PONG");

  // ── CLEAR ─────────────────────────────────────────────────────────────────
  } else if (strcmp(cmd, "CLEAR") == 0) {
    if (windowCount > 0) flushWindow();
    clearAll();
    printLine("OK");

  // ── P:XX — single pattern from serial_braille.py (window buffered) ─────────
  // Format: exactly "P:" followed by exactly 2 hex chars and nothing else.
  } else if (cmd[0] == 'P' && cmd[1] == ':' &&
             cmd[2] != '\0' && cmd[3] != '\0' && cmd[4] == '\0') {
    uint8_t raw = (uint8_t)strtoul(cmd + 2, NULL, 16);
    uint8_t masked = truncate6(raw);
    if (raw != masked) {
      commandOut->print("WARN:truncated 0x");
      if (raw < 0x10) commandOut->print("0");
      commandOut->print(raw, HEX);
      commandOut->print(" -> 0x");
      if (masked < 0x10) commandOut->print("0");
      commandOut->println(masked, HEX);
    }
    windowAccept(raw);
    // If still buffering (windowAccept didn't flush), send OK now so host
    // doesn't stall waiting for acknowledgement.
    if (windowCount > 0) {
      printLine("OK");
    }

  // ── P:<t>:<XX> — direct single tile, tile index is decimal, any width ──────
  } else if (cmd[0] == 'P' && cmd[1] == ':') {
    // Find the second colon
    const char* secondColon = strchr(cmd + 2, ':');
    if (!secondColon) {
      printLine("ERR:bad P: format, expected P:<tile>:<XX>");
      return;
    }
    uint8_t tile = (uint8_t)strtoul(cmd + 2, NULL, 10);
    if (tile >= TILE_COUNT) {
      commandOut->print("ERR:tile out of range (max ");
      commandOut->print(TILE_COUNT - 1);
      commandOut->println(")");
      return;
    }
    uint8_t pattern = truncate6((uint8_t)strtoul(secondColon + 1, NULL, 16));
    setTilePattern(tile, pattern);
    printAllVisualizations();
    printLine("OK");

  // ── PA:<XX...XX> — direct all-tiles, TILE_COUNT*2 hex chars ────────────────
  } else if (cmd[0] == 'P' && cmd[1] == 'A' && cmd[2] == ':') {
    const char* hex = cmd + 3;
    size_t needed = (size_t)TILE_COUNT * 2;
    if (strlen(hex) < needed) {
      commandOut->print("ERR:PA needs ");
      commandOut->print(needed);
      commandOut->println(" hex chars");
      return;
    }
    uint8_t patterns[TILE_COUNT];
    char buf[3] = {0};
    for (uint8_t t = 0; t < TILE_COUNT; t++) {
      buf[0] = hex[t * 2];
      buf[1] = hex[t * 2 + 1];
      patterns[t] = truncate6((uint8_t)strtoul(buf, NULL, 16));
    }
    setAllTiles(patterns);
    printAllVisualizations();
    printLine("OK");

  // ── TEST — dot-by-dot sweep across every tile ──────────────────────────────
  } else if (strcmp(cmd, "TEST") == 0) {
    clearAll();
    uint8_t p[TILE_COUNT];
    for (uint8_t t = 0; t < TILE_COUNT; t++) {
      for (uint8_t d = 0; d < BITS_PER_TILE; d++) {
        memset(p, 0, sizeof(p));
        p[t] = 1 << d;
        setAllTiles(p);
        delay(120);
      }
    }
    memset(p, 0x3F, sizeof(p));
    setAllTiles(p);
    delay(500);
    clearAll();
    printLine("OK");

  // ── Unknown ───────────────────────────────────────────────────────────────
  } else {
    commandOut->print("ERR:unknown cmd '");
    commandOut->print(cmd);
    commandOut->println("'");
  }
}

void readCommands(Stream& stream, char* buffer, int& index, Print& out) {
  while (stream.available() > 0) {
    char c = stream.read();
    if (c == '\r') continue;
    if (c == '\n') {
      buffer[index] = '\0';
      if (index > 0) {
        commandOut = &out;
        processCommand(buffer);
        commandOut = &Serial;
      }
      index = 0;
    } else if (index < (int)INPUT_BUF_SIZE - 1) {
      buffer[index++] = c;
    }
  }
}

void setupWiFi() {
  Serial.print("WIFI:Connecting SSID=");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  if (USE_STATIC_IP) {
    Serial.print("WIFI:Static IP requested=");
    Serial.println(LOCAL_IP);
    if (!WiFi.config(LOCAL_IP, GATEWAY, SUBNET, DNS_PRIMARY, DNS_SECONDARY)) {
      Serial.println("WIFI:Static IP config failed");
    }
  } else {
    Serial.println("WIFI:DHCP enabled, using router-assigned IP");
  }

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
    Serial.print("WIFI:Gateway=");
    Serial.println(WiFi.gatewayIP());
    tcpServer.begin();
    Serial.print("WIFI:TCP_SERVER_PORT=");
    Serial.println(TCP_PORT);
  } else {
    Serial.println("WIFI:Connect timeout, continuing in serial-only mode");
  }
}

// ── Arduino entry points ─────────────────────────────────────────────────────

void setup() {
  Serial.begin(115200);

  pinMode(SER_DATA_PIN, OUTPUT); digitalWrite(SER_DATA_PIN, LOW);
  pinMode(SR_CLOCK_PIN, OUTPUT); digitalWrite(SR_CLOCK_PIN, LOW);
  pinMode(R_CLOCK_PIN,  OUTPUT); digitalWrite(R_CLOCK_PIN,  LOW);
  pinMode(SR_CLEAR_PIN, OUTPUT); digitalWrite(SR_CLEAR_PIN, HIGH);
  pinMode(OE_PIN,       OUTPUT); digitalWrite(OE_PIN,       LOW);

  clearAll();
  delay(200);
  Serial.print("BRAILLE_SR_READY TILES=");
  Serial.print(TILE_COUNT);
  Serial.print(" BITS=");
  Serial.println(BITS_PER_TILE);
  Serial.print("FIRMWARE_BUILD=");
  Serial.println(FIRMWARE_BUILD);

  setupWiFi();
}

void loop() {
  // ── Timeout flush ─────────────────────────────────────────────────────────
  if (windowCount > 0 && windowLastMs > 0 &&
      (millis() - windowLastMs) >= WINDOW_TIMEOUT_MS) {
    uint8_t flushedCount = windowCount;
    flushWindowWithOK(flushedCount);
    printAllVisualizations();
  }

  if (wifiConnected) {
    if (!tcpClient || !tcpClient.connected()) {
      tcpClient = tcpServer.available();
      if (tcpClient) {
        Serial.println("WIFI:Client connected");
        tcpClient.println("READY:BRAILLE_SR");
      }
    }

    if (tcpClient && tcpClient.connected()) {
      readCommands(tcpClient, networkInputBuffer, networkBufferIndex, tcpClient);
    }
  }

  // ── Serial input ──────────────────────────────────────────────────────────
  readCommands(Serial, inputBuffer, bufferIndex, Serial);
}