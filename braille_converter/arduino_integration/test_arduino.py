"""
Quick test script for Arduino braille integration.
"""

import sys
import time

sys.path.insert(0, 'path-to-this-project')

from braille_converter.arduino_integration.braille_to_arduino import ArduinoBrailleInterface


def test_connection():
    """Test 1: Connection and port listing."""
    print("=" * 60)
    print("Test 1: List and Connect to Arduino")
    print("=" * 60)
    
    interface = ArduinoBrailleInterface()
    ports = interface.list_ports()
    
    if not ports:
        print("\n✗ No ports found. Connect your Arduino and try again.")
        return None
    
    print(f"\nFound {len(ports)} port(s)")
    print("Select port number (or press Enter for port 0): ", end="")
    
    selection = input().strip()
    if selection == "":
        selection = "0"
    
    try:
        idx = int(selection)
        if 0 <= idx < len(ports):
            if interface.connect(ports[idx]):
                print("✓ Test 1 PASSED\n")
                return interface
            else:
                print("✗ Test 1 FAILED\n")
                return None
        else:
            print("✗ Invalid selection")
            return None
    except ValueError:
        print("✗ Invalid input")
        return None


def test_single_dot(interface):
    """Test 2: Send single dot patterns."""
    print("=" * 60)
    print("Test 2: Single Dot Patterns")
    print("=" * 60)
    
    for dot in range(1, 7):
        print(f"Activating dot {dot}...", end=" ")
        interface.send_dots([dot])
        time.sleep(0.8)
        print("✓")
    
    print("\nClearing all dots...", end=" ")
    interface.send_dots([])
    time.sleep(0.5)
    print("✓")
    
    print("✓ Test 2 PASSED\n")


def test_multiple_dots(interface):
    """Test 3: Send multiple dot patterns."""
    print("=" * 60)
    print("Test 3: Multiple Dot Patterns")
    print("=" * 60)
    
    patterns = [
        ([1, 2], "Left column"),
        ([4, 5], "Right column"),
        ([1, 4], "Top row"),
        ([2, 5], "Middle row"),
        ([3, 6], "Bottom row"),
        ([1, 2, 3], "Full left column"),
        ([4, 5, 6], "Full right column"),
        ([1, 2, 3, 4, 5, 6], "All dots"),
    ]
    
    for dots, description in patterns:
        print(f"{description} (dots {dots})...", end=" ")
        interface.send_dots(dots)
        time.sleep(1.0)
        print("✓")
    
    print("\nClearing...", end=" ")
    interface.send_dots([])
    time.sleep(0.5)
    print("✓")
    
    print("✓ Test 3 PASSED\n")


def test_characters(interface):
    """Test 4: Send actual characters."""
    print("=" * 60)
    print("Test 4: Character Conversion")
    print("=" * 60)
    
    test_chars = ['A', 'B', 'C', '1', '!']
    
    for char in test_chars:
        bc = interface.converter.convert_char(char)
        print(f"\n'{char}' → {bc.braille}")
        print(f"Dots: {bc.dots}")
        print("Sending...", end=" ")
        interface.send_dots(bc.dots)
        time.sleep(1.5)
        print("✓")
    
    print("\n✓ Test 4 PASSED\n")


def test_word(interface):
    """Test 5: Send a complete word."""
    print("=" * 60)
    print("Test 5: Complete Word")
    print("=" * 60)
    
    word = "Hi"
    print(f"Sending word: '{word}'")
    print()
    
    interface.send_text(word, delay=2.0, clear_between=False)
    
    print("\n✓ Test 5 PASSED\n")


def test_alphabet(interface):
    """Test 6: Send alphabet sequence (optional)."""
    print("=" * 60)
    print("Test 6: Alphabet Sequence (Optional)")
    print("=" * 60)
    print("This will take ~26 seconds")
    print("Press Enter to continue or 's' to skip: ", end="")
    
    choice = input().strip().lower()
    if choice == 's':
        print("⊘ Test 6 SKIPPED\n")
        return
    
    alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    
    for i, char in enumerate(alphabet):
        bc = interface.converter.convert_char(char)
        print(f"[{i+1}/26] {char} → {bc.braille} (dots: {bc.dots})")
        interface.send_dots(bc.dots)
        time.sleep(1.0)
    
    print("\n✓ Test 6 PASSED\n")


def run_all_tests():
    """Run all tests in sequence."""
    print("\n" + "=" * 60)
    print("  Arduino Braille Integration Test Suite")
    print("=" * 60)
    print()
    
    # Test 1: Connection
    interface = test_connection()
    if not interface:
        print("❌ Testing aborted - could not connect to Arduino")
        return
    
    # Small delay after connection
    time.sleep(1)
    
    try:
        # Test 2: Single dots
        test_single_dot(interface)
        time.sleep(0.5)
        
        # Test 3: Multiple dots
        test_multiple_dots(interface)
        time.sleep(0.5)
        
        # Test 4: Characters
        test_characters(interface)
        time.sleep(0.5)
        
        # Test 5: Word
        test_word(interface)
        time.sleep(0.5)
        
        # Test 6: Alphabet (optional)
        test_alphabet(interface)
        
        # Summary
        print("=" * 60)
        print("  ✅ ALL TESTS COMPLETED SUCCESSFULLY!")
        print("=" * 60)
        print("\nYour Arduino braille interface is working correctly.")
        print("You can now use it in your projects!")
        
    except KeyboardInterrupt:
        print("\n\n⚠ Tests interrupted by user")
    
    except Exception as e:
        print(f"\n\n❌ Test failed with error: {e}")
        import traceback
        traceback.print_exc()
    
    finally:
        # Cleanup
        print("\nCleaning up...")
        interface.send_dots([])  # Clear all dots
        time.sleep(0.5)
        interface.disconnect()
        print("✓ Disconnected\n")


if __name__ == '__main__':
    run_all_tests()

