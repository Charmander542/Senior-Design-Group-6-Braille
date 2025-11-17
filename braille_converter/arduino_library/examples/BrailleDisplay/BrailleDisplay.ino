/*
 * BrailleDisplay.ino
 * 
 * Example for driving a physical 8-dot Braille display
 * Demonstrates how to use the converter to control solenoids or actuators
 */

#include <BrailleConverter.h>

BrailleConverter converter;

// Pin configuration for 8-dot braille cell
// Adjust these pins based on your hardware setup
const uint8_t DOT_PINS[8] = {
  2,  // Dot 1
  3,  // Dot 2
  4,  // Dot 3
  5,  // Dot 4
  6,  // Dot 5
  7,  // Dot 6
  8,  // Dot 7
  9   // Dot 8
};

// Display timing
const uint16_t CHAR_DISPLAY_TIME = 2000;  // 2 seconds per character
const uint16_t CLEAR_TIME = 500;          // 0.5 seconds between characters

// Text to display
const char* demoText = "Hello";

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    ; // Wait for serial port to connect
  }
  
  // Initialize dot pins as outputs
  for (uint8_t i = 0; i < 8; i++) {
    pinMode(DOT_PINS[i], OUTPUT);
    digitalWrite(DOT_PINS[i], LOW);
  }
  
  Serial.println("========================================");
  Serial.println("  BrailleConverter Display Example");
  Serial.println("========================================");
  Serial.println();
  Serial.println("Pin Configuration:");
  for (uint8_t i = 0; i < 8; i++) {
    Serial.print("  Dot ");
    Serial.print(i + 1);
    Serial.print(" -> Pin ");
    Serial.println(DOT_PINS[i]);
  }
  Serial.println();
  
  // Test all dots
  Serial.println("Testing all dots...");
  testAllDots();
  
  Serial.println("Ready to display text!");
  Serial.println();
}

void loop() {
  Serial.println("========================================");
  Serial.print("Displaying: \"");
  Serial.print(demoText);
  Serial.println("\"");
  Serial.println("========================================");
  Serial.println();
  
  // Convert and display the text
  uint16_t charCount = converter.convertText(demoText);
  
  for (uint16_t i = 0; i < charCount; i++) {
    BrailleChar bc = converter.getCharAt(i);
    
    Serial.print("[");
    Serial.print(i + 1);
    Serial.print("/");
    Serial.print(charCount);
    Serial.print("] Displaying '");
    Serial.print(bc.original);
    Serial.print("' - Dots: ");
    
    if (bc.dotCount == 0) {
      Serial.println("[none]");
    } else {
      Serial.print("[");
      for (uint8_t j = 0; j < bc.dotCount; j++) {
        Serial.print(bc.dots[j]);
        if (j < bc.dotCount - 1) Serial.print(",");
      }
      Serial.println("]");
    }
    
    // Display the pattern
    displayPattern(bc.dotPattern);
    delay(CHAR_DISPLAY_TIME);
    
    // Clear between characters
    if (i < charCount - 1) {
      Serial.println("  [clearing...]");
      clearDisplay();
      delay(CLEAR_TIME);
    }
  }
  
  Serial.println();
  Serial.println("Sequence complete! Waiting 3 seconds...");
  Serial.println();
  
  clearDisplay();
  delay(3000);
}

// Display a braille pattern on the physical display
void displayPattern(uint8_t pattern) {
  for (uint8_t i = 0; i < 8; i++) {
    // Check if dot i+1 should be raised
    if (pattern & (1 << i)) {
      digitalWrite(DOT_PINS[i], HIGH);  // Raise dot
    } else {
      digitalWrite(DOT_PINS[i], LOW);   // Lower dot
    }
  }
}

// Clear the display (all dots down)
void clearDisplay() {
  for (uint8_t i = 0; i < 8; i++) {
    digitalWrite(DOT_PINS[i], LOW);
  }
}

// Test each dot individually
void testAllDots() {
  Serial.println("  Testing dots 1-8 individually...");
  
  for (uint8_t i = 0; i < 8; i++) {
    Serial.print("    Dot ");
    Serial.print(i + 1);
    Serial.print(" (Pin ");
    Serial.print(DOT_PINS[i]);
    Serial.println(")");
    
    digitalWrite(DOT_PINS[i], HIGH);
    delay(300);
    digitalWrite(DOT_PINS[i], LOW);
    delay(200);
  }
  
  Serial.println("  All dots tested!");
  Serial.println();
}

