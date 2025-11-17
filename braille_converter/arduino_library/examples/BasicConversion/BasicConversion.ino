/*
 * BasicConversion.ino
 * 
 * Basic example of using the BrailleConverter library
 * Demonstrates converting text to 8-dot Braille patterns
 */

#include <BrailleConverter.h>

// Create a converter instance
BrailleConverter converter;

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    ; // Wait for serial port to connect
  }
  
  Serial.println("========================================");
  Serial.println("  BrailleConverter Basic Example");
  Serial.println("========================================");
  Serial.println();
  
  // Example 1: Convert a single character
  Serial.println("Example 1: Single Character");
  Serial.println("----------------------------");
  
  char testChar = 'A';
  BrailleChar bc = converter.convertChar(testChar);
  
  Serial.print("Character: '");
  Serial.print(testChar);
  Serial.println("'");
  Serial.print("Dot Pattern (hex): 0x");
  Serial.println(bc.dotPattern, HEX);
  Serial.print("Raised Dots: ");
  for (uint8_t i = 0; i < bc.dotCount; i++) {
    Serial.print(bc.dots[i]);
    if (i < bc.dotCount - 1) Serial.print(", ");
  }
  Serial.println();
  Serial.println();
  
  converter.printPattern(bc.dotPattern);
  Serial.println();
  
  // Example 2: Convert a word
  Serial.println("Example 2: Convert Word");
  Serial.println("------------------------");
  
  const char* word = "Hello";
  converter.convertText(word);
  converter.printConvertedText();
  Serial.println();
  
  // Example 3: Convert a sentence
  Serial.println("Example 3: Convert Sentence");
  Serial.println("----------------------------");
  
  const char* sentence = "Braille 8-dot!";
  uint16_t count = converter.convertText(sentence);
  
  Serial.print("Converted ");
  Serial.print(count);
  Serial.print(" characters from: \"");
  Serial.print(sentence);
  Serial.println("\"");
  Serial.println();
  
  // Print each character's dot pattern
  for (uint16_t i = 0; i < count; i++) {
    BrailleChar ch = converter.getCharAt(i);
    
    if (ch.original == ' ') {
      Serial.println("[SPACE]");
      continue;
    }
    
    Serial.print("'");
    Serial.print(ch.original);
    Serial.print("' -> Dots: ");
    
    if (ch.dotCount == 0) {
      Serial.println("[none]");
    } else {
      for (uint8_t j = 0; j < ch.dotCount; j++) {
        Serial.print(ch.dots[j]);
        if (j < ch.dotCount - 1) Serial.print(",");
      }
      Serial.println();
    }
  }
  
  Serial.println();
  Serial.println("========================================");
  Serial.println("Setup complete!");
}

void loop() {
  // Nothing to do in loop
  delay(1000);
}

