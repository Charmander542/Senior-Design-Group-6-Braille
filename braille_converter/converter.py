"""
Main converter module for text to braille conversion.
"""

from typing import Union, Optional
from pathlib import Path
from .braille_map import (
    CHAR_TO_BRAILLE,
    get_braille_char,
    char_to_dots,
    dots_to_pattern
)


class BrailleChar:
    """Represents a single character in braille."""
    
    def __init__(self, original_char: str):
        """
        Initialize a BrailleChar.
        
        Args:
            original_char: The original text character
        """
        self.original = original_char
        self.braille = get_braille_char(original_char)
        self.dots = char_to_dots(original_char)
    
    def __str__(self) -> str:
        return self.braille
    
    def __repr__(self) -> str:
        return f"BrailleChar('{self.original}' -> '{self.braille}', dots: {self.dots})"
    
    def get_pattern(self) -> str:
        """Get the visual dot pattern representation."""
        return dots_to_pattern(self.dots)
    
    def to_dict(self) -> dict:
        """Convert to dictionary representation."""
        return {
            'original': self.original,
            'braille': self.braille,
            'dots': self.dots
        }


class BrailleConverter:
    """Main converter class for text to braille conversion."""
    
    def __init__(self):
        """Initialize the BrailleConverter."""
        self.char_map = CHAR_TO_BRAILLE
    
    def convert_char(self, char: str) -> BrailleChar:
        """
        Convert a single character to braille.
        
        Args:
            char: A single character to convert
            
        Returns:
            BrailleChar object
        """
        return BrailleChar(char)
    
    def convert_text(self, text: str, preserve_newlines: bool = True) -> str:
        """
        Convert text to braille.
        
        Args:
            text: The text to convert
            preserve_newlines: Whether to preserve newline characters
            
        Returns:
            The braille representation of the text
        """
        result = []
        for char in text:
            if char == '\n' and preserve_newlines:
                result.append('\n')
            else:
                result.append(get_braille_char(char))
        return ''.join(result)
    
    def convert_text_detailed(self, text: str) -> list[BrailleChar]:
        """
        Convert text to a list of BrailleChar objects with detailed information.
        
        Args:
            text: The text to convert
            
        Returns:
            List of BrailleChar objects
        """
        return [BrailleChar(char) for char in text]
    
    def convert_file(
        self,
        input_path: Union[str, Path],
        output_path: Optional[Union[str, Path]] = None,
        encoding: str = 'utf-8',
        preserve_newlines: bool = True
    ) -> str:
        """
        Convert a text file to braille.
        
        Args:
            input_path: Path to the input text file
            output_path: Optional path to save the braille output
            encoding: File encoding (default: utf-8)
            preserve_newlines: Whether to preserve newline characters
            
        Returns:
            The braille representation of the file contents
        """
        input_path = Path(input_path)
        
        if not input_path.exists():
            raise FileNotFoundError(f"Input file not found: {input_path}")
        
        # Read the input file
        with open(input_path, 'r', encoding=encoding) as f:
            text = f.read()
        
        # Convert to braille
        braille_text = self.convert_text(text, preserve_newlines=preserve_newlines)
        
        # Save to output file if specified
        if output_path:
            output_path = Path(output_path)
            with open(output_path, 'w', encoding=encoding) as f:
                f.write(braille_text)
        
        return braille_text
    
    def convert_and_analyze(self, text: str) -> dict:
        """
        Convert text and return detailed analysis.
        
        Args:
            text: The text to convert
            
        Returns:
            Dictionary containing braille text and statistics
        """
        braille_chars = self.convert_text_detailed(text)
        braille_text = ''.join([str(bc) for bc in braille_chars])
        
        # Calculate statistics
        total_chars = len(text)
        letters = sum(1 for c in text if c.isalpha())
        digits = sum(1 for c in text if c.isdigit())
        spaces = sum(1 for c in text if c.isspace())
        punctuation = total_chars - letters - digits - spaces
        
        return {
            'original_text': text,
            'braille_text': braille_text,
            'characters': [bc.to_dict() for bc in braille_chars],
            'statistics': {
                'total_characters': total_chars,
                'letters': letters,
                'digits': digits,
                'spaces': spaces,
                'punctuation': punctuation
            }
        }


# Convenience functions for quick conversion

def text_to_braille(text: str, preserve_newlines: bool = True) -> str:
    """
    Convert text to braille (convenience function).
    
    Args:
        text: The text to convert
        preserve_newlines: Whether to preserve newline characters
        
    Returns:
        The braille representation of the text
    """
    converter = BrailleConverter()
    return converter.convert_text(text, preserve_newlines=preserve_newlines)


def file_to_braille(
    input_path: Union[str, Path],
    output_path: Optional[Union[str, Path]] = None,
    encoding: str = 'utf-8'
) -> str:
    """
    Convert a text file to braille (convenience function).
    
    Args:
        input_path: Path to the input text file
        output_path: Optional path to save the braille output
        encoding: File encoding (default: utf-8)
        
    Returns:
        The braille representation of the file contents
    """
    converter = BrailleConverter()
    return converter.convert_file(input_path, output_path, encoding=encoding)

