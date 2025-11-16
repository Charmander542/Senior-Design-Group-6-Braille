"""
Send braille patterns to Arduino over serial.

This script converts text to braille and sends the dot patterns to Arduino.
"""

import sys
import time
import serial
import serial.tools.list_ports

# Add braille converter to path
sys.path.insert(0, 'path-to-this-project')
from braille_converter import BrailleConverter


class ArduinoBrailleInterface:
    """Interface for sending braille patterns to Arduino."""
    
    def __init__(self, port=None, baud_rate=115200):
        """
        Initialize Arduino connection.
        
        Args:
            port: Serial port (e.g., '/dev/cu.usbmodem14201' or 'COM3')
            baud_rate: Serial baud rate (default: 115200)
        """
        self.port = port
        self.baud_rate = baud_rate
        self.serial = None
        self.converter = BrailleConverter()
        
        if port:
            self.connect(port)
    
    def list_ports(self):
        """List available serial ports."""
        ports = serial.tools.list_ports.comports()
        available = []
        
        print("Available serial ports:")
        for i, port in enumerate(ports):
            print(f"  [{i}] {port.device} - {port.description}")
            available.append(port.device)
        
        return available
    
    def connect(self, port):
        """Connect to Arduino on specified port."""
        try:
            self.serial = serial.Serial(
                port=port,
                baudrate=self.baud_rate,
                bytesize=serial.EIGHTBITS,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE,
                timeout=1
            )
            time.sleep(2)  # Wait for Arduino to reset
            print(f"✓ Connected to Arduino on {port}")
            return True
        except serial.SerialException as e:
            print(f"✗ Failed to connect: {e}")
            return False
    
    def disconnect(self):
        """Disconnect from Arduino."""
        if self.serial and self.serial.is_open:
            self.serial.close()
            print("✓ Disconnected from Arduino")
    
    def send_dots(self, dots):
        """
        Send dot pattern to Arduino.
        
        Args:
            dots: List of dot numbers (1-6) that should be raised
        
        Format: "DOTS:1,2,3\n" or "DOTS:NONE\n" for space
        """
        if not self.serial or not self.serial.is_open:
            print("✗ Not connected to Arduino")
            return False
        
        if not dots:
            # Empty pattern (space)
            message = "DOTS:NONE\n"
        else:
            # Convert dots list to comma-separated string
            dots_str = ','.join(map(str, sorted(dots)))
            message = f"DOTS:{dots_str}\n"
        
        try:
            self.serial.write(message.encode('utf-8'))
            self.serial.flush()
            
            # Wait for acknowledgment (optional)
            time.sleep(0.05)
            
            return True
        except serial.SerialException as e:
            print(f"✗ Send failed: {e}")
            return False
    
    def send_char(self, char, delay=0.5):
        """
        Convert character to braille and send to Arduino.
        
        Args:
            char: Character to send
            delay: Delay in seconds between characters
        """
        braille_char = self.converter.convert_char(char)
        
        print(f"Sending '{char}' → {braille_char.braille} (dots: {braille_char.dots})")
        
        success = self.send_dots(braille_char.dots)
        
        if success and delay > 0:
            time.sleep(delay)
        
        return success
    
    def send_text(self, text, delay=0.5, clear_between=True):
        """
        Convert text to braille and send each character to Arduino.
        
        Args:
            text: Text to convert and send
            delay: Delay between characters in seconds
            clear_between: Whether to clear display between characters
        """
        print(f"\nSending text: '{text}'")
        print("=" * 60)
        
        braille_chars = self.converter.convert_text_detailed(text)
        
        for i, bc in enumerate(braille_chars):
            if bc.original == '\n':
                print("  [newline - skipping]")
                continue
            
            print(f"[{i+1}/{len(braille_chars)}] ", end="")
            self.send_char(bc.original, delay=delay)
            
            # Optionally clear display between characters
            if clear_between and i < len(braille_chars) - 1:
                time.sleep(delay * 0.5)
        
        print("\n✓ Text sent successfully")
    
    def send_text_continuous(self, text, char_delay=1.0):
        """
        Send text as continuous braille stream.
        
        Args:
            text: Text to send
            char_delay: Time to display each character
        """
        print(f"\nSending continuous stream: '{text}'")
        print("Press Ctrl+C to stop")
        print("=" * 60)
        
        try:
            braille_text = self.converter.convert_text(text)
            braille_chars = self.converter.convert_text_detailed(text)
            
            print(f"Braille: {braille_text}\n")
            
            for i, bc in enumerate(braille_chars):
                if bc.original == '\n':
                    continue
                
                print(f"[{i+1}] {bc.original} → {bc.braille} (dots: {bc.dots})")
                self.send_dots(bc.dots)
                time.sleep(char_delay)
            
            print("\n✓ Stream completed")
            
        except KeyboardInterrupt:
            print("\n\n⚠ Stream stopped by user")
    
    def demo_character(self, char):
        """
        Demonstrate a single character with visual pattern.
        
        Args:
            char: Character to demonstrate
        """
        bc = self.converter.convert_char(char)
        
        print("\n" + "=" * 60)
        print(f"Character: '{bc.original}'")
        print(f"Braille:   {bc.braille}")
        print(f"Dots:      {bc.dots}")
        print("\nPattern:")
        print(bc.get_pattern())
        print("=" * 60)
        
        self.send_dots(bc.dots)
        print("✓ Sent to Arduino")


