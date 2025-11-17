# BrailleConverter Arduino Library - Installation Guide

This guide will help you install and set up the BrailleConverter Arduino library.

## Prerequisites

- Arduino IDE 1.8.0 or later (or Arduino IDE 2.x)
- Arduino board (Uno, Mega, Nano, ESP32, etc.)
- USB cable for programming
- (Optional) Physical Braille display hardware

## Installation Methods

### Method 1: Manual Installation (Recommended)

This is the most straightforward method for a custom library.

#### Step 1: Locate Your Arduino Libraries Folder

The Arduino libraries folder location depends on your operating system:

- **Windows**: `C:\Users\[YourUsername]\Documents\Arduino\libraries\`
- **macOS**: `/Users/[YourUsername]/Documents/Arduino/libraries/`
- **Linux**: `/home/[YourUsername]/Arduino/libraries/`

If the `libraries` folder doesn't exist, create it.

#### Step 2: Copy the Library

1. Navigate to this directory: `braille_converter/arduino_library/`
2. Copy the entire folder
3. Paste it into your Arduino libraries folder
4. Rename it to `BrailleConverter` (if not already named that)

Your folder structure should look like:

```
Arduino/
└── libraries/
    └── BrailleConverter/
        ├── BrailleConverter.h
        ├── BrailleConverter.cpp
        ├── library.properties
        ├── keywords.txt
        ├── README.md
        └── examples/
            ├── BasicConversion/
            ├── SerialInput/
            ├── BrailleDisplay/
            └── FileConversion/
```

#### Step 3: Restart Arduino IDE

Close and reopen the Arduino IDE for it to recognize the new library.

#### Step 4: Verify Installation

1. Open Arduino IDE
2. Go to **File → Examples**
3. Scroll down to find **BrailleConverter**
4. You should see the example sketches listed

### Method 2: ZIP Import

If you prefer using Arduino IDE's built-in import feature:

#### Step 1: Create a ZIP File

1. Navigate to the `arduino_library` folder
2. Compress the entire folder into a ZIP file
   - **Windows**: Right-click → Send to → Compressed (zipped) folder
   - **macOS**: Right-click → Compress
   - **Linux**: `zip -r BrailleConverter.zip arduino_library/`

#### Step 2: Import to Arduino IDE

1. Open Arduino IDE
2. Go to **Sketch → Include Library → Add .ZIP Library...**
3. Select the ZIP file you created
4. Wait for the import to complete
5. You should see a confirmation message

#### Step 3: Verify Installation

Same as Method 1, Step 4.

## Testing the Installation

### Quick Test

1. Open Arduino IDE
2. Go to **File → Examples → BrailleConverter → BasicConversion**
3. Connect your Arduino board
4. Select your board: **Tools → Board**
5. Select your port: **Tools → Port**
6. Click the **Upload** button (→)
7. Open Serial Monitor: **Tools → Serial Monitor**
8. Set baud rate to **115200**
9. You should see Braille conversion output

### Expected Output

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

...
```

## Hardware Setup (Optional)

If you want to connect a physical Braille display:

### Required Components

- Arduino board (Uno, Mega, or similar)
- 8x solenoids or linear actuators (5V or 12V)
- 8x MOSFET transistors (e.g., IRLZ44N)
- 8x flyback diodes (e.g., 1N4007)
- 8x resistors (10kΩ)
- External power supply (voltage matching your solenoids)
- Breadboard and jumper wires

### Wiring Diagram

For each of the 8 dots:

```
Arduino Pin (2-9) ──[10kΩ]── MOSFET Gate
                              │
                              ├── MOSFET Drain ── Solenoid ── +12V
                              │                      │
                              └── MOSFET Source ───GND
                                                     │
                                   [Diode ↓] ────────┘
```

### Pin Mapping

Default pin configuration (can be changed in your code):

| Dot | Arduino Pin |
|-----|-------------|
| 1   | 2           |
| 2   | 3           |
| 3   | 4           |
| 4   | 5           |
| 5   | 6           |
| 6   | 7           |
| 7   | 8           |
| 8   | 9           |

### Testing Hardware

1. Upload the **BrailleDisplay** example
2. Connect solenoids to the appropriate pins
3. The example will test each dot individually
4. Then display the word "Hello" character by character

## Troubleshooting

### Library Not Showing Up

**Problem**: Library doesn't appear in Examples menu

**Solutions**:
1. Ensure folder is named exactly `BrailleConverter` (case-sensitive)
2. Check folder is in correct libraries directory
3. Restart Arduino IDE completely
4. Verify folder structure matches the layout above
5. Check library.properties file exists

### Compilation Errors

**Problem**: Errors when compiling/uploading

**Error: "BrailleConverter.h: No such file or directory"**
- Library not installed correctly
- Re-install using Method 1

**Error: "Multiple libraries found"**
- You may have duplicate copies
- Remove old versions from libraries folder

### Serial Monitor Issues

**Problem**: No output or garbled output

**Solutions**:
1. Set baud rate to 115200 in Serial Monitor
2. Add `while (!Serial) {}` in setup() for boards with native USB
3. Check USB cable and connection
4. Try a different USB port

### Memory Issues

**Problem**: "Low memory available" or crashes

**Solutions**:
1. Use a board with more RAM (Mega, ESP32)
2. Reduce `MAX_INPUT_LENGTH` in BrailleConverter.h
3. Process text in smaller chunks
4. Don't store large strings in RAM

## Platform-Specific Notes

### Arduino Uno/Nano
- ✅ Fully supported
- ⚠️ Limited RAM (2KB) - reduce MAX_INPUT_LENGTH if needed
- ⚠️ Limited pins - use shift registers for more cells

### Arduino Mega
- ✅ Fully supported
- ✅ More RAM (8KB) - can handle larger text
- ✅ More pins - can control multiple cells directly

### ESP32
- ✅ Fully supported
- ✅ Lots of RAM (520KB) - no memory concerns
- ✅ WiFi/Bluetooth - can receive text wirelessly
- ⚠️ 3.3V logic - use level shifters for 5V devices

### ESP8266
- ✅ Mostly supported
- ⚠️ Some pins have restrictions
- ✅ WiFi - can receive text over network

### Arduino Due
- ✅ Fully supported
- ⚠️ 3.3V logic - use level shifters
- ✅ More RAM and speed

## Next Steps

1. **Try the Examples**
   - Start with BasicConversion
   - Move to SerialInput for interactive testing
   - Try BrailleDisplay if you have hardware

2. **Read the Documentation**
   - Check README.md for API reference
   - Review example code for usage patterns

3. **Build Your Project**
   - Adapt BrailleDisplay example for your hardware
   - Integrate with SD card reader for file input
   - Add wireless text input via WiFi/Bluetooth

4. **Customize**
   - Modify pin assignments
   - Adjust timing for your actuators
   - Add additional features

## Getting Help

If you encounter issues:

1. **Check Examples**: Often show the solution
2. **Read Error Messages**: They usually indicate the problem
3. **Verify Wiring**: Most issues are hardware-related
4. **Test Incrementally**: Start simple, add complexity gradually

## Additional Resources

- Arduino Language Reference: https://www.arduino.cc/reference/
- Arduino Forums: https://forum.arduino.cc/
- Example Sketches: See the `examples/` folder

---

**Installation Complete! Ready to convert text to Braille on Arduino! 🎉**

