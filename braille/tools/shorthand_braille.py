#!/usr/bin/env python3
"""
Shorthand (contracted) Braille conversion helpers.

This is a practical subset of common English contractions so users can switch
between regular character-by-character Braille and shorthand-style output.
"""

from __future__ import annotations

from braille import NUMBER_INDICATOR_PATTERN, char_to_braille, pattern_to_grid


_DOT_TO_BIT = {1: 0, 2: 1, 3: 2, 7: 3, 4: 4, 5: 5, 6: 6, 8: 7}


def _make_pattern(dots: tuple[int, ...]) -> int:
    pattern = 0
    for d in dots:
        pattern |= 1 << _DOT_TO_BIT[d]
    return pattern


# Common whole-word contractions (subset)
_WHOLE_WORD = {
    "and": _make_pattern((1, 2, 3, 4, 6)),
    "for": _make_pattern((1, 2, 3, 4, 5, 6)),
    "of": _make_pattern((1, 2, 3, 5, 6)),
    "the": _make_pattern((2, 3, 4, 6)),
    "with": _make_pattern((2, 3, 4, 5, 6)),
}


# Part-word contractions (subset), matched greedily
_PART_WORD = {
    "ing": _make_pattern((3, 4, 6)),
    "ed": _make_pattern((1, 2, 4, 6)),
    "er": _make_pattern((1, 2, 4, 5, 6)),
    "ch": _make_pattern((1, 6)),
    "sh": _make_pattern((1, 4, 6)),
    "th": _make_pattern((1, 4, 5, 6)),
    "wh": _make_pattern((1, 5, 6)),
    "ou": _make_pattern((1, 2, 5, 6)),
    "st": _make_pattern((3, 4)),
}
_PART_WORD_ORDER = sorted(_PART_WORD.keys(), key=len, reverse=True)


def to_shorthand_braille_report(text: str) -> str:
    """Convert text to a terminal-style shorthand Braille report."""
    cleaned = "".join(c for c in text if c == "\n" or (32 <= ord(c) <= 126))
    if not cleaned.strip():
        return "(No printable text in input.)"

    cells: list[tuple[str, int] | None] = []
    in_number_mode = False
    i = 0

    while i < len(cleaned):
        c = cleaned[i]

        if c == "\n":
            cells.append(None)
            in_number_mode = False
            i += 1
            continue

        if c == " ":
            cells.append(("'SPACE'", 0))
            in_number_mode = False
            i += 1
            continue

        if c.isdigit():
            if not in_number_mode:
                cells.append(("#NUM", NUMBER_INDICATOR_PATTERN))
            in_number_mode = True
            cells.append((f"'{c}'", char_to_braille(c)))
            i += 1
            continue

        in_number_mode = False

        if c.isalpha():
            j = i
            while j < len(cleaned) and cleaned[j].isalpha():
                j += 1
            word = cleaned[i:j]
            lower = word.lower()

            whole = _WHOLE_WORD.get(lower)
            if whole is not None:
                cells.append((f"[{lower}]", whole))
            else:
                k = 0
                while k < len(word):
                    remaining = lower[k:]
                    matched = False
                    for token in _PART_WORD_ORDER:
                        if remaining.startswith(token):
                            cells.append((f"[{token}]", _PART_WORD[token]))
                            k += len(token)
                            matched = True
                            break
                    if not matched:
                        ch = word[k]
                        cells.append((f"'{ch}'", char_to_braille(ch)))
                        k += 1

            i = j
            continue

        cells.append((f"'{c}'", char_to_braille(c)))
        i += 1

    total_cells = sum(1 for cell in cells if cell is not None)
    current = 0

    out: list[str] = []
    out.append("========================================")
    out.append("CONVERTING TO SHORTHAND BRAILLE")
    out.append("========================================")
    preview_source = cleaned.replace("\n", " ").strip()
    preview = preview_source[:200] + ("..." if len(preview_source) > 200 else "")
    out.append("Input (first 200 chars):")
    out.append(f"\"{preview}\"")
    out.append("----------------------------------------")
    out.append("")

    for cell in cells:
        if cell is None:
            out.append("[Newline]")
            out.append("")
            continue

        label, pattern = cell
        if label == "#NUM":
            out.append("[Number Indicator]")
        else:
            current += 1
            out.append(f"Cell {current}/{total_cells}: {label}")
        out.extend(pattern_to_grid(pattern))
        out.append("")

    out.append("========================================")
    out.append("CONVERSION COMPLETE!")
    out.append("========================================")
    return "\n".join(out)
