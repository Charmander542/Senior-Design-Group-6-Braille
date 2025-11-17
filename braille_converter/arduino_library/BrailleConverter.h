/*
 * BrailleConverter.h
 * 
 * Arduino library for converting text to 8-dot Braille patterns
 * 
 * 8-dot braille uses a 2x4 grid:
 *    1 • • 4
 *    2 • • 5
 *    3 • • 6
 *    7 • • 8
 * 
 * Each character is represented by which dots are raised (1-8)
 */

#ifndef BRAILLE_CONVERTER_H
#define BRAILLE_CONVERTER_H

#include <Arduino.h>

// Maximum number of dots in a braille cell
#define MAX_BRAILLE_DOTS 8

// Maximum input string length
#define MAX_INPUT_LENGTH 256

// Structure to hold a braille character representation
struct BrailleChar {
    char original;              // Original ASCII character
    uint8_t dotPattern;         // Bit pattern: bit 0=dot1, bit 1=dot2, ..., bit 7=dot8
    uint8_t dotCount;           // Number of raised dots
    uint8_t dots[MAX_BRAILLE_DOTS]; // Array of raised dot numbers (1-8)
    
    // Constructor
    BrailleChar() : original(0), dotPattern(0), dotCount(0) {
        memset(dots, 0, sizeof(dots));
    }
};

class BrailleConverter {
public:
    // Constructor
    BrailleConverter();
    
    // Convert a single character to braille
    BrailleChar convertChar(char c);
    
    // Convert a string to braille (stores result internally)
    uint16_t convertText(const char* text);
    
    // Get the braille character at index from last conversion
    BrailleChar getCharAt(uint16_t index);
    
    // Get the number of characters from last conversion
    uint16_t getCharCount();
    
    // Get dot pattern (byte) for a single character
    uint8_t getDotPattern(char c);
    
    // Get array of raised dots for a character
    uint8_t getDots(char c, uint8_t* dotsArray);
    
    // Convert dot pattern byte to array of raised dot numbers
    uint8_t patternToDots(uint8_t pattern, uint8_t* dotsArray);
    
    // Print braille character info to Serial
    void printChar(const BrailleChar& bc);
    
    // Print visual dot pattern to Serial
    void printPattern(uint8_t pattern);
    
    // Print entire converted text
    void printConvertedText();
    
private:
    BrailleChar convertedChars[MAX_INPUT_LENGTH];
    uint16_t charCount;
    
    // Lookup table: maps ASCII char (0-127) to 8-dot braille pattern
    // For extended ASCII (128-255), returns 0xFF (unknown)
    static const uint8_t CHAR_TO_PATTERN[128];
    
    // Helper: Check if character is uppercase letter
    bool isUpperCase(char c);
    
    // Helper: Convert uppercase to lowercase
    char toLowerCase(char c);
};

// Global convenience functions
namespace Braille {
    // Quick conversion functions
    uint8_t charToDots(char c, uint8_t* dotsArray);
    uint8_t charToPattern(char c);
    void printDotPattern(uint8_t pattern);
}

#endif // BRAILLE_CONVERTER_H

