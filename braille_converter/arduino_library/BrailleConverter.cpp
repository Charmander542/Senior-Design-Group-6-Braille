#include "BrailleConverter.h"

const uint8_t BrailleConverter::CHAR_TO_PATTERN[128] PROGMEM = {
    // Control characters (0-31) - mostly undefined, using 0xFF
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 0-7
    0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, // 8-15 (tab=0x00, newline=0x00, carriage return=0x00)
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 16-23
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 24-31

    // Space and punctuation (32-47)
    0x00, // 32 ' ' space (no dots)
    0x16, // 33 '!' dots 2,3,5
    0x36, // 34 '"' dots 2,3,5,6
    0x3C, // 35 '#' dots 3,4,5,6 (number sign)
    0x12, // 36 '$' dots 2,5 (with prefix in full implementation)
    0x29, // 37 '%' dots 1,4,6 (with prefix)
    0x2F, // 38 '&' dots 1,2,3,4,6
    0x04, // 39 ''' dot 3
    0x23, // 40 '(' dots 1,2,6 (with prefix)
    0x1C, // 41 ')' dots 3,4,5 (with prefix)
    0x14, // 42 '*' dots 3,5 (with prefix)
    0x2C, // 43 '+' dots 3,4,6
    0x02, // 44 ',' dot 2
    0x24, // 45 '-' dots 3,6
    0x32, // 46 '.' dots 2,5,6
    0x0C, // 47 '/' dots 3,4 (with prefix)

    // Digits (48-57) - using number indicator + letters a-j
    // For simplicity, just the letter patterns (full implementation would add number prefix)
    0x1A, // 48 '0' dots 2,4,5 (j pattern)
    0x01, // 49 '1' dot 1 (a pattern)
    0x03, // 50 '2' dots 1,2 (b pattern)
    0x09, // 51 '3' dots 1,4 (c pattern)
    0x19, // 52 '4' dots 1,4,5 (d pattern)
    0x11, // 53 '5' dots 1,5 (e pattern)
    0x0B, // 54 '6' dots 1,2,4 (f pattern)
    0x1B, // 55 '7' dots 1,2,4,5 (g pattern)
    0x13, // 56 '8' dots 1,2,5 (h pattern)
    0x0A, // 57 '9' dots 2,4 (i pattern)

    // Punctuation (58-64)
    0x12, // 58 ':' dots 2,5
    0x06, // 59 ';' dots 2,3
    0x23, // 60 '<' dots 1,2,6 (with prefix)
    0x36, // 61 '=' dots 2,3,5,6 (with prefix)
    0x1C, // 62 '>' dots 3,4,5 (with prefix)
    0x26, // 63 '?' dots 2,3,6
    0x01, // 64 '@' dot 1 (with prefix)

    // Uppercase letters (65-90) - use capital indicator (dot 6) + lowercase
    // For 8-dot braille, uppercase can use dots 7 or pattern shift
    // Here we'll use base pattern + dot 7 (bit 6 = 0x40)
    0x41, // 65 'A' dots 1,7
    0x43, // 66 'B' dots 1,2,7
    0x49, // 67 'C' dots 1,4,7
    0x59, // 68 'D' dots 1,4,5,7
    0x51, // 69 'E' dots 1,5,7
    0x4B, // 70 'F' dots 1,2,4,7
    0x5B, // 71 'G' dots 1,2,4,5,7
    0x53, // 72 'H' dots 1,2,5,7
    0x4A, // 73 'I' dots 2,4,7
    0x5A, // 74 'J' dots 2,4,5,7
    0x45, // 75 'K' dots 1,3,7
    0x47, // 76 'L' dots 1,2,3,7
    0x4D, // 77 'M' dots 1,3,4,7
    0x5D, // 78 'N' dots 1,3,4,5,7
    0x55, // 79 'O' dots 1,3,5,7
    0x4F, // 80 'P' dots 1,2,3,4,7
    0x5F, // 81 'Q' dots 1,2,3,4,5,7
    0x57, // 82 'R' dots 1,2,3,5,7
    0x4E, // 83 'S' dots 2,3,4,7
    0x5E, // 84 'T' dots 2,3,4,5,7
    0x65, // 85 'U' dots 1,3,6,7
    0x67, // 86 'V' dots 1,2,3,6,7
    0x7A, // 87 'W' dots 2,4,5,6,7
    0x6D, // 88 'X' dots 1,3,4,6,7
    0x7D, // 89 'Y' dots 1,3,4,5,6,7
    0x75, // 90 'Z' dots 1,3,5,6,7

    // More punctuation (91-96)
    0x23, // 91 '[' dots 1,2,6 (with prefix)
    0x21, // 92 '\' dots 1,6 (with prefix)
    0x1C, // 93 ']' dots 3,4,5 (with prefix)
    0x23, // 94 '^' dots 1,2,6 (with prefix)
    0x24, // 95 '_' dots 3,6 (with prefix)
    0x22, // 96 '`' dots 2,6 (with prefix)

    // Lowercase letters (97-122)
    0x01, // 97  'a' dot 1
    0x03, // 98  'b' dots 1,2
    0x09, // 99  'c' dots 1,4
    0x19, // 100 'd' dots 1,4,5
    0x11, // 101 'e' dots 1,5
    0x0B, // 102 'f' dots 1,2,4
    0x1B, // 103 'g' dots 1,2,4,5
    0x13, // 104 'h' dots 1,2,5
    0x0A, // 105 'i' dots 2,4
    0x1A, // 106 'j' dots 2,4,5
    0x05, // 107 'k' dots 1,3
    0x07, // 108 'l' dots 1,2,3
    0x0D, // 109 'm' dots 1,3,4
    0x1D, // 110 'n' dots 1,3,4,5
    0x15, // 111 'o' dots 1,3,5
    0x0F, // 112 'p' dots 1,2,3,4
    0x1F, // 113 'q' dots 1,2,3,4,5
    0x17, // 114 'r' dots 1,2,3,5
    0x0E, // 115 's' dots 2,3,4
    0x1E, // 116 't' dots 2,3,4,5
    0x25, // 117 'u' dots 1,3,6
    0x27, // 118 'v' dots 1,2,3,6
    0x3A, // 119 'w' dots 2,4,5,6
    0x2D, // 120 'x' dots 1,3,4,6
    0x3D, // 121 'y' dots 1,3,4,5,6
    0x35, // 122 'z' dots 1,3,5,6

    // Final punctuation (123-127)
    0x23, // 123 '{' dots 1,2,6 (with prefix)
    0x33, // 124 '|' dots 1,2,5,6 (with prefix)
    0x1C, // 125 '}' dots 3,4,5 (with prefix)
    0x31, // 126 '~' dots 1,5,6 (with prefix)
    0xFF  // 127 DEL - undefined
};

