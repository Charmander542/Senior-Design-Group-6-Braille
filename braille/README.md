# Braille Sentence Converter

An Arduino-based project that converts text sentences into Braille patterns. Successfully tested on the [Wokwi](https://wokwi.com/) simulation platform.

## Overview

This project displays Braille characters using an 8-dot cell layout (2x4 grid). While the hardware supports 8 dots, **standard Braille only uses 6 dots** (dots 1-6), so the bottom 2 dots (7 and 8) remain empty during normal operation.

```
Braille Cell Layout:
+---+---+
| 1 | 4 |  <- Row 1
+---+---+
| 2 | 5 |  <- Row 2
+---+---+
| 3 | 6 |  <- Row 3
+---+---+
| 7 | 8 |  <- Row 4 (unused in standard Braille)
+---+---+
```

## Features

- Converts sentences up to **18 characters** (including spaces)
- Supports:
  - **Letters**: a-z (case insensitive)
  - **Numbers**: 0-9 (with automatic number indicator)
  - **Punctuation**: `. , ; : ! ? ' - ( ) "`
  - **Space**: displayed as blank cell
- Visual ASCII representation in Serial Monitor
- LED visualization on Wokwi simulator

## How to Run This Project

### Option 0: Document to Braille (Terminal)

Convert text from a document to Braille and print the result in the terminal (no Arduino needed). Supports **PDF, EPUB, DOCX, HTML, and TXT** files, plus direct download from **Project Gutenberg**.

1. Install Python 3 and dependencies:
   ```bash
   pip install -r requirements.txt
   ```
2. Run the converter with any supported document:
   ```bash
   python tools/doc_to_braille.py book.epub
   python tools/doc_to_braille.py document.pdf
   python tools/doc_to_braille.py notes.docx
   python tools/doc_to_braille.py page.html
   python tools/doc_to_braille.py story.txt
   ```
3. Or download a free book from Project Gutenberg by ID:
   ```bash
   python tools/doc_to_braille.py --gutenberg 1342 --max-chars 300
   ```
4. Useful options:
   - `--preview` -- show extracted text without Braille conversion
   - `--first-paragraph` -- convert only the first paragraph
   - `--max-chars N` -- convert only the first N characters

The legacy `pdf_to_braille.py` script is still available for PDF-only use.

### Option 1: Wokwi Web Simulator (Recommended)

1. Go to [wokwi.com](https://wokwi.com/)
2. Create a new Arduino Uno project
3. Copy the contents from `wokwi_web/` folder:
   - `sketch.ino` -> main sketch file
   - `BrailleCell.h` -> header file
   - `BrailleCell.cpp` -> implementation file
4. Set up the circuit using `diagram.json` or manually add 8 LEDs in a 2x4 grid connected to pins 2-9
5. Click **Start Simulation**
6. Open the Serial Monitor (set baud rate to **115200**)
7. Type a sentence and press Enter to see the Braille conversion

### Option 2: PlatformIO + Wokwi Extension

1. Install [PlatformIO](https://platformio.org/) in VS Code
2. Install the [Wokwi for VS Code](https://marketplace.visualstudio.com/items?itemName=wokwi.wokwi-vscode) extension
3. Clone/download this repository
4. Open the project folder in VS Code
5. Build the project:
   ```bash
   pio run
   ```
6. Press `F1` -> **Wokwi: Start Simulator**
7. Open Serial Monitor and interact with the converter

### Option 3: Physical Hardware

1. Connect 8 LEDs to Arduino Uno pins 2-9:
   - Pin 2 -> Dot 1
   - Pin 3 -> Dot 2
   - Pin 4 -> Dot 3
   - Pin 5 -> Dot 7
   - Pin 6 -> Dot 4
   - Pin 7 -> Dot 5
   - Pin 8 -> Dot 6
   - Pin 9 -> Dot 8
2. Upload the firmware using PlatformIO
3. Open Serial Monitor at 115200 baud
4. Type sentences to convert

## Project Structure

```
braille/
├── tools/                      # Document-to-Braille (terminal)
│   ├── doc_to_braille.py       # CLI: multi-format document to Braille converter
│   ├── pdf_to_braille.py       # Legacy CLI: PDF-only converter
│   └── braille.py              # Braille character mapping and visualization
├── requirements.txt            # Python deps (pypdf, EbookLib, beautifulsoup4, python-docx)
├── src/
│   └── main.cpp                # Main application code (Arduino)
├── lib/
│   └── BrailleCell/
│       ├── BrailleCell.h       # Braille cell library header
│       └── BrailleCell.cpp     # Braille cell implementation
├── wokwi_web/                  # Files for Wokwi web interface
│   ├── sketch.ino
│   ├── BrailleCell.h
│   └── BrailleCell.cpp
├── diagram.json                # Wokwi circuit diagram
├── wokwi.toml                  # Wokwi configuration
└── platformio.ini              # PlatformIO configuration
```

## Example Output

```
========================================
   BRAILLE SENTENCE CONVERTER
========================================

Enter a sentence and press Enter:
----------------------------------------

Input: "hello"
----------------------------------------

Character 1/5: 'h'
+---+---+
| O | . |
| O | O |
| . | . |
| . | . |
+---+---+

Character 2/5: 'e'
+---+---+
| O | O |
| . | . |
| . | . |
| . | . |
+---+---+
...
```

## Current Limitations

- Maximum sentence length: **18 characters** (including spaces)
- Bottom 2 dots (7 & 8) remain unused as standard Braille only requires 6 dots

## Hardware Requirements

- Arduino Uno (or compatible board)
- 8 LEDs (6 blue for active dots, 2 yellow for unused bottom dots)
- Resistors for LEDs (if using physical hardware)
