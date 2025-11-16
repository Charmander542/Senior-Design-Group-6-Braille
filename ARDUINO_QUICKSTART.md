# Arduino Quick Start Guide

Get your Python braille converter working with Arduino in 10 minutes!

## 🎯 What You'll Do

1. Upload Arduino code to your board
2. Install Python dependencies
3. Connect and test the system
4. Send your first braille patterns!

## 📦 What You Need

### Hardware
- ✅ Arduino board (Uno, Mega, Nano, etc.)
- ✅ USB cable
- ✅ 6 LEDs + resistors (for testing) OR your braille actuators

### Software
- ✅ Arduino IDE
- ✅ Python 3.8+
- ✅ This repository

## 🚀 Step-by-Step Setup

### Step 1: Wire Your Arduino (5 minutes)

For testing with LEDs:

```
Arduino Pin 2  →  LED (dot 1)  →  220Ω resistor  →  GND
Arduino Pin 3  →  LED (dot 2)  →  220Ω resistor  →  GND
Arduino Pin 4  →  LED (dot 3)  →  220Ω resistor  →  GND
Arduino Pin 5  →  LED (dot 4)  →  220Ω resistor  →  GND
Arduino Pin 6  →  LED (dot 5)  →  220Ω resistor  →  GND
Arduino Pin 7  →  LED (dot 6)  →  220Ω resistor  →  GND
```

Braille dot layout:
```
1 • • 4
2 • • 5
3 • • 6
```

### Step 2: Upload Arduino Sketch (2 minutes)

1. Open Arduino IDE
2. File → Open → Navigate to:
   ```
   braille_converter/arduino_integration/arduino_receiver.ino
   ```
3. Select your board: Tools → Board → Arduino Uno (or your board)
4. Select your port: Tools → Port → /dev/cu.usbmodem* (or COMx on Windows)
5. Click Upload (→) button
6. Wait for "Done uploading" message

### Step 3: Install Python Dependency (1 minute)

```bash
pip install pyserial
```

### Step 4: Find Your Arduino Port (1 minute)

```bash
python braille_to_arduino.py --list
```

You'll see something like:
```
Available serial ports:
  [0] /dev/cu.usbmodem14201 - Arduino Uno
```

Note the port name (e.g., `/dev/cu.usbmodem14201` on Mac, `COM3` on Windows)

### Step 5: Test Connection (1 minute)

Run the test suite:

```bash
python test_arduino.py
```

This will:
1. ✅ List available ports
2. ✅ Connect to Arduino
3. ✅ Test individual dots
4. ✅ Test multiple dot patterns
5. ✅ Test character conversion
6. ✅ Send a complete word

## 🎮 Try Interactive Mode

```bash
python braille_to_arduino.py
```

Select your port, then try these commands:

```
> char A          # Send letter 'A'
> text Hello      # Send word "Hello"
> demo B          # Show pattern for 'B'
> dots 1,2,3      # Send raw dot pattern
```

## 📝 Quick Python Test

Create a file `test_my_arduino.py`:

```python
import sys
sys.path.insert(0, 'path-to-this-project')

from braille_converter.arduino_integration import ArduinoBrailleInterface

# Change this to your port!
interface = ArduinoBrailleInterface('/dev/cu.usbmodem14201')

# Send 'Hi!'
interface.send_text("Hi!", delay=1.5)

# Disconnect
interface.disconnect()

print("✓ Success!")
```

Run it:
```bash
python test_my_arduino.py
```

## 🎯 What Should Happen

When you send characters:

**Example: Letter 'A'**
- Python converts 'A' to braille: ⠠⠁
- Sends dot pattern: [6, 1]
- Arduino activates pins 2 and 7
- LEDs 1 and 6 light up!

**Example: Letter 'H'**
- Braille: ⠠⠓
- Dots: [5]
- Pin 6 activates
- LED 5 lights up!

## 🐛 Troubleshooting

### "No serial ports found"
- ✅ Check USB cable is connected
- ✅ Try different USB port
- ✅ Restart Arduino

### "Permission denied" (Mac/Linux)
```bash
# Add your user to dialout group (Linux)
sudo usermod -a -G dialout $USER

# Or run with sudo (temporary)
sudo python test_arduino.py
```

### "Port already in use"
- ✅ Close Arduino IDE Serial Monitor
- ✅ Close other programs using the port
- ✅ Unplug and replug Arduino

### LEDs not lighting up
- ✅ Check wiring (LED polarity matters!)
- ✅ Check pin numbers in Arduino code
- ✅ Open Arduino Serial Monitor (115200 baud) to see messages
- ✅ Try sending `dots 1,2,3,4,5,6` to light all LEDs

### Nothing happens
- ✅ Verify Arduino sketch uploaded successfully
- ✅ Check "Done uploading" message in Arduino IDE
- ✅ Try pressing Arduino reset button
- ✅ Check baud rate is 115200 in both Arduino and Python code

## 📊 Understanding the Protocol

### Python sends:
```
DOTS:1,2,3\n      → Raise dots 1, 2, 3
DOTS:NONE\n       → Clear all dots (space)
DOTS:5\n          → Raise only dot 5
```

### Arduino responds:
```
READY:Arduino Braille Receiver    (on startup)
ACK:DOTS:1,2,3                   (dots activated)
ACK:CLEARED                      (dots cleared)
```

## 🎓 Next Steps

1. ✅ **Test complete?** Great! Try the examples:
   ```bash
   python examples.py
   ```

2. 📚 **Read full documentation:**
   ```bash
   cat arduino_integration/README.md
   ```

3. 🔨 **Modify for your hardware:**
   - Edit pin numbers in `arduino_receiver.ino`
   - Add motor/solenoid control code
   - Implement multiple braille cells

4. 🚀 **Build your application:**
   - Use `ArduinoBrailleInterface` in your code
   - Convert text files to braille
   - Create interactive braille displays

## 📖 Example Session

```bash
$ python braille_to_arduino.py
============================================================
  Arduino Braille Interface - Interactive Mode
============================================================
Available serial ports:
  [0] /dev/cu.usbmodem14201 - Arduino Uno

Select port number: 0
✓ Connected to Arduino on /dev/cu.usbmodem14201

Commands:
  char <c>  - Send single character
  text <t>  - Send text with delays
  demo <c>  - Demo character with pattern
  dots <d>  - Send raw dot pattern (e.g., 1,2,3)
  quit      - Exit

> demo A
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

## 🎉 Success!

Your Arduino is now receiving braille patterns from Python!

**What you can do now:**
- ✅ Send any text to your Arduino
- ✅ Control braille display hardware
- ✅ Build interactive applications
- ✅ Process text files to braille
- ✅ Create accessible interfaces

## 📞 Need Help?

Check these resources:
- [Arduino Integration README](braille_converter/arduino_integration/README.md)
- [Braille Converter README](braille_converter/README.md)
- [Examples](braille_converter/arduino_integration/examples.py)
- [Test Suite](braille_converter/arduino_integration/test_arduino.py)

---

**Happy building!** 🚀

Got it working? Try: `python examples.py` to see 10 different examples!

