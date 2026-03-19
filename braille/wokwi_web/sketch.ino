// ============================================
// BRAILLE SENTENCE CONVERTER - Main Sketch
// ============================================
// Copy this to the main sketch.ino tab on Wokwi

#include "BrailleCell.h"

BrailleCell cell;

// Buffer to store the input sentence
const int MAX_SENTENCE_LENGTH = 100;
char sentence[MAX_SENTENCE_LENGTH];
int sentenceIndex = 0;
bool sentenceReady = false;

void printBanner() {
  Serial.println();
  Serial.println("========================================");
  Serial.println("   BRAILLE SENTENCE CONVERTER");
  Serial.println("========================================");
  Serial.println();
  Serial.println("Supported characters:");
  Serial.println("  - Letters: a-z (case insensitive)");
  Serial.println("  - Numbers: 0-9 (with number indicator)");
  Serial.println("  - Punctuation: . , ; : ! ? ' - ( )");
  Serial.println("  - Space: displayed as blank cell");
  Serial.println();
  Serial.println("----------------------------------------");
  Serial.println("Enter a sentence and press Enter:");
  Serial.println("----------------------------------------");
}

void displaySentence(const char* text) {
  Serial.println();
  Serial.println("========================================");
  Serial.println("CONVERTING SENTENCE TO BRAILLE");
  Serial.println("========================================");
  Serial.print("Input: \"");
  Serial.print(text);
  Serial.println("\"");
  Serial.println("----------------------------------------");
  Serial.println();

  int len = strlen(text);
  bool inNumberMode = false;

  for (int i = 0; i < len; i++) {
    char c = text[i];
    
    // Check if we need a number indicator
    bool isDigit = (c >= '0' && c <= '9');
    if (isDigit && !inNumberMode) {
      // Display number indicator first
      Serial.println("[Number Indicator]");
      cell.writeNumberIndicator();
      Serial.println();
      inNumberMode = true;
    } else if (!isDigit && inNumberMode) {
      inNumberMode = false;
    }

    // Display character position
    Serial.print("Character ");
    Serial.print(i + 1);
    Serial.print("/");
    Serial.print(len);
    Serial.print(": ");
    
    // Write the character (displays in terminal)
    cell.write(c);
    Serial.println();
  }

  Serial.println("========================================");
  Serial.println("CONVERSION COMPLETE!");
  Serial.println("========================================");
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  cell.begin();  // Terminal-only mode (no hardware pins)
  
  // Wait a moment for serial to initialize
  delay(500);
  
  printBanner();
  
  // Clear the input buffer
  sentenceIndex = 0;
  sentenceReady = false;
  memset(sentence, 0, MAX_SENTENCE_LENGTH);
}

void loop() {
  // Read incoming characters
  while (Serial.available() > 0) {
    char c = Serial.read();
    
    // Check for end of line
    if (c == '\r' || c == '\n') {
      if (sentenceIndex > 0) {
        sentence[sentenceIndex] = '\0';
        sentenceReady = true;
      }
    } else if (sentenceIndex < MAX_SENTENCE_LENGTH - 1) {
      sentence[sentenceIndex++] = c;
    }
  }

  // Process the sentence when ready
  if (sentenceReady) {
    displaySentence(sentence);
    
    // Reset for next sentence
    sentenceIndex = 0;
    sentenceReady = false;
    memset(sentence, 0, MAX_SENTENCE_LENGTH);
    
    // Prompt for another sentence
    Serial.println();
    printBanner();
  }
}
