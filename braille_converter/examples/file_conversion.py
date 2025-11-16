"""
File conversion examples for the Braille Converter library.
"""

from pathlib import Path
from braille_converter import file_to_braille, BrailleConverter


def create_sample_files():
    """Create sample text files for demonstration."""
    examples_dir = Path(__file__).parent
    samples_dir = examples_dir / "sample_texts"
    samples_dir.mkdir(exist_ok=True)
    
    # Sample 1: Simple greeting
    sample1 = samples_dir / "greeting.txt"
    sample1.write_text("Hello! Welcome to the Braille Converter.\nThis is a new line.")
    
    # Sample 2: Mixed content
    sample2 = samples_dir / "mixed_content.txt"
    sample2.write_text("""Python Programming 101

Python is a powerful language.
Version: 3.11
Contact: support@example.com

Key Features:
- Easy to learn
- Versatile
- Great community!""")
    
    # Sample 3: Numbers and symbols
    sample3 = samples_dir / "numbers_symbols.txt"
    sample3.write_text("""Math Expressions:
1 + 2 = 3
10 - 5 = 5
3 * 4 = 12
20 / 4 = 5

Special: @#$%&*()""")
    
    print(f"Created sample files in: {samples_dir}")
    return samples_dir


def example_1_basic_file_conversion():
    """Example 1: Basic file conversion."""
    print("=" * 60)
    print("Example 1: Basic File Conversion")
    print("=" * 60)
    
    samples_dir = create_sample_files()
    input_file = samples_dir / "greeting.txt"
    output_file = samples_dir / "greeting_braille.txt"
    
    # Read original
    original = input_file.read_text()
    print(f"Original text from {input_file.name}:")
    print(original)
    print()
    
    # Convert
    braille = file_to_braille(input_file, output_file)
    print(f"Braille output:")
    print(braille)
    print(f"\nSaved to: {output_file}")
    print()


def example_2_batch_conversion():
    """Example 2: Batch file conversion."""
    print("=" * 60)
    print("Example 2: Batch File Conversion")
    print("=" * 60)
    
    samples_dir = create_sample_files()
    output_dir = samples_dir.parent / "braille_output"
    output_dir.mkdir(exist_ok=True)
    
    converter = BrailleConverter()
    
    # Convert all txt files
    for txt_file in samples_dir.glob("*.txt"):
        if "braille" not in txt_file.stem:  # Skip already converted files
            output_file = output_dir / f"{txt_file.stem}_braille.txt"
            converter.convert_file(txt_file, output_file)
            print(f"✓ Converted: {txt_file.name} -> {output_file.name}")
    
    print(f"\nAll files saved to: {output_dir}")
    print()


def example_3_detailed_file_analysis():
    """Example 3: Detailed file analysis."""
    print("=" * 60)
    print("Example 3: Detailed File Analysis")
    print("=" * 60)
    
    samples_dir = create_sample_files()
    input_file = samples_dir / "mixed_content.txt"
    
    converter = BrailleConverter()
    
    # Read and analyze
    with open(input_file, 'r') as f:
        text = f.read()
    
    analysis = converter.convert_and_analyze(text)
    
    print(f"File: {input_file.name}")
    print(f"\nStatistics:")
    for key, value in analysis['statistics'].items():
        print(f"  {key.replace('_', ' ').title()}: {value}")
    
    print(f"\nFirst 100 characters of braille output:")
    print(analysis['braille_text'][:100] + "...")
    print()


def example_4_conversion_with_encoding():
    """Example 4: Handle different encodings."""
    print("=" * 60)
    print("Example 4: Conversion with Different Encodings")
    print("=" * 60)
    
    samples_dir = create_sample_files()
    input_file = samples_dir / "greeting.txt"
    
    converter = BrailleConverter()
    
    # Convert with explicit encoding
    braille = converter.convert_file(input_file, encoding='utf-8')
    
    print(f"Successfully converted with UTF-8 encoding")
    print(f"Braille output: {braille}")
    print()


def example_5_compare_original_and_braille():
    """Example 5: Side-by-side comparison."""
    print("=" * 60)
    print("Example 5: Side-by-Side Comparison")
    print("=" * 60)
    
    samples_dir = create_sample_files()
    input_file = samples_dir / "numbers_symbols.txt"
    
    with open(input_file, 'r') as f:
        original_lines = f.readlines()
    
    converter = BrailleConverter()
    
    print(f"{'Original':<40} {'Braille':<40}")
    print("-" * 80)
    
    for line in original_lines:
        line = line.rstrip('\n')
        braille_line = converter.convert_text(line, preserve_newlines=False)
        print(f"{line:<40} {braille_line:<40}")
    
    print()


def main():
    """Run all file conversion examples."""
    example_1_basic_file_conversion()
    example_2_batch_conversion()
    example_3_detailed_file_analysis()
    example_4_conversion_with_encoding()
    example_5_compare_original_and_braille()
    
    print("=" * 60)
    print("All file conversion examples completed!")
    print("=" * 60)


if __name__ == '__main__':
    main()