BrailleConverter::BrailleConverter() : charCount(0) {
    memset(convertedChars, 0, sizeof(convertedChars));
}

BrailleChar BrailleConverter::convertChar(char c) {
    BrailleChar bc;
    bc.original = c;

    if (c == '\n' || c == '\r' || c == '\t') {
        bc.dotPattern = 0x00;
        bc.dotCount = 0;
        return bc;
    }

    if (c >= 0 && c < 128) {
        bc.dotPattern = pgm_read_byte(&CHAR_TO_PATTERN[(uint8_t)c]);
    } else {
        bc.dotPattern = 0xFF;
    }

    bc.dotCount = patternToDots(bc.dotPattern, bc.dots);
    return bc;
}

uint16_t BrailleConverter::convertText(const char* text) {
    charCount = 0;
    if (!text) return 0;

    uint16_t len = strlen(text);
    if (len > MAX_INPUT_LENGTH) len = MAX_INPUT_LENGTH;

    for (uint16_t i = 0; i < len; i++) {
        convertedChars[i] = convertChar(text[i]);
        charCount++;
    }

    return charCount;
}

BrailleChar BrailleConverter::getCharAt(uint16_t index) {
    if (index < charCount) return convertedChars[index];
    return BrailleChar();
}

uint16_t BrailleConverter::getCharCount() { return charCount; }

uint8_t BrailleConverter::getDotPattern(char c) {
    if (c >= 0 && c < 128) {
        return pgm_read_byte(&CHAR_TO_PATTERN[(uint8_t)c]);
    }
    return 0xFF;
}

uint8_t BrailleConverter::getDots(char c, uint8_t* dotsArray) {
    return patternToDots(getDotPattern(c), dotsArray);
}

uint8_t BrailleConverter::patternToDots(uint8_t pattern, uint8_t* dotsArray) {
    if (!dotsArray) return 0;

    if (pattern == 0xFF) {
        for (uint8_t i = 0; i < MAX_BRAILLE_DOTS; i++) {
            dotsArray[i] = i + 1;
        }
        return MAX_BRAILLE_DOTS;
    }

    uint8_t count = 0;
    for (uint8_t i = 0; i < MAX_BRAILLE_DOTS; i++) {
        if (pattern & (1 << i)) {
            dotsArray[count++] = i + 1;
        }
    }
    return count;
}

bool BrailleConverter::isUpperCase(char c) { return (c >= 'A' && c <= 'Z'); }
char BrailleConverter::toLowerCase(char c) {
    return isUpperCase(c) ? c + ('a' - 'A') : c;
}

namespace Braille {
uint8_t charToDots(char c, uint8_t* dotsArray) {
    BrailleConverter converter;
    return converter.getDots(c, dotsArray);
}

uint8_t charToPattern(char c) {
    BrailleConverter converter;
    return converter.getDotPattern(c);
}
}
