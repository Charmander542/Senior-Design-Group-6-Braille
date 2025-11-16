# Arduino Integration for Braille Converter

This directory contains scripts and examples for integrating the Python braille converter library with Arduino hardware.

## 📁 Files

- **`braille_to_arduino.py`** - Python script to send braille patterns to Arduino via serial
- **`arduino_receiver.ino`** - Arduino sketch to receive and display braille patterns
- **`test_arduino.py`** - Simple test script
- **`examples.py`** - Various usage examples

## 🔌 Hardware Setup

### Required Hardware
- Arduino board (Uno, Mega, Nano, etc.)
- USB cable
- 6 output pins for braille dots (can be LEDs, solenoids, motors, etc.)
- Optional: Shift register (74HC595) for multiple cells

### Wiring (Example)

For testing with LEDs:
```
Arduino Pin 2  → Dot 1 → LED + Resistor → GND
Arduino Pin 3  → Dot 2 → LED + Resistor → GND
Arduino Pin 4  → Dot 3 → LED + Resistor → GND
Arduino Pin 5  → Dot 4 → LED + Resistor → GND
Arduino Pin 6  → Dot 5 → LED + Resistor → GND
Arduino Pin 7  → Dot 6 → LED + Resistor → GND
```

Braille dot layout:
```
1 • • 4
2 • • 5
3 • • 6
```

## 💻 Software Setup

### Step 1: Install Python Dependencies

```bash
pip install pyserial
```

### Step 2: Upload Arduino Code

1. Open `arduino_receiver.ino` in Arduino IDE
2. Adjust pin numbers in the sketch if needed:
   ```cpp
   const int DOT_PINS[6] = {2, 3, 4, 5, 6, 7};
   ```
3. Connect your Arduino via USB
4. Select the correct board and port in Arduino IDE
5. Upload the sketch

### Step 3: Find Your Arduino Port

**On macOS/Linux:**
```bash
ls /dev/cu.* 
# or
ls /dev/tty.*
```

**On Windows:**
```bash
# Check Device Manager → Ports (COM & LPT)
# Usually COM3, COM4, etc.
```

**Using Python:**
```bash
cd arduino_integration
python braille_to_arduino.py --list
```

## 🚀 Quick Start

### Interactive Mode

```bash
cd path-to-this-project/braille_converter/arduino_integration
python braille_to_arduino.py
```

Then follow the prompts to:
1. Select your Arduino port
2. Send characters, text, or raw dot patterns

### Command-Line Mode

```bash
# Send text directly
python braille_to_arduino.py --port /dev/cu.usbmodem14201 "Hello"

# List available ports
python braille_to_arduino.py --list
```

### Python Script

```python
import sys
sys.path.insert(0, 'path-to-this-project')

from braille_converter.arduino_integration.braille_to_arduino import ArduinoBrailleInterface

# Connect to Arduino
interface = ArduinoBrailleInterface('/dev/cu.usbmodem14201')

# Send a single character
interface.send_char('A')

# Send text with delays
interface.send_text("Hello!", delay=1.0)

# Send raw dot pattern
interface.send_dots([1, 2, 3])  # Dots 1, 2, and 3

# Disconnect
interface.disconnect()
```

## 📖 Usage Examples

### Example 1: Send Single Character

```python
from braille_to_arduino import ArduinoBrailleInterface

interface = ArduinoBrailleInterface('/dev/cu.usbmodem14201')
interface.demo_character('A')
interface.disconnect()
```

Output:
```
============================================================
Character: 'A'
Braille:   ⠠⠁
Dots:      [6, 1]

Pattern:
● ○
○ ○
○ ●
============================================================
✓ Sent to Arduino
```

### Example 2: Send Word with Delays

```python
interface = ArduinoBrailleInterface('/dev/cu.usbmodem14201')
interface.send_text("Hi!", delay=1.5)
interface.disconnect()
```

### Example 3: Interactive Commands

In interactive mode:
```
> char A          # Send letter 'A'
> text Hello      # Send word "Hello" with delays
> demo B          # Show pattern for 'B' and send
> dots 1,2,3      # Send raw dot pattern
> quit            # Exit
```

### Example 4: Continuous Stream

```python
interface = ArduinoBrailleInterface('/dev/cu.usbmodem14201')
interface.send_text_continuous("Hello World", char_delay=2.0)
interface.disconnect()
```

### Example 5: File to Arduino

```python
import sys
sys.path.insert(0, 'path-to-this-project')

from braille_converter import BrailleConverter
from braille_converter.arduino_integration.braille_to_arduino import ArduinoBrailleInterface

# Read file
with open('message.txt', 'r') as f:
    text = f.read()

# Send to Arduino
interface = ArduinoBrailleInterface('/dev/cu.usbmodem14201')
interface.send_text(text, delay=1.0)
interface.disconnect()
```

## 🔧 Communication Protocol

### Python → Arduino

**Format:** `DOTS:<numbers>\n`

Examples:
- `DOTS:1,2,3\n` - Raise dots 1, 2, and 3
- `DOTS:NONE\n` - Clear all dots (space)
- `DOTS:1\n` - Raise only dot 1

