/*
 * ESP32-S3 — 8-LED diagnostic test
 *
 * Tests each GPIO individually with a long hold so you can
 * clearly see which LEDs work and which don't.
 * Open Serial Monitor at 115200.
 */

#include <Arduino.h>

static const int NUM_LEDS = 8;
// Same order as arduino_8dot_braille.ino DOT_PINS (dot 1..8 -> GPIO).
// Dots 5–6 use GPIO 16/17 (not 12/9 — those conflict with default SPI SCK / I2C SCL on many S3 boards).
const int LED_PINS[NUM_LEDS] = {5, 6, 7, 15, 16, 17, 11, 10};

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n========================================");
  Serial.println("  8-LED Diagnostic Test — ESP32-S3");
  Serial.println("========================================\n");

  for (int i = 0; i < NUM_LEDS; i++) {
    pinMode(LED_PINS[i], OUTPUT);
    digitalWrite(LED_PINS[i], LOW);
  }

  Serial.println("All pins set to OUTPUT. Starting test...\n");
}

void loop() {
  // --- Test each LED one at a time ---
  for (int i = 0; i < NUM_LEDS; i++) {
    Serial.print(">> Dot ");
    Serial.print(i + 1);
    Serial.print("  GPIO ");
    Serial.print(LED_PINS[i]);
    Serial.print(" -> HIGH   (look for this LED now)");
    if (LED_PINS[i] == 46) {
      Serial.print("  *** WARNING: GPIO 46 is input-only on ESP32-S3! ***");
    }
    Serial.println();

    digitalWrite(LED_PINS[i], HIGH);
    delay(1500);
    digitalWrite(LED_PINS[i], LOW);
    delay(500);
  }

  // --- All on together ---
  Serial.println("\n>> ALL LEDs ON (8 should light up)");
  for (int i = 0; i < NUM_LEDS; i++) {
    digitalWrite(LED_PINS[i], HIGH);
  }
  delay(2000);

  for (int i = 0; i < NUM_LEDS; i++) {
    digitalWrite(LED_PINS[i], LOW);
  }
  Serial.println(">> ALL LEDs OFF\n");
  delay(1000);

  Serial.println("--- Cycle done. Restarting in 3s ---\n");
  delay(3000);
}
