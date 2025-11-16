#!/usr/bin/env python3
"""
Quick demo script for the Braille Converter library.
Run this script to see the library in action!
"""

from braille_converter import text_to_braille, BrailleConverter


def print_separator(title=""):
    """Print a nice separator."""
    if title:
        print(f"\n{'='*60}")
        print(f"  {title}")
        print(f"{'='*60}\n")
    else:
        print(f"{'='*60}\n")


def main():
    """Run the demo."""
    print_separator("🔤 BRAILLE CONVERTER DEMO 🔤")
    
    # Demo 1: Simple text
    print("1. Simple Text Conversion")
    print("-" * 60)
    text1 = "Hello World!"
    braille1 = text_to_braille(text1)
    print(f"Input:   {text1}")
    print(f"Braille: {braille1}")
    
    # Demo 2: Your name
    print_separator()
    print("2. Personalized Message")
    print("-" * 60)
    text2 = "Welcome to EC463 Senior Design!"
    braille2 = text_to_braille(text2)
    print(f"Input:   {text2}")
    print(f"Braille: {braille2}")
    
    # Demo 3: Numbers
    print_separator()
    print("3. Numbers and Symbols")
    print("-" * 60)
    text3 = "Room 123, Call: 555-1234"
    braille3 = text_to_braille(text3)
    print(f"Input:   {text3}")
    print(f"Braille: {braille3}")
    
    # Demo 4: Character breakdown
    print_separator()
    print("4. Character-by-Character Breakdown")
    print("-" * 60)
    converter = BrailleConverter()
    text4 = "Hi!"
    
    print(f"Analyzing: '{text4}'")
    print()
    
    for char_obj in converter.convert_text_detailed(text4):
        if char_obj.original != '\n':
            print(f"Character: '{char_obj.original}'")
            print(f"Braille:   {char_obj.braille}")
            print(f"Dots:      {char_obj.dots}")
            print()
    
    # Demo 5: Dot pattern visualization
    print_separator()
    print("5. Visual Dot Pattern (Letter 'A')")
    print("-" * 60)
    char_a = converter.convert_char('A')
    print(f"Character: {char_a.original}")
    print(f"Braille:   {char_a.braille}")
    print(f"Dots:      {char_a.dots}")
    print(f"\nDot Pattern (2x3 grid):")
    print(char_a.get_pattern())
    
    # Demo 6: Alphabet
    print_separator()
    print("6. Complete Alphabet")
    print("-" * 60)
    alphabet = "abcdefghijklmnopqrstuvwxyz"
    braille_alphabet = text_to_braille(alphabet)
    print(f"Lowercase: {alphabet}")
    print(f"Braille:   {braille_alphabet}")
    
    # Demo 7: Statistics
    print_separator()
    print("7. Text Analysis with Statistics")
    print("-" * 60)
    text7 = "Python 3.11 is great for EC463!"
    analysis = converter.convert_and_analyze(text7)
    print(f"Original:  {analysis['original_text']}")
    print(f"Braille:   {analysis['braille_text']}")
    print(f"\nStatistics:")
    for key, value in analysis['statistics'].items():
        print(f"  • {key.replace('_', ' ').title()}: {value}")
    
    # Final message
    print_separator()
    print("✅ Demo complete!")
    print("\nNext steps:")
    print("  • Check out examples/ directory for more examples")
    print("  • Read README.md for full documentation")
    print("  • Try: from braille_converter import text_to_braille")
    print("  • Use the CLI: text-to-braille --help")
    print_separator()


if __name__ == '__main__':
    main()

