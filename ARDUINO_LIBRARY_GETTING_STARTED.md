# Getting Started with the Arduino Braille Converter Library

**Quick guide to start using the new Arduino C++ Braille converter on your hardware!**

## What's New?

You can now convert text to 8-dot Braille **directly on the Arduino** without needing a Python script running on a computer! This means your Braille display can be completely standalone.

## 🚀 Quick Start (5 minutes)

### Step 1: Install the Library

Copy the library to your Arduino libraries folder:

**macOS/Linux:**
```bash
cp -r braille_converter/arduino_library ~/Documents/Arduino/libraries/BrailleConverter
```

**Windows:**
```cmd
xcopy braille_converter\arduino_library "%USERPROFILE%\Documents\Arduino\libraries\BrailleConverter" /E /I
```

**Or manually:**
1. Navigate to `braille_converter/arduino_library/`
2. Copy the entire folder
3. Paste into `Documents/Arduino/libraries/`
4. Rename to `BrailleConverter`

### Step 2: Restart Arduino IDE

Close and reopen Arduino IDE so it recognizes the new library.

### Step 3: Try the Basic Example

1. Open Arduino IDE
2. Go to **File → Examples → BrailleConverter → BasicConversion**
3. Connect your Arduino
4. Select your board: **Tools → Board**
5. Select your port: **Tools → Port**
6. Click Upload (→)
7. Open Serial Monitor (magnifying glass icon)
8. Set baud rate to **115200**

### Step 4: See the Results!

You should see output like:

```
========================================
  BrailleConverter Basic Example
========================================

Example 1: Single Character
----------------------------
Character: 'A'
Dot Pattern (hex): 0x41
Raised Dots: 1, 7

Braille Pattern:
 *  o 
 o  o 
 o  o 
 *  o 

Example 2: Convert Word
------------------------
Converted 5 character(s):
[0] Char: 'H' -> Pattern: 0x53 -> Dots: [1,2,5,7]
[1] Char: 'e' -> Pattern: 0x11 -> Dots: [1,5]
[2] Char: 'l' -> Pattern: 0x07 -> Dots: [1,2,3]
[3] Char: 'l' -> Pattern: 0x07 -> Dots: [1,2,3]
[4] Char: 'o' -> Pattern: 0x15 -> Dots: [1,3,5]
...
```

**Congratulations!** 🎉 The library is working!

## 📖 Understanding the Output

### 8-Dot Braille Layout

The library uses 8-dot Braille (2 columns, 4 rows):

```
1 • • 4
2 • • 5
3 • • 6
7 • • 8
```

### Example: Letter 'A'

**Visual Pattern:**
```
 *  o     <- Dot 1 raised, Dot 4 down
 o  o     <- Dots 2 and 5 down
 o  o     <- Dots 3 and 6 down
 *  o     <- Dot 7 raised, Dot 8 down
```

**Dot Numbers:** [1, 7]  
**Hex Pattern:** 0x41 (binary: 01000001)
- Bit 0 = 1 (dot 1 raised)
- Bit 6 = 1 (dot 7 raised)

## 🔌 Connecting Hardware

### Basic Setup (8 Solenoids)

You'll need to connect solenoids or actuators to represent the 8 dots. **Important:** Arduino pins can't drive solenoids directly - you need driver circuits!

### Circuit for Each Dot

```
Arduino Pin → 10kΩ Resistor → MOSFET Gate
                                 │
                                 ├→ MOSFET Drain → Solenoid → +12V
                                 │                    │
                                 └→ MOSFET Source → GND
                                                      │
                                    [Diode ↓] ────────┘
                                    (1N4007)
```

### Components per Dot
- 1x N-channel MOSFET (e.g., IRLZ44N, IRL540N)
- 1x 10kΩ resistor (gate pull-down)
- 1x 1N4007 diode (flyback protection)
- 1x Solenoid (5V or 12V)

### Pin Mapping (Default)

| Dot Number | Arduino Pin | Function |
|------------|-------------|----------|
| Dot 1 | Pin 2 | Left column, row 1 |
| Dot 2 | Pin 3 | Left column, row 2 |
| Dot 3 | Pin 4 | Left column, row 3 |
| Dot 4 | Pin 5 | Right column, row 1 |
| Dot 5 | Pin 6 | Right column, row 2 |
| Dot 6 | Pin 7 | Right column, row 3 |
| Dot 7 | Pin 8 | Left column, row 4 |
| Dot 8 | Pin 9 | Right column, row 4 |

## 💻 Your First Program

Create a new sketch that displays a character on your Braille hardware:

```cpp
#include <BrailleConverter.h>

BrailleConverter converter;

// Pin configuration
const uint8_t DOT_PINS[8] = {2, 3, 4, 5, 6, 7, 8, 9};

void setup() {
  Serial.begin(115200);
  
  // Initialize pins
  for (uint8_t i = 0; i < 8; i++) {
    pinMode(DOT_PINS[i], OUTPUT);
    digitalWrite(DOT_PINS[i], LOW);
  }
  
  Serial.println("Braille Display Ready!");
  
  // Display the letter 'A'
  displayChar('A');
}

void loop() {
  // Display cycles through "HELLO"
  const char* message = "HELLO";
  
  for (uint8_t i = 0; i < 5; i++) {
    displayChar(message[i]);
    delay(2000);  // 2 seconds per character
    clearDisplay();
    delay(500);   // 0.5 seconds between characters
  }
}

// Function to display a character
void displayChar(char c) {
  BrailleChar bc = converter.convertChar(c);
  
  Serial.print("Displaying: ");
  Serial.print(c);
  Serial.print(" - Dots: ");
  
  for (uint8_t i = 0; i < bc.dotCount; i++) {
    Serial.print(bc.dots[i]);
    if (i < bc.dotCount - 1) Serial.print(",");
  }
  Serial.println();
  
  // Set each pin based on the dot pattern
  for (uint8_t i = 0; i < 8; i++) {
    bool raised = bc.dotPattern & (1 << i);
    digitalWrite(DOT_PINS[i], raised ? HIGH : LOW);
  }
}

// Function to clear display
void clearDisplay() {
  for (uint8_t i = 0; i < 8; i++) {
    digitalWrite(DOT_PINS[i], LOW);
  }
}
```

