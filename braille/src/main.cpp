#include <Arduino.h>
#include "BrailleCell.h"

BrailleCell cell;

// Hardware mode:
// - false: direct pin mode (single tile)
// - true: chained SN74HC595 mode (multiple tiles)
const bool USE_SHIFT_REGISTER = true;

// Direct pin mode mapping: index = bit position in pattern byte
const int DOT_PINS[8] = {2, 3, 4, 8, 5, 6, 7, 9};

// SN74HC595 pin configuration and tile count
const uint8_t SR_DATA_PIN = 11;
const uint8_t SR_CLOCK_PIN = 12;
const uint8_t SR_LATCH_PIN = 8;
const uint8_t TILE_COUNT = 4;
const uint8_t DOT_CHIP_COUNT = 6; // 6-dot Braille: one chip per dot line
const uint8_t DOT_BIT_INDEX[6] = {0, 1, 2, 4, 5, 6};

uint8_t tilePatterns[TILE_COUNT];
char inputBuffer[96];
int bufferIndex = 0;

void writeDotPlaneFrame(const uint8_t* patterns, uint8_t count) {
  uint8_t dotPlanes[DOT_CHIP_COUNT];
  memset(dotPlanes, 0, sizeof(dotPlanes));

  // Convert tile-major patterns into dot-major bytes:
  // each chip byte uses bit N to represent tile N.
  for (uint8_t tile = 0; tile < count && tile < 8; tile++) {
    uint8_t pattern = patterns[tile];
    for (uint8_t dot = 0; dot < DOT_CHIP_COUNT; dot++) {
      if (pattern & (1u << DOT_BIT_INDEX[dot])) {
        dotPlanes[dot] |= (1u << tile);
      }
    }
  }

  digitalWrite(SR_LATCH_PIN, LOW);
  for (int chip = DOT_CHIP_COUNT - 1; chip >= 0; chip--) {
    shiftOut(SR_DATA_PIN, SR_CLOCK_PIN, MSBFIRST, dotPlanes[chip]);
  }
  digitalWrite(SR_LATCH_PIN, HIGH);
}

bool parseFrameCommand(const char* cmd) {
  // Command format: FRAME:XX,YY,ZZ,... one byte per tile
  if (strncmp(cmd, "FRAME:", 6) != 0) return false;

  memset(tilePatterns, 0, sizeof(tilePatterns));
  const char* p = cmd + 6;
  uint8_t tile = 0;

  while (*p && tile < TILE_COUNT) {
    char* endPtr = nullptr;
    unsigned long value = strtoul(p, &endPtr, 16);
    if (endPtr == p || value > 0xFF) {
      Serial.println("ERR:bad frame byte");
      return true;
    }
    tilePatterns[tile++] = (uint8_t)value;

    if (*endPtr == ',') {
      p = endPtr + 1;
      continue;
    }
    if (*endPtr == '\0') {
      break;
    }

    Serial.println("ERR:bad frame format");
    return true;
  }

  writeDotPlaneFrame(tilePatterns, TILE_COUNT);
  Serial.println("OK");
  return true;
}

void processCommand(const char* cmd) {
  if (USE_SHIFT_REGISTER && parseFrameCommand(cmd)) {
    return;
  }

  if (cmd[0] == 'P' && cmd[1] == ':') {
    // Pattern command: "P:XX" (tile 0) where XX is 2-digit hex
    uint8_t pattern = (uint8_t)strtoul(cmd + 2, NULL, 16);
    tilePatterns[0] = pattern;
    if (USE_SHIFT_REGISTER) {
      writeDotPlaneFrame(tilePatterns, TILE_COUNT);
    } else {
      cell.setPattern(pattern);
    }
    cell.printVisualization(pattern);
    Serial.println("OK");
  
  } else if (cmd[0] == 'T' && cmd[1] == ':') {
    // Tile command: "T:<tile>:<hex>" e.g. T:2:3F
    const char* firstColon = strchr(cmd, ':');
    if (!firstColon) {
      Serial.println("ERR:bad tile cmd");
      return;
    }
    const char* secondColon = strchr(firstColon + 1, ':');
    if (!secondColon) {
      Serial.println("ERR:bad tile cmd");
      return;
    }

    int tileIndex = atoi(firstColon + 1);
    uint8_t pattern = (uint8_t)strtoul(secondColon + 1, NULL, 16);
    uint8_t maxTiles = USE_SHIFT_REGISTER ? TILE_COUNT : cell.getTileCount();
    if (tileIndex < 0 || tileIndex >= maxTiles) {
      Serial.println("ERR:tile out of range");
      return;
    }

    tilePatterns[tileIndex] = pattern;
    if (USE_SHIFT_REGISTER) {
      writeDotPlaneFrame(tilePatterns, TILE_COUNT);
    } else {
      cell.setPattern((uint8_t)tileIndex, pattern);
    }
    Serial.println("OK");

  } else if (strcmp(cmd, "CLEAR") == 0) {
    memset(tilePatterns, 0, sizeof(tilePatterns));
    if (USE_SHIFT_REGISTER) {
      writeDotPlaneFrame(tilePatterns, TILE_COUNT);
    } else {
      for (uint8_t i = 0; i < cell.getTileCount(); i++) {
        cell.setPattern(i, 0x00);
      }
    }
    Serial.println("OK");

  } else if (strcmp(cmd, "PING") == 0) {
    Serial.println("PONG");

  } else if (strcmp(cmd, "TEST") == 0) {
    // Sweep each LED one at a time, then all on, then clear
    for (int i = 0; i < 8; i++) {
      cell.setPattern(1 << i);
      delay(150);
    }
    cell.setPattern(0xFF);
    delay(400);
    cell.clear();
    Serial.println("OK");

  } else {
    Serial.print("ERR:unknown cmd '");
    Serial.print(cmd);
    Serial.println("'");
  }
}

void setup() {
  Serial.begin(115200);
  memset(tilePatterns, 0, sizeof(tilePatterns));
  if (USE_SHIFT_REGISTER) {
    pinMode(SR_DATA_PIN, OUTPUT);
    pinMode(SR_CLOCK_PIN, OUTPUT);
    pinMode(SR_LATCH_PIN, OUTPUT);
    digitalWrite(SR_DATA_PIN, LOW);
    digitalWrite(SR_CLOCK_PIN, LOW);
    digitalWrite(SR_LATCH_PIN, HIGH);
    writeDotPlaneFrame(tilePatterns, TILE_COUNT);
  } else {
    cell.begin(DOT_PINS);
  }

  delay(500);
  Serial.println("BRAILLE_LED_READY");
  Serial.print("TILES:");
  Serial.println(USE_SHIFT_REGISTER ? TILE_COUNT : cell.getTileCount());
}

void loop() {
  while (Serial.available() > 0) {
    char c = Serial.read();

    if (c == '\r') continue;

    if (c == '\n') {
      inputBuffer[bufferIndex] = '\0';
      if (bufferIndex > 0) {
        processCommand(inputBuffer);
      }
      bufferIndex = 0;
    } else if (bufferIndex < (int)sizeof(inputBuffer) - 2) {
      inputBuffer[bufferIndex++] = c;
    }
  }
}
