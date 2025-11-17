# BrailleConverter Arduino Library

A comprehensive Arduino library for converting ASCII text to 8-dot Braille patterns. This library enables Arduino-based Braille displays and tactile feedback systems without requiring an external computer running Python scripts.

## Features

- ✅ **8-Dot Braille Support** - Full support for 8-dot Braille cells (2x4 grid)
- ✅ **Complete ASCII Character Set** - Supports uppercase, lowercase, digits, punctuation, and special characters
- ✅ **On-Device Conversion** - All conversion happens directly on the Arduino
- ✅ **Easy to Use API** - Simple, intuitive functions for text conversion
- ✅ **Hardware Ready** - Designed for driving solenoids, actuators, or other tactile displays
- ✅ **Memory Efficient** - Optimized for Arduino's limited RAM
- ✅ **Multiple Examples** - Includes basic, interactive, display, and file-based examples

## 8-Dot Braille Layout

This library uses the standard 8-dot Braille cell layout:

```
1 • • 4
2 • • 5
3 • • 6
7 • • 8
```

Each dot is numbered 1-8, and characters are represented by which dots are raised.

## Installation

### Method 1: Arduino Library Manager (Recommended - if published)
1. Open Arduino IDE
2. Go to **Sketch → Include Library → Manage Libraries**
3. Search for "BrailleConverter"
4. Click "Install"

### Method 2: Manual Installation
1. Download this library folder
2. Copy the entire `arduino_library` folder to your Arduino libraries directory:
   - **Windows**: `Documents\Arduino\libraries\`
   - **macOS**: `~/Documents/Arduino/libraries/`
   - **Linux**: `~/Arduino/libraries/`
3. Rename the folder to `BrailleConverter`
4. Restart the Arduino IDE

### Method 3: ZIP Import
1. Zip the `arduino_library` folder
2. In Arduino IDE, go to **Sketch → Include Library → Add .ZIP Library**
3. Select the zip file
4. Restart the Arduino IDE

## Quick Start

### Basic Example

```cpp
#include <BrailleConverter.h>

BrailleConverter converter;

void setup() {
  Serial.begin(115200);
  
  // Convert a single character
  BrailleChar bc = converter.convertChar('A');
  
  Serial.print("Character: A");
  Serial.print("Raised dots: ");
  for (uint8_t i = 0; i < bc.dotCount; i++) {
    Serial.print(bc.dots[i]);
    if (i < bc.dotCount - 1) Serial.print(",");
  }
  Serial.println();
}

void loop() {
  // Nothing to do
}
```

### Converting Text

```cpp
#include <BrailleConverter.h>

BrailleConverter converter;

void setup() {
  Serial.begin(115200);
  
  // Convert a string
  const char* text = "Hello";
  uint16_t count = converter.convertText(text);
  
  // Access each converted character
  for (uint16_t i = 0; i < count; i++) {
    BrailleChar bc = converter.getCharAt(i);
    
    Serial.print(bc.original);
    Serial.print(" -> Dots: ");
    for (uint8_t j = 0; j < bc.dotCount; j++) {
      Serial.print(bc.dots[j]);
      if (j < bc.dotCount - 1) Serial.print(",");
    }
    Serial.println();
  }
}

void loop() {
  // Nothing to do
}
```

### Driving a Physical Display

```cpp
#include <BrailleConverter.h>

BrailleConverter converter;

// Pin configuration for 8-dot braille cell
const uint8_t DOT_PINS[8] = {2, 3, 4, 5, 6, 7, 8, 9};

void setup() {
  // Initialize pins
  for (uint8_t i = 0; i < 8; i++) {
    pinMode(DOT_PINS[i], OUTPUT);
    digitalWrite(DOT_PINS[i], LOW);
  }
  
  // Convert and display a character
  BrailleChar bc = converter.convertChar('A');
  displayPattern(bc.dotPattern);
}

void displayPattern(uint8_t pattern) {
  for (uint8_t i = 0; i < 8; i++) {
    digitalWrite(DOT_PINS[i], (pattern & (1 << i)) ? HIGH : LOW);
  }
}

void loop() {
  // Nothing to do
}
```

## API Reference

### Classes

#### `BrailleConverter`

Main converter class for text to Braille conversion.

**Methods:**

- `BrailleChar convertChar(char c)` - Convert a single character to Braille
- `uint16_t convertText(const char* text)` - Convert a text string (stored internally)
- `BrailleChar getCharAt(uint16_t index)` - Get converted character at index
- `uint16_t getCharCount()` - Get number of converted characters
- `uint8_t getDotPattern(char c)` - Get dot pattern byte for a character
- `uint8_t getDots(char c, uint8_t* dotsArray)` - Get array of raised dots
- `uint8_t patternToDots(uint8_t pattern, uint8_t* dotsArray)` - Convert pattern to dots array
- `void printChar(const BrailleChar& bc)` - Print character info to Serial
- `void printPattern(uint8_t pattern)` - Print visual dot pattern to Serial
- `void printConvertedText()` - Print all converted text

#### `BrailleChar`

Structure representing a single Braille character.

**Members:**

- `char original` - Original ASCII character
- `uint8_t dotPattern` - Bit pattern (bit 0 = dot 1, bit 7 = dot 8)
- `uint8_t dotCount` - Number of raised dots
- `uint8_t dots[8]` - Array of raised dot numbers (1-8)

### Namespace Functions

The `Braille` namespace provides convenience functions:

```cpp
// Quick conversion functions
uint8_t Braille::charToDots(char c, uint8_t* dotsArray);
uint8_t Braille::charToPattern(char c);
void Braille::printDotPattern(uint8_t pattern);
```

## Examples

The library includes several examples:

1. **BasicConversion** - Simple character and text conversion
2. **SerialInput** - Interactive mode with Serial Monitor
3. **BrailleDisplay** - Driving a physical 8-dot Braille display
4. **FileConversion** - Reading and converting text from SD card

To run an example:
1. Go to **File → Examples → BrailleConverter**
2. Select an example
3. Upload to your Arduino

## Dot Pattern Encoding

Characters are encoded as an 8-bit pattern where each bit represents a dot:

```
Bit 0 = Dot 1
Bit 1 = Dot 2
Bit 2 = Dot 3
Bit 3 = Dot 4
Bit 4 = Dot 5
Bit 5 = Dot 6
Bit 6 = Dot 7
Bit 7 = Dot 8
```

**Example:** The letter 'A' has dots 1 and 7 raised:
- Binary: `01000001`
- Hex: `0x41`

## Character Support

### Supported Characters

- ✅ Lowercase letters (a-z)
- ✅ Uppercase letters (A-Z)
- ✅ Digits (0-9)
- ✅ Common punctuation (. , ! ? ; : ' ")
- ✅ Special characters (@ # $ % & * + - / = < > [ ] { } _ | ~ ^ `)
- ✅ Whitespace (space, tab, newline)

