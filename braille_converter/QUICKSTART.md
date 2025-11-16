# Quick Start Guide

Get started with the Braille Converter library in 5 minutes!

## Installation

```bash
pip install -e braille_converter
```

## Your First Conversion

### Option 1: Run the Demo

The fastest way to see the library in action:

```bash
cd braille_converter
python demo.py
```

This will show you:
- Simple text conversion
- Numbers and symbols
- Character breakdowns
- Visual dot patterns
- Complete alphabet
- Text statistics

### Option 2: Python Code

Create a file called `test.py`:

```python
from braille_converter import text_to_braille

# Convert text to braille
text = "Hello World!"
braille = text_to_braille(text)

print(f"Original: {text}")
print(f"Braille:  {braille}")
```

Run it:
```bash
python test.py
```

Output:
```
Original: Hello World!
Braille:  ⠠⠓⠑⠇⠇⠕⠀⠠⠺⠕⠗⠇⠙⠖
```

### Option 3: Command Line

Convert a text file directly from the command line:

```bash
# Create a test file
echo "Hello from the command line!" > test.txt

# Convert it
text-to-braille test.txt -o test_braille.txt

# View the result
cat test_braille.txt
```

## Common Use Cases

### 1. Convert a String

```python
from braille_converter import text_to_braille

braille = text_to_braille("Your text here")
print(braille)
```

### 2. Convert a File

```python
from braille_converter import file_to_braille

braille = file_to_braille('input.txt', 'output.txt')
```

### 3. Get Character Details

```python
from braille_converter import BrailleConverter

converter = BrailleConverter()
char = converter.convert_char('A')

print(f"Character: {char.original}")
print(f"Braille: {char.braille}")
print(f"Dots: {char.dots}")
print(f"Pattern:\n{char.get_pattern()}")
```

### 4. Analyze Text

```python
from braille_converter import BrailleConverter

converter = BrailleConverter()
analysis = converter.convert_and_analyze("Hello 123!")

print(f"Braille: {analysis['braille_text']}")
print(f"Stats: {analysis['statistics']}")
```

### 5. Process Multiple Files

```python
from braille_converter import BrailleConverter
from pathlib import Path

converter = BrailleConverter()

for txt_file in Path('input_dir').glob('*.txt'):
    output = Path('output_dir') / f"{txt_file.stem}_braille.txt"
    converter.convert_file(txt_file, output)
```

## Understanding the Output

The library uses Unicode Braille Patterns (U+2800 to U+283F). Each character is represented by a specific pattern of raised dots in a 2×3 grid:

```
1 • • 4
2 • • 5
3 • • 6
```

For example:
- `'a'` = ⠁ (dot 1)
- `'b'` = ⠃ (dots 1,2)
- `'A'` = ⠠⠁ (capital indicator + 'a')
- `'1'` = ⠼⠁ (number indicator + 'a')

## Command Line Tools

After installation, you have access to the `text-to-braille` command:

```bash
# Convert a file
text-to-braille input.txt -o output.txt

# Convert text directly
text-to-braille -t "Hello World"

# Show analysis
text-to-braille -t "ABC 123" --analyze

# Show dot patterns
text-to-braille -t "Hi" --pattern

# Get help
text-to-braille --help
```

## Next Steps

1. **Explore Examples**: Check out the `examples/` directory
   ```bash
   cd examples
   python basic_usage.py
   python file_conversion.py
   ```

2. **Read Full Documentation**: See `README.md` for complete API reference

3. **Try the Examples**: Experiment with different text inputs

4. **Integration**: Import the library into your own projects

## Troubleshooting

### Import Error

If you get `ModuleNotFoundError: No module named 'braille_converter'`:

```bash
# Make sure you're in the correct directory
cd path-to-this-project

# Install in development mode
pip install -e braille_converter
```

### Unicode Display Issues

If braille characters don't display correctly, ensure your terminal supports Unicode. Most modern terminals do, but you may need to:
- Use a terminal with UTF-8 support
- Install fonts that include Unicode Braille Patterns
- Try a different terminal emulator

### File Encoding Issues

If you encounter encoding errors with files:

```python
# Specify encoding explicitly
converter.convert_file('input.txt', encoding='utf-8')
```

## Support

For issues or questions:
- Check the full README.md
- Review examples in the `examples/` directory
- Check the source code in the package

## Quick Reference Card

```python
# Import
from braille_converter import text_to_braille, file_to_braille, BrailleConverter

# Simple conversion
braille = text_to_braille("text")

# File conversion
braille = file_to_braille("input.txt", "output.txt")

# Detailed conversion
converter = BrailleConverter()
chars = converter.convert_text_detailed("text")
analysis = converter.convert_and_analyze("text")
```

Happy converting! 🔤➡️⠃⠗⠁⠊⠇⠇⠑

