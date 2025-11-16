"""
Basic usage examples for the Braille Converter library.
"""

from braille_converter import text_to_braille, BrailleConverter


def example_1_simple_conversion():
    """Example 1: Simple text conversion."""
    print("=" * 60)
    print("Example 1: Simple Text Conversion")
    print("=" * 60)
    
    text = "Hello World!"
    braille = text_to_braille(text)
    
    print(f"Original: {text}")
    print(f"Braille:  {braille}")
    print()


def example_2_character_details():
    """Example 2: Detailed character information."""
    print("=" * 60)
    print("Example 2: Detailed Character Information")
    print("=" * 60)
    
    converter = BrailleConverter()
    text = "ABC"
    
    braille_chars = converter.convert_text_detailed(text)
    
    for bc in braille_chars:
        print(f"\nCharacter: '{bc.original}'")
        print(f"Braille:   {bc.braille}")
        print(f"Dots:      {bc.dots}")
        print(f"Pattern:")
        print(bc.get_pattern())
    print()


def example_3_numbers_and_punctuation():
    """Example 3: Numbers and punctuation."""
    print("=" * 60)
    print("Example 3: Numbers and Punctuation")
    print("=" * 60)
    
    text = "Call me at 555-1234!"
    braille = text_to_braille(text)
    
    print(f"Original: {text}")
    print(f"Braille:  {braille}")
    print()


def example_4_analysis():
    """Example 4: Text analysis."""
    print("=" * 60)
    print("Example 4: Text Analysis")
    print("=" * 60)
    
    converter = BrailleConverter()
    text = "Python 3.11 is awesome!"
    
    analysis = converter.convert_and_analyze(text)
    
    print(f"Original: {analysis['original_text']}")
    print(f"Braille:  {analysis['braille_text']}")
    print(f"\nStatistics:")
    for key, value in analysis['statistics'].items():
        print(f"  {key}: {value}")
    print()


def example_5_multiline():
    """Example 5: Multiline text."""
    print("=" * 60)
    print("Example 5: Multiline Text")
    print("=" * 60)
    
    text = """Line 1
Line 2
Line 3"""
    
    braille = text_to_braille(text, preserve_newlines=True)
    
    print("Original:")
    print(text)
    print("\nBraille:")
    print(braille)
    print()


def example_6_alphabet_showcase():
    """Example 6: Complete alphabet."""
    print("=" * 60)
    print("Example 6: Complete Alphabet")
    print("=" * 60)
    
    lowercase = "abcdefghijklmnopqrstuvwxyz"
    uppercase = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    digits = "0123456789"
    
    print(f"Lowercase: {lowercase}")
    print(f"Braille:   {text_to_braille(lowercase)}")
    print()
    print(f"Uppercase: {uppercase}")
    print(f"Braille:   {text_to_braille(uppercase)}")
    print()
    print(f"Digits:    {digits}")
    print(f"Braille:   {text_to_braille(digits)}")
    print()


def main():
    """Run all examples."""
    example_1_simple_conversion()
    example_2_character_details()
    example_3_numbers_and_punctuation()
    example_4_analysis()
    example_5_multiline()
    example_6_alphabet_showcase()
    
    print("=" * 60)
    print("All examples completed!")
    print("=" * 60)


if __name__ == '__main__':
    main()

