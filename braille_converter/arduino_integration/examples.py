"""
Example usage scenarios for Arduino braille integration.
"""

import sys
import time

sys.path.insert(0, 'path-to-this-project')

from braille_converter.arduino_integration.braille_to_arduino import ArduinoBrailleInterface


# ============================================================
# CONFIGURATION
# ============================================================

# Change this to your Arduino port
# macOS: '/dev/cu.usbmodem14201' or similar
# Linux: '/dev/ttyACM0' or '/dev/ttyUSB0'
# Windows: 'COM3' or similar
ARDUINO_PORT = '/dev/cu.usbmodem14201'  # <<< CHANGE THIS


# ============================================================
# Example 1: Hello World
# ============================================================

def example_1_hello_world():
    """Send 'Hello!' to Arduino."""
    print("=" * 60)
    print("Example 1: Hello World")
    print("=" * 60)
    
    interface = ArduinoBrailleInterface(ARDUINO_PORT)
    interface.send_text("Hello!", delay=1.5)
    interface.disconnect()
    
    print("✓ Example 1 complete\n")


# ============================================================
# Example 2: Character Demo
# ============================================================

def example_2_character_demo():
    """Demonstrate each character with visual pattern."""
    print("=" * 60)
    print("Example 2: Character Demo with Patterns")
    print("=" * 60)
    
    interface = ArduinoBrailleInterface(ARDUINO_PORT)
    
    for char in "ABC123":
        interface.demo_character(char)
        time.sleep(2)
    
    interface.disconnect()
    
    print("✓ Example 2 complete\n")


# ============================================================
# Example 3: Alphabet Showcase
# ============================================================

def example_3_alphabet():
    """Display entire alphabet."""
    print("=" * 60)
    print("Example 3: Alphabet Showcase")
    print("=" * 60)
    
    interface = ArduinoBrailleInterface(ARDUINO_PORT)
    
    alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    
    print(f"Displaying alphabet ({len(alphabet)} characters)")
    print("This will take about 30 seconds...\n")
    
    for char in alphabet:
        interface.send_char(char, delay=1.0)
    
    interface.disconnect()
    
    print("\n✓ Example 3 complete\n")


# ============================================================
# Example 4: Numbers and Punctuation
# ============================================================

def example_4_numbers_punctuation():
    """Test numbers and punctuation."""
    print("=" * 60)
    print("Example 4: Numbers and Punctuation")
    print("=" * 60)
    
    interface = ArduinoBrailleInterface(ARDUINO_PORT)
    
    text = "Hello! Call 911."
    print(f"Text: '{text}'\n")
    
    interface.send_text(text, delay=1.2)
    interface.disconnect()
    
    print("✓ Example 4 complete\n")


# ============================================================
# Example 5: File to Arduino
# ============================================================

def example_5_file_conversion():
    """Read text from file and send to Arduino."""
    print("=" * 60)
    print("Example 5: File to Arduino")
    print("=" * 60)
    
    # Create sample file
    sample_file = "sample_message.txt"
    with open(sample_file, 'w') as f:
        f.write("EC463 Senior Design")
    
    print(f"Created sample file: {sample_file}")
    
    # Read and send
    with open(sample_file, 'r') as f:
        text = f.read()
    
    print(f"File contents: '{text}'\n")
    
    interface = ArduinoBrailleInterface(ARDUINO_PORT)
    interface.send_text(text, delay=1.0)
    interface.disconnect()
    
    print("✓ Example 5 complete\n")


# ============================================================
# Example 6: Custom Dot Patterns
# ============================================================

def example_6_custom_patterns():
    """Send custom dot patterns."""
    print("=" * 60)
    print("Example 6: Custom Dot Patterns")
    print("=" * 60)
    
    interface = ArduinoBrailleInterface(ARDUINO_PORT)
    
    patterns = [
        ([1], "Dot 1 only"),
        ([1, 2], "Left vertical"),
        ([4, 5], "Right vertical"),
        ([1, 2, 3], "Full left"),
        ([4, 5, 6], "Full right"),
        ([1, 2, 3, 4, 5, 6], "All dots"),
        ([], "Clear (space)"),
    ]
    
    for dots, description in patterns:
        print(f"{description}: {dots}")
        interface.send_dots(dots)
        time.sleep(1.5)
    
    interface.disconnect()
    
    print("\n✓ Example 6 complete\n")


# ============================================================
# Example 7: Scrolling Message
# ============================================================

def example_7_scrolling_message():
    """Display scrolling message."""
    print("=" * 60)
    print("Example 7: Scrolling Message")
    print("=" * 60)
    
    message = "Hello World! Welcome to braille."
    
    interface = ArduinoBrailleInterface(ARDUINO_PORT)
    
    print(f"Message: '{message}'")
    print(f"Length: {len(message)} characters")
    print("Scrolling at 1 second per character...\n")
    
    interface.send_text_continuous(message, char_delay=1.0)
    
    interface.disconnect()
    
    print("✓ Example 7 complete\n")


