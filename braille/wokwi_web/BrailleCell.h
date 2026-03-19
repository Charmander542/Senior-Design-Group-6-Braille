// ============================================
// BrailleCell.h - Header File
// ============================================
// Add this as a new tab named "BrailleCell.h" on Wokwi

#ifndef BRAILLE_CELL_H
#define BRAILLE_CELL_H

#include <Arduino.h>

class BrailleCell {
  
public: 
  BrailleCell();
  void begin();
  void begin(const int dotPins[8]);
  void clear();
  void write(char c);
  void writeNumberIndicator();
  void setPattern(uint8_t pattern);
  void printVisualization(uint8_t pattern, const char* label = nullptr);

private:
  int _dotPins[8]; 
  uint8_t _translateToBraille(char c);
  void _writeToPins(uint8_t pattern);
  static uint8_t _bitIndexForDot(int dotNumber);
  static uint8_t _makePattern(const int* dots, int count);
};

#endif
