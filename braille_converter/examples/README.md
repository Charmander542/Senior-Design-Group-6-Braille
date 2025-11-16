# Braille Converter Examples

This directory contains example scripts demonstrating how to use the Braille Converter library.

## Running the Examples

### Basic Usage Examples

```bash
cd /Users/yuyena/Documents/EC463/-Design-Group-6-Braille/braille_converter/examples
python basic_usage.py
```

This will demonstrate:
- Simple text conversion
- Detailed character information with dot patterns
- Numbers and punctuation conversion
- Text analysis with statistics
- Multiline text handling
- Complete alphabet showcase

### File Conversion Examples

```bash
python file_conversion.py
```

This will demonstrate:
- Basic file conversion
- Batch file processing
- Detailed file analysis
- Handling different encodings
- Side-by-side comparison of original and braille

## Example Outputs

### Simple Conversion
```
Original: Hello World!
Braille:  ⠠⠓⠑⠇⠇⠕⠀⠠⠺⠕⠗⠇⠙⠖
```

### Character Details with Dot Pattern
```
Character: 'A'
Braille:   ⠠⠁
Dots:      [6, 1]
Pattern:
● ○
○ ○
○ ●
```

### File Analysis
```
File: mixed_content.txt

Statistics:
  Total Characters: 142
  Letters: 98
  Digits: 5
  Spaces: 24
  Punctuation: 15
```

## Creating Your Own Examples

Here's a simple template to get started:

```python
from braille_converter import text_to_braille, BrailleConverter

# Simple conversion
text = "Your text here"
braille = text_to_braille(text)
print(f"Original: {text}")
print(f"Braille: {braille}")

# Detailed analysis
converter = BrailleConverter()
analysis = converter.convert_and_analyze(text)
print(f"Statistics: {analysis['statistics']}")
```

## Sample Text Files

The `file_conversion.py` script automatically creates sample text files in the `sample_texts/` directory:
- `greeting.txt` - Simple greeting with newlines
- `mixed_content.txt` - Mixed content with letters, numbers, and symbols
- `numbers_symbols.txt` - Mathematical expressions and special characters

Output files are saved to `braille_output/` directory.

## Notes

- All examples use UTF-8 encoding by default
- Braille output uses Unicode Braille Patterns (U+2800 to U+283F)
- Examples preserve newlines in multiline text
- Unsupported characters are replaced with ⠿ (all dots raised)

