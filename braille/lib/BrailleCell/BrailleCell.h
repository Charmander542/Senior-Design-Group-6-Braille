/*
 * BrailleCell.h - Library for driving a single 2x3 6-dot Braille cell.
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
   * @param dotPins An array of 6 pin numbers (int) that control dots 1-6.
   */
  void begin(const int dotPins[6]);
  
  /**
   * @brief Initializes chained SN74HC595 shift registers for multiple tiles.
   * @param dataPin SER pin for the shift register chain.
   * @param clockPin SRCLK pin for the shift register chain.
   * @param latchPin RCLK pin for the shift register chain.
   * @param tileCount Number of 6-bit tiles in the chain.
   * @param outPin OE pin for the shift register chain (logical low for ON).
   */
  void beginShiftRegister(uint8_t dataPin, uint8_t clockPin, uint8_t latchPin, uint8_t tileCount, uint8_t outPin);

  /**
   * @brief Clears the cell (lowers all 6 dots).
   */
  void clear();

  /**
   * @brief Displays a single ASCII character on the cell.
   * Also prints a visual 2x3 (o)/(.) grid to Serial.
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
   * @brief Displays a raw 6-bit pattern on a specific tile in a shift register chain.
   * @param tileIndex Zero-based tile index.
   * @param pattern The 6-bit pattern.
   */
  void setPattern(uint8_t tileIndex, uint8_t pattern);
  
  /**
   * @brief Gets number of tiles currently configured.
   * @return Number of tiles (always 1 in direct pin mode).
   */
  uint8_t getTileCount() const;

  /**
   * @brief Prints the current pattern visualization to Serial.
   * @param pattern The pattern to visualize.
   * @param label Optional label to print above the visualization.
   */
  void printVisualization(uint8_t pattern, const char* label = nullptr);

private:
  
  int _dotPins[6]; 
  uint8_t _tilePatterns[32];
  uint8_t _tileCount;
  bool _useShiftRegister;
  uint8_t _dataPin;
  uint8_t _clockPin;
  uint8_t _latchPin;
  uint8_t _outPin;
  uint8_t _clearPin;
  
  uint8_t _translateToBraille(char c);
  void _writeToPins(uint8_t pattern);
  void _writeToShiftRegisters();
  static uint8_t _bitIndexForDot(int dotNumber);
  static uint8_t _makePattern(const int* dots, int count);
};

#endif
