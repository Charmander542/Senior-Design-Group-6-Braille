# Braille Converter Library

A Python library for converting text to 6-dot braille representation following standard English Braille (Grade 1).

## Features

- ✨ Convert text to Unicode braille characters
- 📁 Convert entire text files to braille
- 🔍 Detailed character-by-character analysis
- 📊 Visual dot pattern representation
- 🎯 Support for letters, numbers, punctuation, and special characters
- 🌐 Full Unicode braille support (U+2800 to U+283F)

## Installation

### From Source

```bash
cd path-to-this-project
pip install -e braille_converter
```

## Quick Start

### Basic Text Conversion

```python
from braille_converter import text_to_braille

# Convert simple text
text = "Hello World!"
braille = text_to_braille(text)
print(braille)  # Output: ⠠⠓⠑⠇⠇⠕⠀⠠⠺⠕⠗⠇⠙⠖
```

### Convert a File

```python
from braille_converter import file_to_braille

# Convert a text file to braille
braille_text = file_to_braille('input.txt', 'output_braille.txt')
print(braille_text)
```

### Detailed Character Information

```python
from braille_converter import BrailleConverter

converter = BrailleConverter()

# Get detailed information about each character
text = "Hi!"
braille_chars = converter.convert_text_detailed(text)

for bc in braille_chars:
    print(f"Character: {bc.original}")
    print(f"Braille: {bc.braille}")
    print(f"Dots: {bc.dots}")
    print(f"Pattern:\n{bc.get_pattern()}\n")
```

Output:
```
Character: H
Braille: ⠠⠓
Dots: [6, 1, 2, 5]
Pattern:
● ○
● ●
○ ●

Character: i
Braille: ⠊
Dots: [2, 4]
Pattern:
○ ●
● ○
○ ○

Character: !
Braille: ⠖
Dots: [2, 3, 5]
Pattern:
○ ○
● ●
● ○
```

### Full Analysis

```python
from braille_converter import BrailleConverter

converter = BrailleConverter()
analysis = converter.convert_and_analyze("Hello 123!")

print(f"Original: {analysis['original_text']}")
print(f"Braille: {analysis['braille_text']}")
print(f"Statistics: {analysis['statistics']}")
```

## Understanding 6-Dot Braille

The 6-dot braille system uses a 2×3 grid of dots:

```
1 • • 4
2 • • 5
3 • • 6
```

Each character is represented by which dots are raised. For example:
- Letter 'a': dot 1 (⠁)
- Letter 'b': dots 1,2 (⠃)
- Letter 'c': dots 1,4 (⠉)

### Capital Letters
Capital letters are indicated by prefixing the letter with a capital indicator (dot 6: ⠠).
- 'A' = ⠠⠁ (capital indicator + 'a')

### Numbers
Numbers are indicated by prefixing with a number indicator (⠼) followed by letters a-j:
- '1' = ⠼⠁ (number indicator + 'a')
- '0' = ⠼⠚ (number indicator + 'j')

## API Reference

### Functions

#### `text_to_braille(text: str, preserve_newlines: bool = True) -> str`
Convert text string to braille.

**Parameters:**
- `text`: The text to convert
- `preserve_newlines`: Whether to preserve newline characters (default: True)

**Returns:** Braille string

#### `file_to_braille(input_path, output_path=None, encoding='utf-8') -> str`
Convert a text file to braille.

**Parameters:**
- `input_path`: Path to input file
- `output_path`: Optional output file path
- `encoding`: File encoding (default: 'utf-8')

**Returns:** Braille text

### Classes

#### `BrailleChar`
Represents a single character in braille.

**Attributes:**
- `original`: Original text character
- `braille`: Braille representation
- `dots`: List of raised dot numbers (1-6)

**Methods:**
- `get_pattern()`: Returns visual dot pattern
- `to_dict()`: Returns dictionary representation

#### `BrailleConverter`
Main converter class with advanced features.

**Methods:**
- `convert_char(char)`: Convert single character
- `convert_text(text, preserve_newlines)`: Convert text string
- `convert_text_detailed(text)`: Convert with detailed info
- `convert_file(input_path, output_path, encoding)`: Convert file
- `convert_and_analyze(text)`: Convert with statistics

## Examples

### Example 1: Simple Conversion

```python
from braille_converter import text_to_braille

message = "Python is awesome!"
braille = text_to_braille(message)
print(f"Original: {message}")
print(f"Braille: {braille}")
```

### Example 2: Batch File Processing

```python
from braille_converter import BrailleConverter
from pathlib import Path

converter = BrailleConverter()
input_dir = Path("text_files")
output_dir = Path("braille_output")
output_dir.mkdir(exist_ok=True)

for txt_file in input_dir.glob("*.txt"):
    output_file = output_dir / f"{txt_file.stem}_braille.txt"
    converter.convert_file(txt_file, output_file)
    print(f"Converted: {txt_file} -> {output_file}")
```

### Example 3: Character Analysis

```python
from braille_converter import BrailleConverter

converter = BrailleConverter()
char = converter.convert_char('A')

print(f"Character: {char.original}")
print(f"Braille Unicode: {char.braille}")
print(f"Raised dots: {char.dots}")
print("\nDot Pattern:")
print(char.get_pattern())
```

## Character Support

The library supports:
- **Lowercase letters**: a-z
- **Uppercase letters**: A-Z (with capital indicator)
- **Numbers**: 0-9 (with number indicator)
- **Punctuation**: , ; : . ? ! ' " - ( ) / \ etc.
- **Symbols**: @ # $ % & * + = < > [ ] { } _ | ~ ^ `
- **Whitespace**: space, newline, tab

Unsupported characters are replaced with ⠿ (all dots).

## Technical Details

- Uses Unicode Braille Patterns block (U+2800 to U+283F)
- Follows standard English Braille (Grade 1) conventions
- UTF-8 encoding support for file I/O
- Pure Python implementation with no external dependencies

## License

This project is part of the Senior Design Group 6 Braille project at EC463.

## Contributing

Contributions are welcome! Please feel free to submit issues or pull requests.

## Contact

For questions or support, please contact the development team.

