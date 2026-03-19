/*
 * BrailleCell.h - Library for driving a single 2x4 8-dot Braille cell.
 * Supports letters a-z, numbers 0-9, and common punctuation.
 */

#ifndef BRAILLE_CELL_H
#define BRAILLE_CELL_H

#include <Arduino.h>

class BrailleCell {
  
public: 
  
  // Constructor
  BrailleCell();

  /**
   * @brief Initializes the Braille cell for terminal-only mode (no hardware).
   */
  void begin();

  /**
   * @brief Initializes the Braille cell and sets up the pins.
   * @param dotPins An array of 8 pin numbers (int) that control dots 1-8.
   */
  void begin(const int dotPins[8]);

  /**
   * @brief Clears the cell (lowers all 8 dots).
   */
  void clear();

  /**
   * @brief Displays a single ASCII character on the cell.
   * Also prints a visual 2x4 (o)/(.) grid to Serial.
   * @param c The character to display (e.g., 'a', 'b', '1', '.').
   */
  void write(char c);

  /**
   * @brief Displays the number indicator (dots 3,4,5,6).
   * In Braille, numbers use the same patterns as letters a-j,
   * but are preceded by a number indicator.
   */
  void writeNumberIndicator();

  /**
   * @brief Displays a raw 8-bit pattern on the cell.
   * @param pattern The 8-bit pattern.
   */
  void setPattern(uint8_t pattern);

  /**
   * @brief Prints the current pattern visualization to Serial.
   * @param pattern The pattern to visualize.
   * @param label Optional label to print above the visualization.
   */
  void printVisualization(uint8_t pattern, const char* label = nullptr);

private:
  
  int _dotPins[8]; 
  
  uint8_t _translateToBraille(char c);
  void _writeToPins(uint8_t pattern);
  static uint8_t _bitIndexForDot(int dotNumber);
  static uint8_t _makePattern(const int* dots, int count);
};

#endif
