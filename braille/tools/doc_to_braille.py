#!/usr/bin/env python3
"""
Convert documents (PDF, EPUB, DOCX, HTML, TXT) to Braille and display in the terminal.
Uses the same 2x4 Braille cell layout as the Arduino BrailleCell library.

Supports downloading free ebooks from Project Gutenberg by ID.
"""

import sys
import re
from pathlib import Path

_SCRIPT_DIR = Path(__file__).resolve().parent
if str(_SCRIPT_DIR) not in sys.path:
    sys.path.insert(0, str(_SCRIPT_DIR))

from braille import (
    char_to_braille,
    NUMBER_INDICATOR_PATTERN,
    print_cell,
)

SUPPORTED_EXTENSIONS = {".pdf", ".epub", ".docx", ".html", ".htm", ".txt"}


# ---------------------------------------------------------------------------
# Text extraction — one function per format
# ---------------------------------------------------------------------------

def extract_text_from_pdf(path: Path) -> str:
    try:
        from pypdf import PdfReader
    except ImportError:
        _missing("pypdf", "pip install pypdf")
    reader = PdfReader(path)
    return "\n".join(page.extract_text() or "" for page in reader.pages)


def extract_text_from_epub(path: Path) -> str:
    try:
        import ebooklib
        from ebooklib import epub
    except ImportError:
        _missing("ebooklib", "pip install EbookLib")
    try:
        from bs4 import BeautifulSoup
    except ImportError:
        _missing("beautifulsoup4", "pip install beautifulsoup4")

    book = epub.read_epub(str(path), options={"ignore_ncx": True})
    parts: list[str] = []
    for item in book.get_items_of_type(ebooklib.ITEM_DOCUMENT):
        soup = BeautifulSoup(item.get_body_content(), "html.parser")
        text = soup.get_text(separator="\n")
        if text.strip():
            parts.append(text)
    return "\n".join(parts)


def extract_text_from_docx(path: Path) -> str:
    try:
        import docx
    except ImportError:
        _missing("python-docx", "pip install python-docx")
    doc = docx.Document(str(path))
    return "\n".join(para.text for para in doc.paragraphs)


def extract_text_from_html(path: Path) -> str:
    try:
        from bs4 import BeautifulSoup
    except ImportError:
        _missing("beautifulsoup4", "pip install beautifulsoup4")
    raw = path.read_text(encoding="utf-8", errors="replace")
    soup = BeautifulSoup(raw, "html.parser")
    for tag in soup(["script", "style", "nav", "footer", "header"]):
        tag.decompose()
    return soup.get_text(separator="\n")


def extract_text_from_txt(path: Path) -> str:
    return path.read_text(encoding="utf-8", errors="replace")


_EXTRACTORS = {
    ".pdf": extract_text_from_pdf,
    ".epub": extract_text_from_epub,
    ".docx": extract_text_from_docx,
    ".html": extract_text_from_html,
    ".htm": extract_text_from_html,
    ".txt": extract_text_from_txt,
}


def extract_text(path: Path) -> str:
    ext = path.suffix.lower()
    extractor = _EXTRACTORS.get(ext)
    if extractor is None:
        print(
            f"Error: Unsupported file type '{ext}'. "
            f"Supported: {', '.join(sorted(SUPPORTED_EXTENSIONS))}",
            file=sys.stderr,
        )
        sys.exit(1)
    return extractor(path)


# ---------------------------------------------------------------------------
# Project Gutenberg downloader
# ---------------------------------------------------------------------------

def download_gutenberg(book_id: int, dest_dir: Path | None = None) -> Path:
    """Download a plain-text book from Project Gutenberg and return its path."""
    import urllib.request
    import urllib.error
    import ssl

    url = f"https://www.gutenberg.org/cache/epub/{book_id}/pg{book_id}.txt"
    if dest_dir is None:
        dest_dir = Path.cwd()
    dest_dir.mkdir(parents=True, exist_ok=True)
    dest = dest_dir / f"gutenberg_{book_id}.txt"

    if dest.exists():
        print(f"Using cached file: {dest}")
        return dest

    print(f"Downloading Project Gutenberg book #{book_id} ...")
    print(f"  URL: {url}")
    try:
        urllib.request.urlretrieve(url, dest)
    except urllib.error.URLError as exc:
        if "CERTIFICATE_VERIFY_FAILED" in str(exc):
            print("  SSL certificate verification failed; retrying without verification...")
            ctx = ssl.create_default_context()
            ctx.check_hostname = False
            ctx.verify_mode = ssl.CERT_NONE
            try:
                req = urllib.request.Request(url)
                with urllib.request.urlopen(req, context=ctx) as resp:
                    dest.write_bytes(resp.read())
            except Exception as inner:
                print(f"Error: Could not download book #{book_id} ({inner})", file=sys.stderr)
                sys.exit(1)
        else:
            print(f"Error: Could not download book #{book_id} ({exc})", file=sys.stderr)
            sys.exit(1)
    except urllib.error.HTTPError as exc:
        print(f"Error: Could not download book #{book_id} ({exc})", file=sys.stderr)
        sys.exit(1)
    print(f"  Saved to: {dest}")
    return dest