### Arduino → Python

**Acknowledgments:**
- `READY:Arduino Braille Receiver` - On startup
- `ACK:DOTS:1,2,3` - Dots activated
- `ACK:CLEARED` - All dots cleared
- `ERROR:Unknown command` - Invalid message

## 🎯 Advanced Usage

### Custom Pin Configuration

Edit `arduino_receiver.ino`:
```cpp
// Change these to match your hardware
const int DOT_PINS[6] = {2, 3, 4, 5, 6, 7};
```

### Adjust Baud Rate

Both files must match:
```python
# In braille_to_arduino.py
ArduinoBrailleInterface(port, baud_rate=9600)
```

```cpp
// In arduino_receiver.ino
const int BAUD_RATE = 9600;
```

### Multiple Braille Cells

For multiple cells, you can:
1. Use more pins (48 pins for 8 cells)
2. Use shift registers (see commented code in .ino)
3. Use I2C port expanders

### PWM Control for Motors

For solenoids or motors, use PWM:
```cpp
// See commented PWM functions in arduino_receiver.ino
activateDotPWM(dotNum, intensity);
raiseDotSmoothly(dotNum, duration);
```

## 🧪 Testing

### Test 1: Basic Connection

```bash
python test_arduino.py
```

### Test 2: All Dots

In interactive mode:
```
> dots 1,2,3,4,5,6    # All dots on
> dots                # All dots off
```

### Test 3: Alphabet

```python
interface = ArduinoBrailleInterface(your_port)
for char in "ABCDEFGHIJKLMNOPQRSTUVWXYZ":
    interface.send_char(char, delay=1.0)
interface.disconnect()
```

### Test 4: Monitor Serial Output

Open Arduino Serial Monitor (115200 baud) to see:
- Incoming commands
- Acknowledgments
- Errors

## 🐛 Troubleshooting

### "No serial ports found"
- Check Arduino is connected via USB
- Try a different USB cable/port
- Install Arduino drivers

### "Permission denied" (macOS/Linux)
```bash
# Add user to dialout group (Linux)
sudo usermod -a -G dialout $USER

# Or use sudo (not recommended)
sudo python braille_to_arduino.py
```

### "Port already in use"
- Close Arduino IDE Serial Monitor
- Close other programs using the port
- Disconnect and reconnect Arduino

### Arduino not responding
- Check baud rate matches (115200)
- Press Arduino reset button
- Re-upload arduino_receiver.ino
- Check Serial Monitor for "READY" message

### Dots not activating
- Check pin numbers in Arduino code
- Test with LEDs first
- Verify wiring and connections
- Check if outputs need transistors/drivers

## 📊 Performance

- **Baud Rate**: 115200 (adjustable)
- **Character Delay**: 0.5-2.0 seconds (typical)
- **Latency**: ~50ms per character
- **Max Speed**: ~20 characters/second

## 🔄 Workflow

```
Text Input
    ↓
Python Braille Converter
    ↓
Dot Patterns (1-6)
    ↓
PySerial (USB)
    ↓
Arduino Serial
    ↓
Pin Control
    ↓
Braille Cell Display
```

## 📝 Example Session

```bash
$ python braille_to_arduino.py
============================================================
  Arduino Braille Interface - Interactive Mode
============================================================
Available serial ports:
  [0] /dev/cu.usbmodem14201 - Arduino Uno
  [1] /dev/cu.Bluetooth-Incoming-Port - Bluetooth

Select port number: 0
✓ Connected to Arduino on /dev/cu.usbmodem14201

============================================================
Commands:
  char <c>  - Send single character
  text <t>  - Send text with delays
  demo <c>  - Demo character with pattern
  dots <d>  - Send raw dot pattern (e.g., 1,2,3)
  quit      - Exit
============================================================

> demo H
============================================================
Character: 'H'
Braille:   ⠠⠓
Dots:      [5]

Pattern:
○ ○
○ ●
○ ○
============================================================
✓ Sent to Arduino

> text Hi!

Sending text: 'Hi!'
============================================================
[1/3] Sending 'H' → ⠠⠓ (dots: [5])
[2/3] Sending 'i' → ⠊ (dots: [2, 6])
[3/3] Sending '!' → ⠖ (dots: [2, 3, 4])

✓ Text sent successfully

> quit
✓ Disconnected from Arduino
```

## 🎓 Next Steps

1. ✅ Upload Arduino sketch
2. ✅ Install PySerial
3. ✅ Test connection with `--list`
4. ✅ Run interactive mode
5. ✅ Try sending your first character
6. ✅ Integrate with your braille hardware
7. ✅ Build your application!

## 📚 Additional Resources

- [PySerial Documentation](https://pyserial.readthedocs.io/)
- [Arduino Serial Reference](https://www.arduino.cc/reference/en/language/functions/communication/serial/)
- [Braille Converter README](../README.md)

---

**Ready to test your braille display!** 🎉

Start with:
```bash
python braille_to_arduino.py
```

