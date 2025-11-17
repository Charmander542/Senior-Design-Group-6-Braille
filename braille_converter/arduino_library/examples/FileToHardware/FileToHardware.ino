/*
 * FileToHardware.ino
 * 
 * Complete example: Read .txt file from SD card and display on 8-dot Braille hardware
 * 
 * This demonstrates the COMPLETE workflow:
 * 1. Read text from .txt file on SD card
 * 2. Convert each character to 8-bit Braille pattern
 * 3. Output directly to 8 Arduino pins (for solenoids/actuators)
 * 
 * Hardware Requirements:
 * - Arduino board (Uno, Mega, etc.)
 * - SD card module (CS pin 10)
 * - 8 output pins for Braille dots (pins 2-9)
 * - SD card with "input.txt" file
 * - Optional: 8 solenoids/actuators with driver circuits
 */

#include <BrailleConverter.h>
#include <SD.h>

BrailleConverter converter;

// SD card configuration
const int CS_PIN = 10;
const char* INPUT_FILE = "input.txt";

// 8-dot Braille hardware pins
const uint8_t DOT_PINS[8] = {
  2,  // Dot 1 (top-left)
  3,  // Dot 2
  4,  // Dot 3
  5,  // Dot 4 (top-right)
  6,  // Dot 5
  7,  // Dot 6
  8,  // Dot 7 (bottom-left)
  9   // Dot 8 (bottom-right)
};

// Timing configuration
const uint16_t CHAR_DISPLAY_TIME = 2000;  // 2 seconds per character
const uint16_t CLEAR_TIME = 500;          // 0.5 seconds between characters

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    ; // Wait for serial port
  }
  
  Serial.println("=========================================");
  Serial.println(" .TXT File to 8-Bit Braille Hardware");
  Serial.println("=========================================");
  Serial.println();
  
  // Initialize hardware pins
  Serial.println("1. Initializing hardware pins...");
  for (uint8_t i = 0; i < 8; i++) {
    pinMode(DOT_PINS[i], OUTPUT);
    digitalWrite(DOT_PINS[i], LOW);
  }
  Serial.println("   ✓ 8 pins configured (2-9)");
  Serial.println();
  
  // Initialize SD card
  Serial.println("2. Initializing SD card...");
  if (!SD.begin(CS_PIN)) {
    Serial.println("   ✗ SD card initialization FAILED!");
    Serial.println("   Check: SD card inserted, wiring, CS pin");
    while (1) delay(1000);
  }
  Serial.println("   ✓ SD card ready");
  Serial.println();
  
  // Test hardware
  Serial.println("3. Testing all dots...");
  testAllDots();
  Serial.println("   ✓ Hardware test complete");
  Serial.println();
  
  // Process the file
  Serial.println("4. Reading and displaying text file...");
  Serial.println("=========================================");
  processFileToHardware(INPUT_FILE);
  
  Serial.println();
  Serial.println("=========================================");
  Serial.println("Demo complete! Will repeat every 5 seconds.");
  Serial.println("=========================================");
}

void loop() {
  delay(5000);
  Serial.println("\n--- Repeating display ---\n");
  processFileToHardware(INPUT_FILE);
}

/**
 * Main function: Read .txt file and display each character on hardware
 */
