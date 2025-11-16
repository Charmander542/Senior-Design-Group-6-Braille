# Senior Design Group 6 - Braille Project

Welcome to the EC463 Senior Design Group 6 Braille project repository.

Main Google Doc before README migration: https://docs.google.com/document/d/1c1Lonarr0yz-ggmuZfPKdth9k53kwFiAMk2oVoeEsT8/edit?usp=sharing

## Project Structure

### 📁 `electrical/`
Contains hardware-related files including:
- **driver.cpp**: Driver code for hardware control
- **pcb/**: PCB design files
  - `conductors/`: Braille cell conductor board designs (KiCad files)
  - `controller/`: Braille cell controller board designs (KiCad files)
  - `devboard/`: Development board schematics

### 📚 `braille_converter/`
A Python library for converting text to 6-dot braille representation.

**Features:**
- ✨ Convert text to Unicode braille characters
- 📁 Convert entire text files to braille
- 🔍 Detailed character-by-character analysis
- 📊 Visual dot pattern representation
- 🎯 Full support for letters, numbers, punctuation, and symbols

**Quick Start:**
```python
from braille_converter import text_to_braille

text = "Hello World!"
braille = text_to_braille(text)
print(braille)  # Output: ⠠⠓⠑⠇⠇⠕⠀⠠⠺⠕⠗⠇⠙⠖
```

**Documentation:**
- [Full README](braille_converter/README.md) - Complete API documentation
- [Quick Start Guide](braille_converter/QUICKSTART.md) - Get started in 5 minutes
- [Examples](braille_converter/examples/) - Working code examples

**Try it now:**
```bash
cd braille_converter
python demo.py
```

### 🔌 `braille_converter/arduino_integration/`
Arduino integration for sending braille patterns to hardware over serial.

**Features:**
- 🔌 Serial communication with Arduino
- 📡 Send braille dot patterns to hardware
- 🎮 Interactive and command-line interfaces
- 🧪 Comprehensive test suite
- 📖 Full examples and documentation

**Quick Test:**
```bash
# Install PySerial
pip install pyserial

# Upload arduino_receiver.ino to your Arduino

# Run interactive mode
cd braille_converter/arduino_integration
python braille_to_arduino.py
```

**Documentation:**
- [Arduino Integration README](braille_converter/arduino_integration/README.md) - Complete guide
- Arduino sketch included (`arduino_receiver.ino`)
- 10+ working examples

## Getting Started

### For Software Development (Braille Converter)
```bash
# Install the library
pip install -e braille_converter

# Run the demo
cd braille_converter
python demo.py

# Run examples
cd braille_converter/examples
python basic_usage.py
python file_conversion.py
```

### For Arduino Integration
```bash
# Install PySerial
pip install pyserial

# 1. Upload arduino_receiver.ino to your Arduino using Arduino IDE

# 2. Find your Arduino port
cd braille_converter/arduino_integration
python braille_to_arduino.py --list

# 3. Run interactive mode
python braille_to_arduino.py

# 4. Or run test suite
python test_arduino.py

# 5. Or run examples
python examples.py
```

See [Arduino Integration README](braille_converter/arduino_integration/README.md) for detailed setup.

### For Hardware Development
Navigate to the `electrical/` directory to access:
- PCB design files (KiCad format)
- Hardware driver code
- Development board schematics

## Team

EC463 Senior Design - Group 6

## License

This project is part of the EC463 Senior Design course.
