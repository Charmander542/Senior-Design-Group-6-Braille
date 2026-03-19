#include "BrailleCell.h"

BrailleCell::BrailleCell() {
  for (int i = 0; i < 8; i++) {
    _dotPins[i] = -1;
  }
}

void BrailleCell::begin() {
  // Terminal-only mode - no hardware pins needed
  for (int i = 0; i < 8; i++) {
    _dotPins[i] = -1;
  }
}

void BrailleCell::begin(const int dotPins[8]) {
  for (int i = 0; i < 8; i++) {
    _dotPins[i] = dotPins[i];
    if (_dotPins[i] >= 0) {
      pinMode(_dotPins[i], OUTPUT);
      digitalWrite(_dotPins[i], LOW);
    }
  }
  clear();
}

void BrailleCell::clear() {
  setPattern(0);
}

void BrailleCell::write(char c) {
  uint8_t pattern = _translateToBraille(c);
  setPattern(pattern);

  // Print visualization to Serial
  if (Serial) {
    char label[30];
    if (c == ' ') {
      snprintf(label, sizeof(label), "'SPACE'");
    } else if (c >= 33 && c <= 126) {
      snprintf(label, sizeof(label), "'%c'", c);
    } else {
      snprintf(label, sizeof(label), "(0x%02X)", (unsigned char)c);
    }
    printVisualization(pattern, label);
  }
}

void BrailleCell::writeNumberIndicator() {
  // Number indicator in Braille: dots 3, 4, 5, 6
  const int dots[] = {3, 4, 5, 6};
  uint8_t pattern = _makePattern(dots, 4);
  setPattern(pattern);
  
  if (Serial) {
    printVisualization(pattern, "#NUM");
  }
}

void BrailleCell::setPattern(uint8_t pattern) {
  _writeToPins(pattern);
}

void BrailleCell::printVisualization(uint8_t pattern, const char* label) {
  if (!Serial) return;
  
  if (label) {
    Serial.println(label);
  }
  
  Serial.println("+---+---+");
  
  // Braille cell layout:
  // | 1 | 4 |
  // | 2 | 5 |
  // | 3 | 6 |
  // | 7 | 8 |
  const int leftDots[4]  = {1, 2, 3, 7};
  const int rightDots[4] = {4, 5, 6, 8};
  
  for (int row = 0; row < 4; row++) {
    uint8_t leftBit = (1u << _bitIndexForDot(leftDots[row]));
    uint8_t rightBit = (1u << _bitIndexForDot(rightDots[row]));
    
    Serial.print("| ");
    Serial.print((pattern & leftBit) ? "O" : ".");
    Serial.print(" | ");
    Serial.print((pattern & rightBit) ? "O" : ".");
    Serial.println(" |");
  }
  
  Serial.println("+---+---+");
}

uint8_t BrailleCell::_makePattern(const int* dots, int count) {
  uint8_t p = 0;
  for (int i = 0; i < count; i++) {
    uint8_t bit = BrailleCell::_bitIndexForDot(dots[i]);
    p |= (1u << bit);
  }
  return p;
}

uint8_t BrailleCell::_translateToBraille(char c) {
  // Convert to lowercase for letters
  char lc = (char)tolower((unsigned char)c);
  
  // Helper macro for creating patterns
  #define PATTERN(...) \
    do { \
      const int _dots[] = {__VA_ARGS__}; \
      return _makePattern(_dots, sizeof(_dots)/sizeof(_dots[0])); \
    } while(0)

  switch (lc) {
    // Letters a-z
    case 'a': PATTERN(1);
    case 'b': PATTERN(1,2);
    case 'c': PATTERN(1,4);
    case 'd': PATTERN(1,4,5);
    case 'e': PATTERN(1,5);
    case 'f': PATTERN(1,2,4);
    case 'g': PATTERN(1,2,4,5);
    case 'h': PATTERN(1,2,5);
    case 'i': PATTERN(2,4);
    case 'j': PATTERN(2,4,5);
    case 'k': PATTERN(1,3);
    case 'l': PATTERN(1,2,3);
    case 'm': PATTERN(1,3,4);
    case 'n': PATTERN(1,3,4,5);
    case 'o': PATTERN(1,3,5);
    case 'p': PATTERN(1,2,3,4);
    case 'q': PATTERN(1,2,3,4,5);
    case 'r': PATTERN(1,2,3,5);
    case 's': PATTERN(2,3,4);
    case 't': PATTERN(2,3,4,5);
    case 'u': PATTERN(1,3,6);
    case 'v': PATTERN(1,2,3,6);
    case 'w': PATTERN(2,4,5,6);
    case 'x': PATTERN(1,3,4,6);
    case 'y': PATTERN(1,3,4,5,6);
    case 'z': PATTERN(1,3,5,6);
    
    // Numbers 0-9 (same as letters a-j, preceded by number indicator)
    case '1': PATTERN(1);           // same as 'a'
    case '2': PATTERN(1,2);         // same as 'b'
    case '3': PATTERN(1,4);         // same as 'c'
    case '4': PATTERN(1,4,5);       // same as 'd'
    case '5': PATTERN(1,5);         // same as 'e'
    case '6': PATTERN(1,2,4);       // same as 'f'
    case '7': PATTERN(1,2,4,5);     // same as 'g'
    case '8': PATTERN(1,2,5);       // same as 'h'
    case '9': PATTERN(2,4);         // same as 'i'
    case '0': PATTERN(2,4,5);       // same as 'j'
    
    // Punctuation
    case '.': PATTERN(2,5,6);       // Period
    case ',': PATTERN(2);           // Comma
    case ';': PATTERN(2,3);         // Semicolon
    case ':': PATTERN(2,5);         // Colon
    case '!': PATTERN(2,3,5);       // Exclamation mark
    case '?': PATTERN(2,3,6);       // Question mark
    case '\'': PATTERN(3);          // Apostrophe
    case '-': PATTERN(3,6);         // Hyphen
    case '(': PATTERN(1,2,6);       // Opening parenthesis  
    case ')': PATTERN(3,4,5);       // Closing parenthesis
    case '"': PATTERN(2,3,5,6);     // Quotation mark (opening)
    
    case ' ': return 0;             // Space = no dots
    
    default:
      return 0; // Unsupported characters show as blank
  }
  
  #undef PATTERN
}

void BrailleCell::_writeToPins(uint8_t pattern) {
  for (int i = 0; i < 8; i++) {
    int pin = _dotPins[i];
    if (pin >= 0) {
      bool on = (pattern & (1u << i)) != 0;
      digitalWrite(pin, on ? HIGH : LOW);
    }
  }
}

uint8_t BrailleCell::_bitIndexForDot(int dotNumber) {
  // Bit layout:
  // bit0=dot1, bit1=dot2, bit2=dot3, bit3=dot7, 
  // bit4=dot4, bit5=dot5, bit6=dot6, bit7=dot8
  switch (dotNumber) {
    case 1: return 0;
    case 2: return 1;
    case 3: return 2;
    case 7: return 3;
    case 4: return 4;
    case 5: return 5;
    case 6: return 6;
    case 8: return 7;
    default: return 0;
  }
}
