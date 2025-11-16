"""
Command-line interface for the Braille Converter library.
"""

import argparse
import sys
from pathlib import Path
from .converter import BrailleConverter


def main():
    """Main CLI entry point."""
    parser = argparse.ArgumentParser(
        description="Convert text files to 6-dot braille representation",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Convert a file and print to stdout
  text-to-braille input.txt
  
  # Convert a file and save to output
  text-to-braille input.txt -o output_braille.txt
  
  # Convert text directly
  text-to-braille -t "Hello World"
  
  # Show detailed analysis
  text-to-braille -t "ABC" --analyze
        """
    )
    
    parser.add_argument(
        'input',
        nargs='?',
        help='Input text file to convert'
    )
    
    parser.add_argument(
        '-t', '--text',
        help='Convert text directly instead of a file'
    )
    
    parser.add_argument(
        '-o', '--output',
        help='Output file path (optional, prints to stdout if not specified)'
    )
    
    parser.add_argument(
        '-e', '--encoding',
        default='utf-8',
        help='File encoding (default: utf-8)'
    )
    
    parser.add_argument(
        '--analyze',
        action='store_true',
        help='Show detailed analysis of the conversion'
    )
    
    parser.add_argument(
        '--pattern',
        action='store_true',
        help='Show visual dot patterns for each character'
    )
    
    parser.add_argument(
        '-v', '--version',
        action='version',
        version='%(prog)s 1.0.0'
    )
    
    args = parser.parse_args()
    
    # Validate arguments
    if not args.text and not args.input:
        parser.error("Either provide an input file or use -t/--text option")
        return 1
    
    if args.text and args.input:
        parser.error("Cannot specify both input file and -t/--text option")
        return 1
    
    converter = BrailleConverter()
    
    try:
        # Convert text or file
        if args.text:
            text = args.text
            if args.analyze:
                analysis = converter.convert_and_analyze(text)
                print(f"Original Text: {analysis['original_text']}")
                print(f"Braille: {analysis['braille_text']}")
                print(f"\nStatistics:")
                for key, value in analysis['statistics'].items():
                    print(f"  {key}: {value}")
                
                if args.pattern:
                    print("\nCharacter Details:")
                    for char_info in analysis['characters']:
                        if char_info['original'] != '\n':
                            print(f"\n'{char_info['original']}' -> {char_info['braille']}")
                            print(f"Dots: {char_info['dots']}")
            else:
                braille = converter.convert_text(text)
                if args.pattern:
                    braille_chars = converter.convert_text_detailed(text)
                    for bc in braille_chars:
                        if bc.original != '\n':
                            print(f"\n'{bc.original}' -> {bc.braille}")
                            print(bc.get_pattern())
                else:
                    print(braille)
                
                if args.output:
                    with open(args.output, 'w', encoding=args.encoding) as f:
                        f.write(braille)
                    print(f"\nSaved to: {args.output}", file=sys.stderr)
        
        else:  # File input
            input_path = Path(args.input)
            if not input_path.exists():
                print(f"Error: File not found: {input_path}", file=sys.stderr)
                return 1
            
            braille = converter.convert_file(
                input_path,
                output_path=args.output,
                encoding=args.encoding
            )
            
            if args.analyze:
                with open(input_path, 'r', encoding=args.encoding) as f:
                    text = f.read()
                analysis = converter.convert_and_analyze(text)
                print(f"File: {input_path}")
                print(f"\nStatistics:")
                for key, value in analysis['statistics'].items():
                    print(f"  {key}: {value}")
                print(f"\nBraille output:")
            
            if not args.output:
                print(braille)
            else:
                print(f"Converted {input_path} -> {args.output}", file=sys.stderr)
        
        return 0
    
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        return 1


if __name__ == '__main__':
    sys.exit(main())

