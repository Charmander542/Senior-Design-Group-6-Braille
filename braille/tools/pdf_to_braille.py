#!/usr/bin/env python3
"""
Read a PDF file, extract its text, and print the Braille equivalent to the terminal.
Uses the same 2x4 Braille cell layout as the Arduino BrailleCell library.
"""

import sys
from pathlib import Path

# Allow running as: python tools/pdf_to_braille.py file.pdf (from repo root)
_SCRIPT_DIR = Path(__file__).resolve().parent
if str(_SCRIPT_DIR) not in sys.path:
    sys.path.insert(0, str(_SCRIPT_DIR))

try:
    from pypdf import PdfReader
except ImportError:
    print("Missing dependency: pypdf. Install with: pip install pypdf", file=sys.stderr)
    sys.exit(1)

from braille import (
    char_to_braille,
    NUMBER_INDICATOR_PATTERN,
    print_cell,
)


def extract_text_from_pdf(pdf_path: Path) -> str:
    """Extract raw text from all pages of a PDF."""
    reader = PdfReader(pdf_path)
    parts = []
    for page in reader.pages:
        parts.append(page.extract_text() or "")
    return "\n".join(parts)


def clean_extracted_text(raw: str) -> str:
    """Clean text from PDF extraction: normalize spaces, drop junk lines."""
    # Only keep printable ASCII and newlines
    text = "".join(c for c in raw if c == "\n" or (ord(c) >= 32 and ord(c) <= 126))
    # Collapse multiple spaces to one
    while "  " in text:
        text = text.replace("  ", " ")
    # Collapse multiple newlines to one, then strip each line
    lines = [line.strip() for line in text.splitlines()]
    # Drop lines that are only digits (often page numbers) or empty
    lines = [L for L in lines if L and not (L.isdigit() and len(L) <= 4)]
    # Rejoin with single newline and strip
    return "\n".join(lines).strip()


def main_usage() -> str:
    return (
        "Usage: python pdf_to_braille.py [options] <path-to-pdf>\n"
        "Options:\n"
        "  --preview             Print extracted text only (no Braille conversion).\n"
        "  --first-paragraph     Convert only the first paragraph (up to first blank line).\n"
        "  --max-chars N         Convert only the first N characters.\n"
        "Example: python pdf_to_braille.py document.pdf\n"
        "         python pdf_to_braille.py --first-paragraph document.pdf"
    )


def display_text_as_braille(text: str) -> None:
    """Print text as Braille cells to the terminal (same style as Arduino Serial output)."""
    # Normalize: keep newlines as-is for readability, strip other control chars
    cleaned = "".join(c for c in text if c == "\n" or (ord(c) >= 32 and ord(c) <= 126))
    if not cleaned.strip():
        print("(No printable text in input.)")
        return

    print()
    print("========================================")
    print("CONVERTING TO BRAILLE")
    print("========================================")
    print("Input (first 200 chars):")
    preview = cleaned.replace("\n", " ").strip()[:200]
    if len(cleaned.replace("\n", " ").strip()) > 200:
        preview += "..."
    print(f'"{preview}"')
    print("----------------------------------------")
    print()

    in_number_mode = False
    char_index = 0
    total_chars = sum(1 for c in cleaned if c != "\n")

    for i, c in enumerate(cleaned):
        if c == "\n":
            print("[Newline]")
            print()
            continue

        is_digit = c.isdigit()
        if is_digit and not in_number_mode:
            print("[Number Indicator]")
            print_cell(NUMBER_INDICATOR_PATTERN)
            print()
            in_number_mode = True
        elif not is_digit and in_number_mode:
            in_number_mode = False

        char_index += 1
        print(f"Character {char_index}/{total_chars}: ", end="")
        if c == " ":
            label = "'SPACE'"
        elif 33 <= ord(c) <= 126:
            label = f"'{c}'"
        else:
            label = f"(0x{ord(c):02X})"
        pattern = char_to_braille(c)
        print_cell(pattern, label)
        print()

    print("========================================")
    print("CONVERSION COMPLETE!")
    print("========================================")
    print()


def main() -> None:
    argv = sys.argv[1:]
    preview = "--preview" in argv
    first_paragraph = "--first-paragraph" in argv
    max_chars: int | None = None
    pdf_path = None
    i = 0
    while i < len(argv):
        a = argv[i]
        if a == "--preview" or a == "--first-paragraph":
            i += 1
            continue
        if a == "--max-chars":
            i += 1
            if i < len(argv) and argv[i].isdigit():
                max_chars = int(argv[i])
                i += 1
            continue
        if a.startswith("--max-chars="):
            try:
                max_chars = int(a.split("=", 1)[1])
            except ValueError:
                pass
            i += 1
            continue
        if not a.startswith("--"):
            pdf_path = Path(a)
            i += 1
            break
        i += 1
    # In case path was before some flags
    if pdf_path is None:
        for a in argv:
            if not a.startswith("--") and a.isdigit() is False:
                pdf_path = Path(a)
                break

    if pdf_path is None:
        print(main_usage(), file=sys.stderr)
        sys.exit(1)
    if not pdf_path.exists():
        print(f"Error: File not found: {pdf_path}", file=sys.stderr)
        sys.exit(1)
    if pdf_path.suffix.lower() != ".pdf":
        print("Warning: File does not have .pdf extension.", file=sys.stderr)

    raw = extract_text_from_pdf(pdf_path)
    text = clean_extracted_text(raw)

    if first_paragraph:
        first = text.split("\n\n")[0].strip()
        if first:
            text = first
    if max_chars is not None and max_chars > 0:
        text = text[:max_chars]

    if preview:
        print("Extracted text (after cleaning):")
        print("-" * 40)
        print(text)
        print("-" * 40)
        return

    if not text.strip():
        print("(No printable text left after cleaning. Use --preview to see raw extraction.)", file=sys.stderr)
        sys.exit(1)
    display_text_as_braille(text)


if __name__ == "__main__":
    main()
