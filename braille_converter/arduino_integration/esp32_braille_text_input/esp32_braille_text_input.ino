/*
 * ESP32 — Braille Text Input (8-LED test)
 *
 * Serial protocol (115200 baud, newline-terminated):
 *   TEXT:<string>     — convert text to braille and display char by char
 *   DELAY:<ms>        — set display time per character (default 1000 ms)
 *   DOTS:<1,2,...,8>  — manually raise specific dots 1-8
 *   DOTS:NONE         — clear all LEDs
 *   TEST              — sweep all 8 LEDs one by one
 *   PING              — reply PONG
 *   HELP              — list commands
 *
 * 8-LED Braille layout:
 *   Dot 1 • • Dot 4
 *   Dot 2 • • Dot 5
 *   Dot 3 • • Dot 6
 *   Dot 7 • • Dot 8   ← Dot 7 = capital indicator, Dot 8 = number indicator
 *
 * Wiring (adjust DOT_PINS for your board):
 *   DOT_PINS[0] → dot 1 → LED + resistor → GND
 *   DOT_PINS[1] → dot 2 → LED + resistor → GND
 *   ...
 *   DOT_PINS[7] → dot 8 → LED + resistor → GND
 *
 * Avoid GPIO 6-11 (SPI flash) and 34-39 (input-only) on ESP32-WROOM.
 */

#include <Arduino.h>

// ── Pin configuration ────────────────────────────────────────────────────────
//   Dots:   1    2    3    4    5    6    7    8
const int DOT_PINS[8] = {18, 19, 21, 22, 23, 25, 26, 27};

static const uint32_t BAUD_RATE      = 115200;
static const uint32_t SERIAL_WAIT_MS = 500;   // time for USB CDC to settle
static uint32_t       charDelayMs    = 1000;  // ms to show each character

// ── Braille lookup table ─────────────────────────────────────────────────────
// Each entry stores the dot bitmask for one character.
// Bit positions:  bit0=dot1  bit1=dot2  bit2=dot3  bit3=dot4
//                 bit4=dot5  bit5=dot6  bit6=dot7  bit7=dot8
//
// Dots 1-6 follow standard Grade-1 English Braille (6-dot cell).
// Dot 7 (bit6, 0x40) lights up as a CAPITAL indicator.
// Dot 8 (bit7, 0x80) lights up as a NUMBER indicator.
// Unknown characters show all 8 dots (0xFF).

struct BrailleEntry {
  char    ch;
  uint8_t dots;
};

static const BrailleEntry BRAILLE_MAP[] = {
  // ── Lowercase letters ──────────────────────────────────────────────────
  {'a', 0x01}, {'b', 0x03}, {'c', 0x09}, {'d', 0x19}, {'e', 0x11},
  {'f', 0x0B}, {'g', 0x1B}, {'h', 0x13}, {'i', 0x0A}, {'j', 0x1A},
  {'k', 0x05}, {'l', 0x07}, {'m', 0x0D}, {'n', 0x1D}, {'o', 0x15},
  {'p', 0x0F}, {'q', 0x1F}, {'r', 0x17}, {'s', 0x0E}, {'t', 0x1E},
  {'u', 0x25}, {'v', 0x27}, {'w', 0x3A}, {'x', 0x2D}, {'y', 0x3D},
  {'z', 0x35},

  // ── Uppercase letters (same braille + dot7 capital indicator) ──────────
  {'A', 0x01 | 0x40}, {'B', 0x03 | 0x40}, {'C', 0x09 | 0x40},
  {'D', 0x19 | 0x40}, {'E', 0x11 | 0x40}, {'F', 0x0B | 0x40},
  {'G', 0x1B | 0x40}, {'H', 0x13 | 0x40}, {'I', 0x0A | 0x40},
  {'J', 0x1A | 0x40}, {'K', 0x05 | 0x40}, {'L', 0x07 | 0x40},
  {'M', 0x0D | 0x40}, {'N', 0x1D | 0x40}, {'O', 0x15 | 0x40},
  {'P', 0x0F | 0x40}, {'Q', 0x1F | 0x40}, {'R', 0x17 | 0x40},
  {'S', 0x0E | 0x40}, {'T', 0x1E | 0x40}, {'U', 0x25 | 0x40},
  {'V', 0x27 | 0x40}, {'W', 0x3A | 0x40}, {'X', 0x2D | 0x40},
  {'Y', 0x3D | 0x40}, {'Z', 0x35 | 0x40},

  // ── Digits (same braille as a-j + dot8 number indicator) ──────────────
  {'1', 0x01 | 0x80}, {'2', 0x03 | 0x80}, {'3', 0x09 | 0x80},
  {'4', 0x19 | 0x80}, {'5', 0x11 | 0x80}, {'6', 0x0B | 0x80},
  {'7', 0x1B | 0x80}, {'8', 0x13 | 0x80}, {'9', 0x0A | 0x80},
  {'0', 0x1A | 0x80},

  // ── Punctuation & symbols ──────────────────────────────────────────────
  {' ', 0x00},
  {',', 0x02}, {';', 0x06}, {':', 0x12}, {'.', 0x32},
  {'?', 0x26}, {'!', 0x16}, {'-', 0x24}, {'\'', 0x04},
  {'"', 0x36}, {'(', 0x13}, {')', 0x2C},
};

