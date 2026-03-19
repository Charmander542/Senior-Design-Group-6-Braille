// ============================================
// BrailleCell.cpp - Implementation File
// ============================================
// Add this as a new tab named "BrailleCell.cpp" on Wokwi

#include "BrailleCell.h"

BrailleCell::BrailleCell() {
  for (int i = 0; i < 8; i++) {
    _dotPins[i] = -1;
  }
}

void BrailleCell::begin() {
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
  char lc = (char)tolower((unsigned char)c);
  
  #define PATTERN(...) \
    do { \
      const int _dots[] = {__VA_ARGS__}; \
      return _makePattern(_dots, sizeof(_dots)/sizeof(_dots[0])); \
    } while(0)

  switch (lc) {
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
    
    case '1': PATTERN(1);
    case '2': PATTERN(1,2);
    case '3': PATTERN(1,4);
    case '4': PATTERN(1,4,5);
    case '5': PATTERN(1,5);
    case '6': PATTERN(1,2,4);
    case '7': PATTERN(1,2,4,5);
    case '8': PATTERN(1,2,5);
    case '9': PATTERN(2,4);
    case '0': PATTERN(2,4,5);
    
    case '.': PATTERN(2,5,6);
    case ',': PATTERN(2);
    case ';': PATTERN(2,3);
    case ':': PATTERN(2,5);
    case '!': PATTERN(2,3,5);
    case '?': PATTERN(2,3,6);
    case '\'': PATTERN(3);
    case '-': PATTERN(3,6);
    case '(': PATTERN(1,2,6);
    case ')': PATTERN(3,4,5);
    case '"': PATTERN(2,3,5,6);
    
    case ' ': return 0;
    
    default: return 0;
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
