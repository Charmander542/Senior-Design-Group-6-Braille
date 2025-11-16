"""
Braille Converter Library
A Python library for converting text to 6-dot braille representation.
"""

from .converter import (
    text_to_braille,
    file_to_braille,
    BrailleConverter,
    BrailleChar
)

__version__ = "1.0.0"
__all__ = [
    "text_to_braille",
    "file_to_braille",
    "BrailleConverter",
    "BrailleChar"
]

