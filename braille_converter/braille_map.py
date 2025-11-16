"""
Braille character mapping for 6-dot braille system.

6-dot braille uses a 2x3 grid:
    1 • • 4
    2 • • 5
    3 • • 6

Each character is represented by which dots are raised.
"""

# Standard 6-dot braille Unicode characters (U+2800 to U+283F)
# The mapping follows the standard English Braille (Grade 1)

CHAR_TO_BRAILLE = {
    # Lowercase letters
    'a': '⠁',  # dot 1
    'b': '⠃',  # dots 1,2
    'c': '⠉',  # dots 1,4
    'd': '⠙',  # dots 1,4,5
    'e': '⠑',  # dots 1,5
    'f': '⠋',  # dots 1,2,4
    'g': '⠛',  # dots 1,2,4,5
    'h': '⠓',  # dots 1,2,5
    'i': '⠊',  # dots 2,4
    'j': '⠚',  # dots 2,4,5
    'k': '⠅',  # dots 1,3
    'l': '⠇',  # dots 1,2,3
    'm': '⠍',  # dots 1,3,4
    'n': '⠝',  # dots 1,3,4,5
    'o': '⠕',  # dots 1,3,5
    'p': '⠏',  # dots 1,2,3,4
    'q': '⠟',  # dots 1,2,3,4,5
    'r': '⠗',  # dots 1,2,3,5
    's': '⠎',  # dots 2,3,4
    't': '⠞',  # dots 2,3,4,5
    'u': '⠥',  # dots 1,3,6
    'v': '⠧',  # dots 1,2,3,6
    'w': '⠺',  # dots 2,4,5,6
    'x': '⠭',  # dots 1,3,4,6
    'y': '⠽',  # dots 1,3,4,5,6
    'z': '⠵',  # dots 1,3,5,6
    
    # Capital letter indicator (dot 6) + letter
    # For simplicity, uppercase letters use the capital indicator
    'A': '⠠⠁',
    'B': '⠠⠃',
    'C': '⠠⠉',
    'D': '⠠⠙',
    'E': '⠠⠑',
    'F': '⠠⠋',
    'G': '⠠⠛',
    'H': '⠠⠓',
    'I': '⠠⠊',
    'J': '⠠⠚',
    'K': '⠠⠅',
    'L': '⠠⠇',
    'M': '⠠⠍',
    'N': '⠠⠝',
    'O': '⠠⠕',
    'P': '⠠⠏',
    'Q': '⠠⠟',
    'R': '⠠⠗',
    'S': '⠠⠎',
    'T': '⠠⠞',
    'U': '⠠⠥',
    'V': '⠠⠧',
    'W': '⠠⠺',
    'X': '⠠⠭',
    'Y': '⠠⠽',
    'Z': '⠠⠵',
    
    # Numbers (number indicator + letters a-j)
    '0': '⠼⠚',  # number sign + j
    '1': '⠼⠁',  # number sign + a
    '2': '⠼⠃',  # number sign + b
    '3': '⠼⠉',  # number sign + c
    '4': '⠼⠙',  # number sign + d
    '5': '⠼⠑',  # number sign + e
    '6': '⠼⠋',  # number sign + f
    '7': '⠼⠛',  # number sign + g
    '8': '⠼⠓',  # number sign + h
    '9': '⠼⠊',  # number sign + i
    
    # Punctuation and symbols
    ' ': '⠀',   # space (no dots)
    ',': '⠂',   # dot 2
    ';': '⠆',   # dots 2,3
    ':': '⠒',   # dots 2,5
    '.': '⠲',   # dots 2,5,6
    '?': '⠦',   # dots 2,3,6
    '!': '⠖',   # dots 2,3,5
    "'": '⠄',   # dot 3
    '"': '⠶',   # dots 2,3,5,6
    '-': '⠤',   # dots 3,6
    '(': '⠐⠣',  # dots 5 + 1,2,6
    ')': '⠐⠜',  # dots 5 + 3,4,5
    '/': '⠸⠌',  # dots 4,5,6 + 3,4
    '\\': '⠸⠡', # dots 4,5,6 + 1,6
    '@': '⠈⠁',  # dot 4 + a
    '#': '⠼',   # dots 3,4,5,6
    '$': '⠈⠎',  # dot 4 + s
    '%': '⠸⠴',  # dots 4,5,6 + 3,4,6
    '&': '⠈⠯',  # dot 4 + 1,2,3,4,6
    '*': '⠈⠔',  # dot 4 + 3,5
    '+': '⠬',   # dots 3,4,6
    '=': '⠸⠶',  # dots 4,5,6 + 2,3,5,6
    '<': '⠈⠣',  # dot 4 + 1,2,6
    '>': '⠈⠜',  # dot 4 + 3,4,5
    '[': '⠨⠣',  # dots 4,6 + 1,2,6
    ']': '⠨⠜',  # dots 4,6 + 3,4,5
    '{': '⠸⠣',  # dots 4,5,6 + 1,2,6
    '}': '⠸⠜',  # dots 4,5,6 + 3,4,5
    '_': '⠨⠤',  # dots 4,6 + 3,6
    '|': '⠸⠳',  # dots 4,5,6 + 1,2,3,5,6
    '~': '⠈⠱',  # dot 4 + 1,5,6
    '^': '⠘⠣',  # dots 4,5 + 1,2,6
    '`': '⠈⠢',  # dot 4 + 2,6
    
    # Newline and tab
    '\n': '\n',
    '\t': '⠀⠀',  # two spaces in braille
}

# Reverse mapping for decoding (braille to text)
BRAILLE_TO_CHAR = {v: k for k, v in CHAR_TO_BRAILLE.items()}


def get_braille_char(char: str) -> str:
    """
    Get the braille representation of a single character.
    
    Args:
        char: A single character to convert
        
    Returns:
        The braille representation, or a placeholder if not found
    """
    return CHAR_TO_BRAILLE.get(char, '⠿')  # ⠿ is used for unknown characters


def char_to_dots(char: str) -> list:
    """
    Convert a character to its dot pattern representation.
    
    Args:
        char: A single character
        
    Returns:
        A list of dot numbers (1-6) that are raised
    """
    braille = get_braille_char(char)
    if braille == '\n':
        return []
    
    # Unicode braille starts at U+2800 (no dots)
    # Each subsequent character adds dots based on binary pattern
    code_point = ord(braille[0]) - 0x2800
    
    dots = []
    dot_positions = [1, 2, 3, 7, 4, 5, 6, 8]  # Mapping of bits to dot numbers
    
    for i, dot_num in enumerate(dot_positions[:6]):
        if code_point & (1 << i):
            dots.append(dot_num if dot_num <= 6 else dot_num - 1)
    
    return sorted(dots)


def dots_to_pattern(dots: list) -> str:
    """
    Convert a list of dot numbers to a visual pattern.
    
    Args:
        dots: List of dot numbers (1-6)
        
    Returns:
        A string representation of the dot pattern
    """
    pattern = [
        ['○', '○'],  # Row 1: dots 1, 4
        ['○', '○'],  # Row 2: dots 2, 5
        ['○', '○']   # Row 3: dots 3, 6
    ]
    
    dot_positions = {
        1: (0, 0), 2: (1, 0), 3: (2, 0),
        4: (0, 1), 5: (1, 1), 6: (2, 1)
    }
    
    for dot in dots:
        if dot in dot_positions:
            row, col = dot_positions[dot]
            pattern[row][col] = '●'
    
    return '\n'.join([' '.join(row) for row in pattern])