def strip_gutenberg_header_footer(text: str) -> str:
    """Remove the standard Project Gutenberg header and footer boilerplate."""
    start_markers = [
        "*** START OF THE PROJECT GUTENBERG EBOOK",
        "*** START OF THIS PROJECT GUTENBERG EBOOK",
    ]
    end_markers = [
        "*** END OF THE PROJECT GUTENBERG EBOOK",
        "*** END OF THIS PROJECT GUTENBERG EBOOK",
        "End of the Project Gutenberg EBook",
        "End of Project Gutenberg",
    ]
    start_idx = 0
    for marker in start_markers:
        idx = text.find(marker)
        if idx != -1:
            newline = text.find("\n", idx)
            start_idx = newline + 1 if newline != -1 else idx + len(marker)
            break

    end_idx = len(text)
    for marker in end_markers:
        idx = text.find(marker)
        if idx != -1:
            end_idx = idx
            break

    return text[start_idx:end_idx].strip()


# ---------------------------------------------------------------------------
# Text cleaning (shared across formats)
# ---------------------------------------------------------------------------

def clean_extracted_text(raw: str) -> str:
    text = "".join(c for c in raw if c == "\n" or (32 <= ord(c) <= 126))
    text = re.sub(r" {2,}", " ", text)
    lines = [line.strip() for line in text.splitlines()]
    lines = [ln for ln in lines if ln and not (ln.isdigit() and len(ln) <= 4)]
    return "\n".join(lines).strip()


# ---------------------------------------------------------------------------
# Braille display (unchanged from pdf_to_braille.py)
# ---------------------------------------------------------------------------

def display_text_as_braille(text: str) -> None:
    cleaned = "".join(c for c in text if c == "\n" or (32 <= ord(c) <= 126))
    if not cleaned.strip():
        print("(No printable text in input.)")
        return

    print()
    print("========================================")
    print("CONVERTING TO BRAILLE")
    print("========================================")
    preview = cleaned.replace("\n", " ").strip()[:200]
    if len(cleaned.replace("\n", " ").strip()) > 200:
        preview += "..."
    print("Input (first 200 chars):")
    print(f'"{preview}"')
    print("----------------------------------------")
    print()

    in_number_mode = False
    char_index = 0
    total_chars = sum(1 for c in cleaned if c != "\n")

    for c in cleaned:
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
        if c == " ":
            label = "'SPACE'"
        elif 33 <= ord(c) <= 126:
            label = f"'{c}'"
        else:
            label = f"(0x{ord(c):02X})"
        print(f"Character {char_index}/{total_chars}: ", end="")
        print_cell(char_to_braille(c), label)
        print()

    print("========================================")
    print("CONVERSION COMPLETE!")
    print("========================================")
    print()


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _missing(package: str, install_cmd: str):
    print(
        f"Missing dependency: {package}. Install with: {install_cmd}",
        file=sys.stderr,
    )
    sys.exit(1)


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

USAGE = """\
Usage: python doc_to_braille.py [options] <file>
       python doc_to_braille.py --gutenberg <book-id> [options]

Supported formats: PDF, EPUB, DOCX, HTML, TXT

Options:
  --preview             Print extracted text only (no Braille conversion).
  --first-paragraph     Convert only the first paragraph.
  --max-chars N         Convert only the first N characters.
  --gutenberg ID        Download a book from Project Gutenberg by numeric ID
                        (e.g. 1342 for Pride and Prejudice).

Examples:
  python doc_to_braille.py book.epub
  python doc_to_braille.py document.pdf --first-paragraph
  python doc_to_braille.py notes.docx --max-chars 500
  python doc_to_braille.py page.html --preview
  python doc_to_braille.py story.txt
  python doc_to_braille.py --gutenberg 1342 --max-chars 300
"""


def main() -> None:
    argv = sys.argv[1:]
    if not argv or "--help" in argv or "-h" in argv:
        print(USAGE, file=sys.stderr)
        sys.exit(0 if ("--help" in argv or "-h" in argv) else 1)

    preview = "--preview" in argv
    first_paragraph = "--first-paragraph" in argv
    max_chars: int | None = None
    gutenberg_id: int | None = None
    file_path: Path | None = None

    i = 0
    while i < len(argv):
        a = argv[i]
        if a in ("--preview", "--first-paragraph"):
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
        if a == "--gutenberg":
            i += 1
            if i < len(argv) and argv[i].isdigit():
                gutenberg_id = int(argv[i])
            else:
                print("Error: --gutenberg requires a numeric book ID.", file=sys.stderr)
                sys.exit(1)
            i += 1
            continue
        if a.startswith("--gutenberg="):
            try:
                gutenberg_id = int(a.split("=", 1)[1])
            except ValueError:
                print("Error: --gutenberg requires a numeric book ID.", file=sys.stderr)
                sys.exit(1)
            i += 1
            continue
        if not a.startswith("--"):
            file_path = Path(a)
        i += 1

    # Resolve input source
    if gutenberg_id is not None:
        file_path = download_gutenberg(gutenberg_id, _SCRIPT_DIR / "downloads")

    if file_path is None:
        print(USAGE, file=sys.stderr)
        sys.exit(1)
    if not file_path.exists():
        print(f"Error: File not found: {file_path}", file=sys.stderr)
        sys.exit(1)

    # Extract text
    raw = extract_text(file_path)

    # Gutenberg-specific: strip boilerplate
    if gutenberg_id is not None:
        raw = strip_gutenberg_header_footer(raw)

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
        print(
            "(No printable text left after cleaning. Use --preview to see raw extraction.)",
            file=sys.stderr,
        )
        sys.exit(1)

    display_text_as_braille(text)


if __name__ == "__main__":
    main()