### Uppercase Handling

Uppercase letters use dot 7 in addition to the lowercase pattern:
- `'a'` = dot 1 → `0x01`
- `'A'` = dots 1,7 → `0x41`

### Unknown Characters

Unknown characters (not in the ASCII table) return pattern `0xFF` (all dots raised).

## Hardware Integration

### Basic Wiring

For an 8-dot Braille cell:

```
Arduino Pin 2  → Dot 1 (solenoid/actuator)
Arduino Pin 3  → Dot 2
Arduino Pin 4  → Dot 3
Arduino Pin 5  → Dot 4
Arduino Pin 6  → Dot 5
Arduino Pin 7  → Dot 6
Arduino Pin 8  → Dot 7
Arduino Pin 9  → Dot 8
```

### Driver Considerations

- **Solenoids**: Use transistor or MOSFET drivers (Arduino pins can't drive solenoids directly)
- **Current**: Add flyback diodes for inductive loads
- **Power**: External power supply recommended for multiple solenoids
- **PWM**: Consider PWM for softer actuation

### Example Circuit

```
Arduino Pin → Resistor → MOSFET Gate
                       MOSFET Drain → Solenoid → +12V
                       MOSFET Source → GND
                       Flyback Diode across Solenoid
```

## Memory Considerations

- **RAM Usage**: Approximately 512 bytes for conversion buffer (stores up to 256 characters)
- **Flash Usage**: ~4KB for lookup tables and code
- **Stack Usage**: Minimal (< 100 bytes)

### For Low-Memory Boards

If memory is limited:
1. Reduce `MAX_INPUT_LENGTH` in the header file
2. Convert and display characters one at a time
3. Use streaming approach instead of buffering entire text

## Performance

- **Conversion Speed**: ~100,000 characters/second on Arduino Uno (16MHz)
- **Lookup Time**: O(1) constant time per character
- **Memory Access**: Direct table lookup, no computation required

## Troubleshooting

### Library Not Found
- Ensure the folder is named `BrailleConverter` (exact case)
- Restart Arduino IDE after installation
- Check the library is in the correct Arduino libraries folder

### Dots Not Raising
- Check pin connections
- Verify power supply for solenoids/actuators
- Test each pin individually with `digitalWrite()`
- Check driver circuit (transistors/MOSFETs)

### Serial Output Issues
- Ensure Serial.begin(115200) is called in setup()
- Match baud rate in Serial Monitor
- Wait for Serial to connect: `while (!Serial) {}`

### Memory Errors
- Reduce `MAX_INPUT_LENGTH` if needed
- Don't store large strings in RAM (use PROGMEM)
- Process text in chunks for large files

## Advanced Usage

### Custom Character Mapping

You can modify the lookup table in `BrailleConverter.cpp` to customize mappings:

```cpp
const uint8_t BrailleConverter::CHAR_TO_PATTERN[128] = {
  // Modify entries here
  0x01,  // 'a' = dot 1
  // ... etc
};
```

### Reading from SD Card

See the `FileConversion` example for reading text files from an SD card.

### Multiple Braille Cells

To control multiple cells, maintain separate pin arrays and call `displayPattern()` for each cell with different character data.

## Comparison with Python Version

### Advantages of Arduino Version
- ✅ No external computer required
- ✅ Lower latency (no serial communication delay)
- ✅ Standalone operation
- ✅ Real-time conversion
- ✅ Lower power consumption

### Trade-offs
- ⚠️ Limited memory (vs unlimited on computer)
- ⚠️ Text input still requires external source (SD card, Serial, etc.)
- ⚠️ Less flexible for complex text processing

## Contributing

Contributions are welcome! Please submit pull requests or open issues on the project repository.

## License

[Add your license information here]

## Authors

Senior Design Group 6 - EC463

## Version History

- **1.0.0** (2025-11-17)
  - Initial release
  - Full 8-dot Braille support
  - Complete ASCII character set
  - Example sketches included

## Support

For questions, issues, or suggestions:
- Open an issue on GitHub
- Check the examples for common use cases
- Refer to the API reference above

---

**Happy Braille Converting! 🤚**