## 🎮 Interactive Mode

Try the SerialInput example to type text and see it converted in real-time:

1. Open **File → Examples → BrailleConverter → SerialInput**
2. Upload to Arduino
3. Open Serial Monitor
4. Type any text and press Enter
5. See the Braille conversion!

**Commands:**
- `help` - Show available commands
- `demo` - Run demonstration
- Type any text to convert it

## 📁 Reading from SD Card

If you want to read text files from an SD card:

1. Connect SD card module to Arduino:
   - CS → Pin 10
   - SCK → Pin 13
   - MOSI → Pin 11
   - MISO → Pin 12
   - VCC → 5V
   - GND → GND

2. Create a file `input.txt` on the SD card

3. Open **File → Examples → BrailleConverter → FileConversion**

4. Upload and watch it convert the file!

## 🔧 Common Patterns

### Convert Single Character

```cpp
BrailleChar bc = converter.convertChar('A');
// Now use bc.dots[] or bc.dotPattern
```

### Convert String

```cpp
const char* text = "Hello World";
uint16_t count = converter.convertText(text);

for (uint16_t i = 0; i < count; i++) {
  BrailleChar bc = converter.getCharAt(i);
  displayChar(bc);
  delay(2000);
}
```

### Get Just the Dot Pattern

```cpp
uint8_t pattern = converter.getDotPattern('B');
// pattern = 0x43 (dots 1,2,7)
```

### Get Dots as Array

```cpp
uint8_t dots[8];
uint8_t count = converter.getDots('C', dots);
// count = 3
// dots = {1, 4, 7}
```

### Check if Specific Dot is Raised

```cpp
uint8_t pattern = converter.getDotPattern('A');
bool dot1 = pattern & (1 << 0);  // Check dot 1
bool dot7 = pattern & (1 << 6);  // Check dot 7
```

## 📚 API Reference Quick Guide

### Main Functions

| Function | What It Does | Returns |
|----------|-------------|---------|
| `convertChar(c)` | Convert one character | `BrailleChar` struct |
| `convertText(text)` | Convert string (stored internally) | Number of characters |
| `getCharAt(i)` | Get character at index | `BrailleChar` struct |
| `getDotPattern(c)` | Get bit pattern for character | `uint8_t` (0-255) |
| `getDots(c, array)` | Fill array with raised dots | Number of dots |

### BrailleChar Structure

```cpp
struct BrailleChar {
  char original;          // Original character (e.g., 'A')
  uint8_t dotPattern;     // Bit pattern (e.g., 0x41)
  uint8_t dotCount;       // Number of raised dots (e.g., 2)
  uint8_t dots[8];        // Array of dot numbers (e.g., {1, 7})
};
```

## ⚠️ Common Issues

### 1. Library Not Found

**Error:** `BrailleConverter.h: No such file or directory`

**Fix:**
- Make sure folder is named exactly `BrailleConverter`
- Check it's in the right libraries folder
- Restart Arduino IDE

### 2. No Serial Output

**Fix:**
- Set baud rate to 115200 in Serial Monitor
- Add `while (!Serial) {}` after `Serial.begin()`

### 3. Solenoids Not Working

**Fix:**
- Check wiring (especially MOSFET connections)
- Test pins with LED first
- Verify external power supply voltage
- Check flyback diodes polarity

### 4. Out of Memory

**Fix:**
- Use Arduino Mega (8KB RAM vs Uno's 2KB)
- Reduce `MAX_INPUT_LENGTH` in BrailleConverter.h
- Process text in smaller chunks

## 📖 More Resources

- **Full API Documentation:** [README.md](braille_converter/arduino_library/README.md)
- **Installation Guide:** [INSTALLATION.md](braille_converter/arduino_library/INSTALLATION.md)
- **Quick Reference:** [QUICK_REFERENCE.md](braille_converter/arduino_library/QUICK_REFERENCE.md)
- **Conversion Details:** [ARDUINO_CONVERSION_SUMMARY.md](braille_converter/arduino_library/ARDUINO_CONVERSION_SUMMARY.md)

## 🎯 Next Steps

1. ✅ Install library (done!)
2. ✅ Try BasicConversion example (done!)
3. 🔲 Connect hardware (solenoids/actuators)
4. 🔲 Try BrailleDisplay example with hardware
5. 🔲 Build your custom Braille display application!

## 💡 Project Ideas

- **Scrolling Braille Display**: Show text character by character
- **WiFi Braille Display**: Receive text over WiFi (ESP32)
- **Braille Clock**: Display time in Braille
- **Braille Notifications**: Display incoming messages
- **Educational Tool**: Learn Braille interactively
- **Accessible Interface**: Make devices accessible for blind users

## 🆘 Need Help?

1. Check the examples - they cover most use cases
2. Read the error messages carefully
3. Test hardware incrementally (one dot at a time)
4. Use Serial.print() to debug

---

**You're all set! Start building amazing Braille projects! 🚀**

*Created by Senior Design Group 6 - EC463*