static const int BRAILLE_MAP_LEN =
    (int)(sizeof(BRAILLE_MAP) / sizeof(BRAILLE_MAP[0]));

// ── Input buffer ─────────────────────────────────────────────────────────────
String inputBuffer;
static const size_t INPUT_MAX = 256;

// ── LED helpers ──────────────────────────────────────────────────────────────

void clearAll() {
  for (int i = 0; i < 8; i++) digitalWrite(DOT_PINS[i], LOW);
}

// dots: bitmask where bit(n-1) corresponds to dot n
void showPattern(uint8_t dots) {
  for (int i = 0; i < 8; i++) {
    digitalWrite(DOT_PINS[i], (dots >> i) & 1 ? HIGH : LOW);
  }
}

uint8_t lookupChar(char c) {
  for (int i = 0; i < BRAILLE_MAP_LEN; i++) {
    if (BRAILLE_MAP[i].ch == c) return BRAILLE_MAP[i].dots;
  }
  return 0xFF;  // all dots on = unknown character
}

// ── Startup / test sweep ─────────────────────────────────────────────────────

void sweepDots() {
  clearAll();
  for (int d = 0; d < 8; d++) {
    digitalWrite(DOT_PINS[d], HIGH);
    delay(150);
    digitalWrite(DOT_PINS[d], LOW);
    delay(60);
  }
  // All on briefly
  for (int i = 0; i < 8; i++) digitalWrite(DOT_PINS[i], HIGH);
  delay(400);
  clearAll();
}

// ── TEXT command ─────────────────────────────────────────────────────────────

void displayText(const String& text) {
  Serial.print("DISPLAYING:");
  Serial.println(text);

  for (int idx = 0; idx < (int)text.length(); idx++) {
    char    c    = text[idx];
    uint8_t dots = lookupChar(c);

    showPattern(dots);

    // Report the current character and its dot pattern (LSB = dot1)
    Serial.print("CHAR:");
    Serial.print(c);
    Serial.print("=DOTS_BIN:");
    for (int b = 7; b >= 0; b--) {
      Serial.print((dots >> b) & 1);
    }
    Serial.println();

    delay(charDelayMs);
  }

  clearAll();
  Serial.println("ACK:TEXT_DONE");
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
    Serial.println("PONG");

  } else if (msg.equalsIgnoreCase("TEST")) {
    sweepDots();
    Serial.println("ACK:TEST");

  } else if (msg.equalsIgnoreCase("HELP")) {
    Serial.println(
      "CMDS: TEXT:<str> | DELAY:<ms> | DOTS:<1..8,...> | DOTS:NONE | TEST | PING | HELP");

  } else if (msg.startsWith("DELAY:")) {
    int ms = msg.substring(6).toInt();
    if (ms > 0) {
      charDelayMs = (uint32_t)ms;
      Serial.print("ACK:DELAY:");
      Serial.println(charDelayMs);
    } else {
      Serial.println("ERROR:Invalid delay value");
    }

  } else if (msg.startsWith("TEXT:")) {
    displayText(msg.substring(5));

  } else if (msg.startsWith("DOTS:")) {
    String dotsStr = msg.substring(5);
    dotsStr.trim();
    if (dotsStr.equalsIgnoreCase("NONE") || dotsStr.length() == 0) {
      clearAll();
      Serial.println("ACK:CLEARED");
    } else {
      activateDotsFromString(dotsStr);
      Serial.println("ACK:DOTS:" + dotsStr);
    }

  } else {
    Serial.println("ERROR:Unknown command — send HELP for list");
  }
}

// ── Arduino entry points ─────────────────────────────────────────────────────

void setup() {
  Serial.begin(BAUD_RATE);
  delay(SERIAL_WAIT_MS);

  for (int i = 0; i < 8; i++) {
    pinMode(DOT_PINS[i], OUTPUT);
    digitalWrite(DOT_PINS[i], LOW);
  }

  Serial.println("READY:ESP32 Braille 8-LED Text Input");
  Serial.flush();

  sweepDots();
  Serial.println("ACK:STARTUP_SWEEP_DONE");
}

void loop() {
  while (Serial.available() > 0) {
    char c = static_cast<char>(Serial.read());
    if (c == '\n') {
      processLine(inputBuffer);
      inputBuffer = "";
    } else if (c != '\r') {
      if (inputBuffer.length() < INPUT_MAX) inputBuffer += c;
    }
  }
}
