"""
Unit tests for the Braille Converter library.
"""

import pytest
from pathlib import Path
import tempfile
from braille_converter import text_to_braille, file_to_braille, BrailleConverter, BrailleChar


class TestBasicConversion:
    """Test basic text to braille conversion."""
    
    def test_lowercase_letters(self):
        """Test lowercase letter conversion."""
        assert text_to_braille('a') == '⠁'
        assert text_to_braille('b') == '⠃'
        assert text_to_braille('z') == '⠵'
    
    def test_uppercase_letters(self):
        """Test uppercase letter conversion."""
        assert text_to_braille('A') == '⠠⠁'
        assert text_to_braille('B') == '⠠⠃'
        assert text_to_braille('Z') == '⠠⠵'
    
    def test_numbers(self):
        """Test number conversion."""
        assert text_to_braille('1') == '⠼⠁'
        assert text_to_braille('2') == '⠼⠃'
        assert text_to_braille('0') == '⠼⠚'
    
    def test_punctuation(self):
        """Test punctuation conversion."""
        assert text_to_braille(' ') == '⠀'
        assert text_to_braille('.') == '⠲'
        assert text_to_braille(',') == '⠂'
        assert text_to_braille('!') == '⠖'
        assert text_to_braille('?') == '⠦'
    
    def test_word_conversion(self):
        """Test complete word conversion."""
        result = text_to_braille('hello')
        assert len(result) == 5
        assert result == '⠓⠑⠇⠇⠕'
    
    def test_sentence_conversion(self):
        """Test sentence conversion."""
        text = "Hi!"
        braille = text_to_braille(text)
        assert len(braille) > 0
        assert '⠠' in braille  # Capital indicator
        assert '⠖' in braille  # Exclamation mark


class TestBrailleChar:
    """Test BrailleChar class."""
    
    def test_braille_char_creation(self):
        """Test creating a BrailleChar object."""
        bc = BrailleChar('a')
        assert bc.original == 'a'
        assert bc.braille == '⠁'
        assert bc.dots == [1]
    
    def test_braille_char_str(self):
        """Test string representation."""
        bc = BrailleChar('b')
        assert str(bc) == '⠃'
    
    def test_braille_char_repr(self):
        """Test repr representation."""
        bc = BrailleChar('a')
        repr_str = repr(bc)
        assert 'BrailleChar' in repr_str
        assert 'a' in repr_str
    
    def test_braille_char_pattern(self):
        """Test dot pattern generation."""
        bc = BrailleChar('a')
        pattern = bc.get_pattern()
        assert '●' in pattern
        assert '○' in pattern
    
    def test_braille_char_to_dict(self):
        """Test dictionary conversion."""
        bc = BrailleChar('a')
        d = bc.to_dict()
        assert d['original'] == 'a'
        assert d['braille'] == '⠁'
        assert d['dots'] == [1]


class TestBrailleConverter:
    """Test BrailleConverter class."""
    
    def test_converter_creation(self):
        """Test creating a converter."""
        converter = BrailleConverter()
        assert converter is not None
    
    def test_convert_char(self):
        """Test converting a single character."""
        converter = BrailleConverter()
        bc = converter.convert_char('a')
        assert isinstance(bc, BrailleChar)
        assert bc.original == 'a'
    
    def test_convert_text(self):
        """Test converting text."""
        converter = BrailleConverter()
        result = converter.convert_text('hello')
        assert result == '⠓⠑⠇⠇⠕'
    
    def test_convert_text_detailed(self):
        """Test detailed text conversion."""
        converter = BrailleConverter()
        chars = converter.convert_text_detailed('ab')
        assert len(chars) == 2
        assert all(isinstance(c, BrailleChar) for c in chars)
    
    def test_convert_and_analyze(self):
        """Test text analysis."""
        converter = BrailleConverter()
        analysis = converter.convert_and_analyze('Hello 123!')
        
        assert 'original_text' in analysis
        assert 'braille_text' in analysis
        assert 'statistics' in analysis
        assert 'characters' in analysis
        
        stats = analysis['statistics']
        assert stats['total_characters'] == 10
        assert stats['letters'] == 5
        assert stats['digits'] == 3


class TestFileConversion:
    """Test file conversion functionality."""
    
    def test_file_conversion(self):
        """Test converting a file."""
        with tempfile.NamedTemporaryFile(mode='w', suffix='.txt', delete=False) as f:
            f.write('Hello World')
            temp_input = f.name
        
        try:
            braille = file_to_braille(temp_input)
            assert len(braille) > 0
            assert '⠠' in braille  # Capital indicator
        finally:
            Path(temp_input).unlink()
    
    def test_file_conversion_with_output(self):
        """Test converting a file with output."""
        with tempfile.NamedTemporaryFile(mode='w', suffix='.txt', delete=False) as f:
            f.write('Test')
            temp_input = f.name
        
        with tempfile.NamedTemporaryFile(mode='w', suffix='.txt', delete=False) as f:
            temp_output = f.name
        
        try:
            braille = file_to_braille(temp_input, temp_output)
            
            # Check output file was created
            assert Path(temp_output).exists()
            
            # Read and verify
            with open(temp_output, 'r') as f:
                content = f.read()
            assert content == braille
        finally:
            Path(temp_input).unlink()
            Path(temp_output).unlink()
    
    def test_file_not_found(self):
        """Test handling of missing file."""
        converter = BrailleConverter()
        with pytest.raises(FileNotFoundError):
            converter.convert_file('nonexistent_file.txt')


class TestNewlineHandling:
    """Test newline and whitespace handling."""
    
    def test_preserve_newlines(self):
        """Test preserving newlines."""
        text = "Line1\nLine2"
        braille = text_to_braille(text, preserve_newlines=True)
        assert '\n' in braille
    
    def test_multiline_text(self):
        """Test multiline text conversion."""
        text = "First\nSecond\nThird"
        braille = text_to_braille(text, preserve_newlines=True)
        lines = braille.split('\n')
        assert len(lines) == 3


class TestSpecialCases:
    """Test special cases and edge cases."""
    
    def test_empty_string(self):
        """Test empty string conversion."""
        result = text_to_braille('')
        assert result == ''
    
    def test_only_spaces(self):
        """Test string with only spaces."""
        result = text_to_braille('   ')
        assert len(result) == 3
        assert result == '⠀⠀⠀'
    
    def test_unknown_character(self):
        """Test handling of unknown characters."""
        # Some unicode characters might not be in the mapping
        result = text_to_braille('§')
        assert len(result) > 0  # Should still produce output


class TestStatistics:
    """Test statistics functionality."""
    
    def test_statistics_accuracy(self):
        """Test that statistics are accurate."""
        converter = BrailleConverter()
        text = "ABC 123!"
        analysis = converter.convert_and_analyze(text)
        
        stats = analysis['statistics']
        assert stats['letters'] == 3  # ABC
        assert stats['digits'] == 3   # 123
        assert stats['spaces'] == 1   # one space
        assert stats['punctuation'] == 1  # !
        assert stats['total_characters'] == 8


def run_tests():
    """Run all tests."""
    pytest.main([__file__, '-v'])


if __name__ == '__main__':
    run_tests()