# ============================================================
# Example 8: Interactive Character Explorer
# ============================================================

def example_8_character_explorer():
    """Let user explore different characters interactively."""
    print("=" * 60)
    print("Example 8: Interactive Character Explorer")
    print("=" * 60)
    
    interface = ArduinoBrailleInterface(ARDUINO_PORT)
    
    print("\nEnter characters to see them in braille on Arduino")
    print("(Type 'quit' to exit)\n")
    
    try:
        while True:
            char = input("Character: ").strip()
            
            if char.lower() == 'quit':
                break
            
            if len(char) == 0:
                continue
            
            if len(char) > 1:
                print("Please enter only one character at a time")
                continue
            
            interface.demo_character(char)
            time.sleep(1)
    
    except KeyboardInterrupt:
        print("\n\nInterrupted")
    
    finally:
        interface.disconnect()
    
    print("✓ Example 8 complete\n")


# ============================================================
# Example 9: Name Displayer
# ============================================================

def example_9_name_displayer():
    """Ask for user's name and display it."""
    print("=" * 60)
    print("Example 9: Name Displayer")
    print("=" * 60)
    
    name = input("Enter your name: ").strip()
    
    if not name:
        print("No name entered")
        return
    
    interface = ArduinoBrailleInterface(ARDUINO_PORT)
    
    print(f"\nDisplaying: '{name}'\n")
    interface.send_text(name, delay=1.5)
    
    interface.disconnect()
    
    print("✓ Example 9 complete\n")


# ============================================================
# Example 10: Morse Code-like Dots
# ============================================================

def example_10_pattern_sequence():
    """Display a sequence of patterns like Morse code."""
    print("=" * 60)
    print("Example 10: Pattern Sequence")
    print("=" * 60)
    
    interface = ArduinoBrailleInterface(ARDUINO_PORT)
    
    # Create a pattern sequence
    sequence = [
        [1],           # Short
        [],            # Gap
        [1, 2, 3],     # Long
        [],            # Gap
        [1],           # Short
        [],            # Gap
        [1],           # Short
        [],            # Gap
        [1, 2, 3],     # Long
        [],            # Gap
        [1, 2, 3],     # Long
        [],            # Gap
    ]
    
    print("Playing pattern sequence...\n")
    
    for i, pattern in enumerate(sequence):
        if pattern:
            print(f"[{i+1}] Dots: {pattern}")
        else:
            print(f"[{i+1}] Clear")
        
        interface.send_dots(pattern)
        time.sleep(0.5)
    
    interface.disconnect()
    
    print("\n✓ Example 10 complete\n")


# ============================================================
# Main Menu
# ============================================================

def main():
    """Display menu and run examples."""
    print("\n" + "=" * 60)
    print("  Arduino Braille Integration Examples")
    print("=" * 60)
    print(f"\nCurrent Arduino port: {ARDUINO_PORT}")
    print("(Edit ARDUINO_PORT in examples.py to change)\n")
    
    examples = [
        ("Hello World", example_1_hello_world),
        ("Character Demo", example_2_character_demo),
        ("Alphabet Showcase", example_3_alphabet),
        ("Numbers and Punctuation", example_4_numbers_punctuation),
        ("File to Arduino", example_5_file_conversion),
        ("Custom Dot Patterns", example_6_custom_patterns),
        ("Scrolling Message", example_7_scrolling_message),
        ("Interactive Character Explorer", example_8_character_explorer),
        ("Name Displayer", example_9_name_displayer),
        ("Pattern Sequence", example_10_pattern_sequence),
    ]
    
    print("Examples:")
    for i, (name, _) in enumerate(examples, 1):
        print(f"  {i}. {name}")
    print("  0. Run all examples")
    print()
    
    try:
        choice = int(input("Select example (0-10): ").strip())
        
        if choice == 0:
            # Run all
            for name, func in examples:
                print(f"\n\n{'='*60}")
                print(f"Running: {name}")
                print('='*60)
                func()
                time.sleep(1)
            
            print("\n" + "=" * 60)
            print("  ✅ All examples completed!")
            print("=" * 60)
        
        elif 1 <= choice <= len(examples):
            examples[choice - 1][1]()
        
        else:
            print("Invalid choice")
    
    except ValueError:
        print("Invalid input")
    except KeyboardInterrupt:
        print("\n\nInterrupted by user")
    except Exception as e:
        print(f"\nError: {e}")
        import traceback
        traceback.print_exc()


if __name__ == '__main__':
    # Check if port is configured
    if ARDUINO_PORT == '/dev/cu.usbmodem14201':
        print("\n⚠ WARNING: Using default Arduino port")
        print("Please edit ARDUINO_PORT in examples.py to match your setup\n")
        print("Run this to find your port:")
        print("  python braille_to_arduino.py --list\n")
        
        response = input("Continue anyway? (y/n): ").strip().lower()
        if response != 'y':
            print("Exiting...")
            sys.exit(0)
    
    main()

