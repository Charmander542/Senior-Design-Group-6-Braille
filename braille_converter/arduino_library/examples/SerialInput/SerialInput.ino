/*
 * SerialInput.ino
 * 
 * Interactive example: Type text via Serial Monitor
 * and see the Braille conversion in real-time
 */

#include <BrailleConverter.h>

BrailleConverter converter;
String inputBuffer = "";
const uint16_t BUFFER_SIZE = 256;

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    ; // Wait for serial port to connect
  }
  
  Serial.println("========================================");
  Serial.println("  BrailleConverter Interactive Mode");
  Serial.println("========================================");
  Serial.println();
  Serial.println("Type text and press Enter to convert");
  Serial.println("Commands:");
  Serial.println("  'help' - Show this help");
  Serial.println("  'clear' - Clear screen");
  Serial.println("  'demo' - Run demo");
  Serial.println();
  Serial.print("> ");
}

void loop() {
  // Check for serial input
  if (Serial.available() > 0) {
    char c = Serial.read();
    
    // Echo character
    Serial.print(c);
    
    if (c == '\n' || c == '\r') {
      // Process the input
      Serial.println(); // New line
      
      if (inputBuffer.length() > 0) {
        processInput(inputBuffer);
        inputBuffer = "";
      }
      
      Serial.print("> ");
    } else if (c == 8 || c == 127) {
      // Backspace
      if (inputBuffer.length() > 0) {
        inputBuffer.remove(inputBuffer.length() - 1);
      }
    } else if (inputBuffer.length() < BUFFER_SIZE) {
      inputBuffer += c;
    }
  }
}

void processInput(String input) {
  input.trim();
  
  if (input.length() == 0) {
    return;
  }
  
  // Handle commands
  if (input.equalsIgnoreCase("help")) {
    showHelp();
    return;
  }
  
  if (input.equalsIgnoreCase("clear")) {
    Serial.write(27);       // ESC
    Serial.print("[2J");    // Clear screen
    Serial.write(27);       // ESC
    Serial.print("[H");     // Move cursor to home
    Serial.println("Screen cleared!");
    return;
  }
  
  if (input.equalsIgnoreCase("demo")) {
    runDemo();
    return;
  }
  
  // Convert the input
  convertAndDisplay(input);
}

void convertAndDisplay(String text) {
  Serial.println();
  Serial.println("========================================");
  Serial.print("Input: \"");
  Serial.print(text);
  Serial.println("\"");
  Serial.println("========================================");
  
  // Convert text
  char buffer[BUFFER_SIZE];
  text.toCharArray(buffer, BUFFER_SIZE);
  uint16_t count = converter.convertText(buffer);
  
  Serial.print("Converted ");
  Serial.print(count);
  Serial.println(" character(s):");
  Serial.println();
  
  // Display each character
  for (uint16_t i = 0; i < count; i++) {
    BrailleChar bc = converter.getCharAt(i);
    
    Serial.print("[");
    if (i < 10) Serial.print(" ");
    Serial.print(i);
    Serial.print("] '");
    
    if (bc.original == ' ') {
      Serial.print("SPACE");
    } else if (bc.original == '\n') {
      Serial.print("NEWLINE");
    } else if (bc.original == '\t') {
      Serial.print("TAB");
    } else {
      Serial.print(bc.original);
    }
    
    Serial.print("' -> 0x");
    if (bc.dotPattern < 0x10) Serial.print("0");
    Serial.print(bc.dotPattern, HEX);
    Serial.print(" -> Dots: ");
    
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
  }
  
  Serial.println("========================================");
  Serial.println();
}

void showHelp() {
  Serial.println();
  Serial.println("========================================");
  Serial.println("  Available Commands");
  Serial.println("========================================");
  Serial.println("  help  - Show this help message");
  Serial.println("  clear - Clear the screen");
  Serial.println("  demo  - Run demonstration");
  Serial.println();
  Serial.println("Any other text will be converted to");
  Serial.println("8-dot Braille patterns.");
  Serial.println("========================================");
  Serial.println();
}

void runDemo() {
  Serial.println();
  Serial.println("========================================");
  Serial.println("  Running Demo");
  Serial.println("========================================");
  Serial.println();
  
  // Demo alphabet
  Serial.println("Lowercase alphabet:");
  for (char c = 'a'; c <= 'z'; c++) {
    BrailleChar bc = converter.convertChar(c);
    Serial.print(c);
    Serial.print(":");
    for (uint8_t i = 0; i < bc.dotCount; i++) {
      Serial.print(bc.dots[i]);
      if (i < bc.dotCount - 1) Serial.print(",");
    }
    Serial.print(" ");
    if ((c - 'a' + 1) % 6 == 0) Serial.println();
  }
  Serial.println();
  Serial.println();
  
  // Demo digits
  Serial.println("Digits:");
  for (char c = '0'; c <= '9'; c++) {
    BrailleChar bc = converter.convertChar(c);
    Serial.print(c);
    Serial.print(":");
    for (uint8_t i = 0; i < bc.dotCount; i++) {
      Serial.print(bc.dots[i]);
      if (i < bc.dotCount - 1) Serial.print(",");
    }
    Serial.print(" ");
  }
  Serial.println();
  Serial.println();
  
  // Demo visual pattern
  Serial.println("Visual pattern for 'A':");
  BrailleChar bc = converter.convertChar('A');
  converter.printPattern(bc.dotPattern);
  
  Serial.println();
  Serial.println("========================================");
  Serial.println();
}

