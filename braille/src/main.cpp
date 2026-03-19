#include <Arduino.h>
#include "BrailleCell.h"

BrailleCell cell;

// Pin mapping: index = bit position in pattern byte
// Physical wiring: pin 2=dot1, pin 3=dot2, pin 4=dot3, pin 5=dot4,
//                  pin 6=dot5, pin 7=dot6, pin 8=dot7, pin 9=dot8
// Bit encoding:  bit0=dot1, bit1=dot2, bit2=dot3, bit3=dot7,
//                bit4=dot4, bit5=dot5, bit6=dot6, bit7=dot8
const int DOT_PINS[8] = {2, 3, 4, 8, 5, 6, 7, 9};

char inputBuffer[32];
int bufferIndex = 0;

void processCommand(const char* cmd) {
  if (cmd[0] == 'P' && cmd[1] == ':') {
    // Pattern command: "P:XX" where XX is 2-digit hex
    uint8_t pattern = (uint8_t)strtoul(cmd + 2, NULL, 16);
    cell.setPattern(pattern);
    cell.printVisualization(pattern);
    Serial.println("OK");

  } else if (strcmp(cmd, "CLEAR") == 0) {
    cell.clear();
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
  cell.begin(DOT_PINS);

  delay(500);
  Serial.println("BRAILLE_LED_READY");
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