void processFileToHardware(const char* filename) {
  // Open the file
  File file = SD.open(filename);
  if (!file) {
    Serial.println("ERROR: Could not open file!");
    Serial.print("Looking for: ");
    Serial.println(filename);
    return;
  }
  
  Serial.print("Opened file: ");
  Serial.println(filename);
  Serial.print("File size: ");
  Serial.print(file.size());
  Serial.println(" bytes");
  Serial.println();
  
  uint32_t charNumber = 0;
  
  // Read and process each character from file
  while (file.available()) {
    char c = file.read();
    
    // Skip newlines and carriage returns for display
    if (c == '\n' || c == '\r') {
      Serial.println("[newline]");
      continue;
    }
    
    charNumber++;
    
    // Convert character to 8-bit Braille pattern
    BrailleChar bc = converter.convertChar(c);
    
    // Display information
    Serial.print("[");
    Serial.print(charNumber);
    Serial.print("] '");
    Serial.print(c);
    Serial.print("' -> ");
    
    // Show 8-bit pattern in binary and hex
    Serial.print("0x");
    if (bc.dotPattern < 0x10) Serial.print("0");
    Serial.print(bc.dotPattern, HEX);
    Serial.print(" (0b");
    for (int8_t i = 7; i >= 0; i--) {
      Serial.print((bc.dotPattern >> i) & 1);
    }
    Serial.print(") -> Dots: [");
    
    // Show which dots are raised
    if (bc.dotCount == 0) {
      Serial.print("none");
    } else {
      for (uint8_t i = 0; i < bc.dotCount; i++) {
        Serial.print(bc.dots[i]);
        if (i < bc.dotCount - 1) Serial.print(",");
      }
    }
    Serial.println("]");
    
    // OUTPUT TO HARDWARE: Send 8-bit pattern to Arduino pins
    displayPattern8Bit(bc.dotPattern);
    
    // Hold for viewing
    delay(CHAR_DISPLAY_TIME);
    
    // Clear between characters
    clearDisplay();
    delay(CLEAR_TIME);
  }
  
  file.close();
  
  Serial.println();
  Serial.print("Processed ");
  Serial.print(charNumber);
  Serial.println(" characters from file.");
}

/**
 * Output 8-bit pattern directly to Arduino pins
 * This is the KEY function that drives the hardware
 */
void displayPattern8Bit(uint8_t pattern) {
  // Each bit controls one pin/dot
  // Bit 0 (LSB) = Dot 1, Bit 7 (MSB) = Dot 8
  
  for (uint8_t i = 0; i < 8; i++) {
    bool dotRaised = pattern & (1 << i);  // Check if bit i is set
    digitalWrite(DOT_PINS[i], dotRaised ? HIGH : LOW);
  }
  
  // Debug output showing pin states
  Serial.print("    Pins: [");
  for (uint8_t i = 0; i < 8; i++) {
    Serial.print(digitalRead(DOT_PINS[i]) ? "HIGH" : "LOW");
    if (i < 7) Serial.print(", ");
  }
  Serial.println("]");
}

/**
 * Clear all dots (all pins LOW)
 */
void clearDisplay() {
  for (uint8_t i = 0; i < 8; i++) {
    digitalWrite(DOT_PINS[i], LOW);
  }
}

/**
 * Test each dot individually
 */
void testAllDots() {
  for (uint8_t i = 0; i < 8; i++) {
    Serial.print("   Testing Dot ");
    Serial.print(i + 1);
    Serial.print(" (Pin ");
    Serial.print(DOT_PINS[i]);
    Serial.println(")");
    
    digitalWrite(DOT_PINS[i], HIGH);
    delay(200);
    digitalWrite(DOT_PINS[i], LOW);
    delay(100);
  }
}

/**
 * Alternative: Process entire file at once, then display
 */
void processFileBuffered(const char* filename) {
  File file = SD.open(filename);
  if (!file) return;
  
  // Read entire file into string
  String fileContent = "";
  while (file.available()) {
    char c = file.read();
    if (c != '\n' && c != '\r') {
      fileContent += c;
    }
  }
  file.close();
  
  // Convert to char array
  char buffer[256];
  fileContent.toCharArray(buffer, 256);
  
  // Convert all at once
  uint16_t count = converter.convertText(buffer);
  
  Serial.print("Converted ");
  Serial.print(count);
  Serial.println(" characters.");
  Serial.println();
  
  // Display each character on hardware
  for (uint16_t i = 0; i < count; i++) {
    BrailleChar bc = converter.getCharAt(i);
    
    Serial.print("[");
    Serial.print(i + 1);
    Serial.print("/");
    Serial.print(count);
    Serial.print("] '");
    Serial.print(bc.original);
    Serial.print("' -> 0x");
    Serial.println(bc.dotPattern, HEX);
    
    displayPattern8Bit(bc.dotPattern);
    delay(CHAR_DISPLAY_TIME);
    clearDisplay();
    delay(CLEAR_TIME);
  }
}

