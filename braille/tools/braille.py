"""
Braille character mapping and terminal visualization.
Matches the 8-dot (2x4) layout used in BrailleCell (dots 1-6 standard, 7-8 unused).
"""

# Dot number -> bit index (matches BrailleCell::_bitIndexForDot)
# bit0=dot1, bit1=dot2, bit2=dot3, bit3=dot7, bit4=dot4, bit5=dot5, bit6=dot6, bit7=dot8
_DOT_TO_BIT = {1: 0, 2: 1, 3: 2, 7: 3, 4: 4, 5: 5, 6: 6, 8: 7}

# Display order: left column = dots 1,2,3,7; right column = 4,5,6,8
_LEFT_DOTS = [1, 2, 3, 7]
_RIGHT_DOTS = [4, 5, 6, 8]


def _make_pattern(dots: tuple) -> int:
    p = 0
    for d in dots:
        p |= 1 << _DOT_TO_BIT[d]
    return p


def char_to_braille(c: str) -> int:
    """Return 8-bit Braille pattern for one character. 0 = space/unsupported."""
    if not c:
        return 0
    lc = c.lower()
    # Letters a-z
    letters = {
        'a': (1,), 'b': (1, 2), 'c': (1, 4), 'd': (1, 4, 5), 'e': (1, 5),
        'f': (1, 2, 4), 'g': (1, 2, 4, 5), 'h': (1, 2, 5), 'i': (2, 4), 'j': (2, 4, 5),
        'k': (1, 3), 'l': (1, 2, 3), 'm': (1, 3, 4), 'n': (1, 3, 4, 5), 'o': (1, 3, 5),
        'p': (1, 2, 3, 4), 'q': (1, 2, 3, 4, 5), 'r': (1, 2, 3, 5), 's': (2, 3, 4), 't': (2, 3, 4, 5),
        'u': (1, 3, 6), 'v': (1, 2, 3, 6), 'w': (2, 4, 5, 6), 'x': (1, 3, 4, 6), 'y': (1, 3, 4, 5, 6), 'z': (1, 3, 5, 6),
    }
    if lc in letters:
        return _make_pattern(letters[lc])
    # Numbers 0-9 (same as a-j)
    numbers = {
        '1': (1,), '2': (1, 2), '3': (1, 4), '4': (1, 4, 5), '5': (1, 5),
        '6': (1, 2, 4), '7': (1, 2, 4, 5), '8': (1, 2, 5), '9': (2, 4), '0': (2, 4, 5),
    }
    if lc in numbers:
        return _make_pattern(numbers[lc])
    # Punctuation
    punct = {
        '.': (2, 5, 6), ',': (2,), ';': (2, 3), ':': (2, 5), '!': (2, 3, 5), '?': (2, 3, 6),
        "'": (3,), '-': (3, 6), '(': (1, 2, 6), ')': (3, 4, 5), '"': (2, 3, 5, 6),
    }
    if lc in punct:
        return _make_pattern(punct[lc])
    if c == ' ':
        return 0
    return 0


NUMBER_INDICATOR_PATTERN = _make_pattern((3, 4, 5, 6))  # dots 3,4,5,6


def pattern_to_grid(pattern: int) -> list[str]:
    """Return 4 lines for terminal (2x4 cell)."""
    lines = ["+---+---+", "", "", "", "+---+---+"]
    for row in range(4):
        left_bit = 1 << _DOT_TO_BIT[_LEFT_DOTS[row]]
        right_bit = 1 << _DOT_TO_BIT[_RIGHT_DOTS[row]]
        left = "O" if (pattern & left_bit) else "."
        right = "O" if (pattern & right_bit) else "."
        lines[row + 1] = f"| {left} | {right} |"
    return lines


def print_cell(pattern: int, label: str | None = None) -> None:
    """Print one Braille cell to terminal with optional label."""
    if label:
        print(label)
    for line in pattern_to_grid(pattern):
        print(line)