def interactive_mode():
    """Run in interactive mode."""
    print("=" * 60)
    print("  Arduino Braille Interface - Interactive Mode")
    print("=" * 60)
    
    # Create interface
    interface = ArduinoBrailleInterface()
    
    # List and select port
    ports = interface.list_ports()
    
    if not ports:
        print("\n✗ No serial ports found!")
        return
    
    print("\nSelect port number: ", end="")
    try:
        selection = int(input().strip())
        if 0 <= selection < len(ports):
            selected_port = ports[selection]
        else:
            print("Invalid selection")
            return
    except ValueError:
        print("Invalid input")
        return
    
    # Connect
    if not interface.connect(selected_port):
        return
    
    print("\n" + "=" * 60)
    print("Commands:")
    print("  char <c>  - Send single character")
    print("  text <t>  - Send text with delays")
    print("  demo <c>  - Demo character with pattern")
    print("  dots <d>  - Send raw dot pattern (e.g., 1,2,3)")
    print("  quit      - Exit")
    print("=" * 60)
    
    try:
        while True:
            print("\n> ", end="")
            cmd = input().strip()
            
            if not cmd:
                continue
            
            parts = cmd.split(None, 1)
            command = parts[0].lower()
            
            if command == 'quit':
                break
            
            elif command == 'char' and len(parts) > 1:
                char = parts[1][0]
                interface.send_char(char, delay=0)
            
            elif command == 'text' and len(parts) > 1:
                text = parts[1]
                interface.send_text(text, delay=1.0)
            
            elif command == 'demo' and len(parts) > 1:
                char = parts[1][0]
                interface.demo_character(char)
            
            elif command == 'dots' and len(parts) > 1:
                try:
                    dots = [int(d.strip()) for d in parts[1].split(',')]
                    print(f"Sending dots: {dots}")
                    interface.send_dots(dots)
                    print("✓ Sent")
                except ValueError:
                    print("✗ Invalid dot pattern. Use format: 1,2,3")
            
            else:
                print("✗ Unknown command or missing argument")
    
    except KeyboardInterrupt:
        print("\n\n⚠ Interrupted by user")
    
    finally:
        interface.disconnect()


def main():
    """Main entry point."""
    if len(sys.argv) > 1:
        # Command-line mode
        if sys.argv[1] == '--list':
            interface = ArduinoBrailleInterface()
            interface.list_ports()
        
        elif sys.argv[1] == '--port' and len(sys.argv) > 3:
            port = sys.argv[2]
            text = sys.argv[3]
            
            interface = ArduinoBrailleInterface(port)
            interface.send_text(text, delay=1.0)
            interface.disconnect()
        
        else:
            print("Usage:")
            print("  python braille_to_arduino.py                    # Interactive mode")
            print("  python braille_to_arduino.py --list              # List ports")
            print("  python braille_to_arduino.py --port PORT TEXT    # Send text")
            print("\nExamples:")
            print("  python braille_to_arduino.py --list")
            print("  python braille_to_arduino.py --port /dev/cu.usbmodem14201 'Hello'")
    
    else:
        # Interactive mode
        interactive_mode()


if __name__ == '__main__':
    main()

