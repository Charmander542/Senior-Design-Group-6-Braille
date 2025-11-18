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
#include <avr/pgmspace.h>  // For PROGMEM storage

#define MAX_BRAILLE_DOTS 8
#define MAX_INPUT_LENGTH 128  // Reduce to save SRAM

struct BrailleChar {
    char original;
    uint8_t dotPattern;
    uint8_t dotCount;
    uint8_t dots[MAX_BRAILLE_DOTS];

    BrailleChar() : original(0), dotPattern(0), dotCount(0) {
        memset(dots, 0, sizeof(dots));
    }
};

class BrailleConverter {
public:
    BrailleConverter();

    BrailleChar convertChar(char c);
    uint16_t convertText(const char* text);
    BrailleChar getCharAt(uint16_t index);
    uint16_t getCharCount();
    uint8_t getDotPattern(char c);
    uint8_t getDots(char c, uint8_t* dotsArray);
    uint8_t patternToDots(uint8_t pattern, uint8_t* dotsArray);

private:
    BrailleChar convertedChars[MAX_INPUT_LENGTH];
    uint16_t charCount;

    static const uint8_t CHAR_TO_PATTERN[128] PROGMEM;

    bool isUpperCase(char c);
    char toLowerCase(char c);
};

namespace Braille {
uint8_t charToDots(char c, uint8_t* dotsArray);
uint8_t charToPattern(char c);
}

#endif // BRAILLE_CONVERTER_H
