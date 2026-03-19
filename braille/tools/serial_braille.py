#!/usr/bin/env python3
"""
Send Braille patterns to an Arduino over serial to drive 8 LEDs.

Protocol:
  PC  -> Arduino:  "P:XX\n"   (XX = 2-digit hex pattern)
  Arduino -> PC:   "OK\n"     (acknowledgement)
  PC  -> Arduino:  "CLEAR\n"  (turn off all LEDs)
  PC  -> Arduino:  "PING\n"   / Arduino -> "PONG\n"
  PC  -> Arduino:  "TEST\n"   (sweep all LEDs)

Usage:
  python serial_braille.py                           # interactive mode
  python serial_braille.py "hello world"             # convert a string
  python serial_braille.py --file story.txt          # convert a text file
  python serial_braille.py --port /dev/cu.usbmodem1  # specify serial port
  python serial_braille.py --delay 800               # ms between characters
  python serial_braille.py --list-ports              # list available ports
"""

import sys
import time
import argparse
from pathlib import Path

_SCRIPT_DIR = Path(__file__).resolve().parent
if str(_SCRIPT_DIR) not in sys.path:
    sys.path.insert(0, str(_SCRIPT_DIR))

try:
    import serial
    import serial.tools.list_ports
except ImportError:
    print(
        "Missing dependency: pyserial\n"
        "Install with:  pip install pyserial",
        file=sys.stderr,
    )
    sys.exit(1)

from braille import char_to_braille, NUMBER_INDICATOR_PATTERN, print_cell


def find_arduino_port() -> str | None:
    """Auto-detect an Arduino serial port."""
    ports = serial.tools.list_ports.comports()
    for p in ports:
        desc = (p.description or "").lower()
        mfr = (p.manufacturer or "").lower()
        if any(
            kw in desc or kw in mfr
            for kw in ("arduino", "ch340", "cp210", "ftdi", "usbmodem", "usbserial")
        ):
            return p.device
    for p in ports:
        if "usbmodem" in p.device or "usbserial" in p.device or "ttyACM" in p.device or "ttyUSB" in p.device:
            return p.device
    return None


def list_ports():
    """Print all available serial ports."""
    ports = serial.tools.list_ports.comports()
    if not ports:
        print("No serial ports found.")
        return
    print(f"{'Port':<30} {'Description':<40} {'Manufacturer'}")
    print("-" * 100)
    for p in ports:
        print(f"{p.device:<30} {(p.description or ''):<40} {p.manufacturer or ''}")


class ArduinoBraille:
    """Manages the serial connection and sends braille patterns."""

    def __init__(self, port: str, baud: int = 115200, timeout: float = 3.0):
        self.ser = serial.Serial(port, baud, timeout=timeout)
        self._wait_ready()

    def _wait_ready(self):
        """Wait for Arduino to boot and send BRAILLE_LED_READY."""
        print(f"Connecting to {self.ser.port} @ {self.ser.baudrate} baud ...")
        deadline = time.time() + 5.0
        while time.time() < deadline:
            line = self.ser.readline().decode("ascii", errors="replace").strip()
            if line:
                print(f"  <- {line}")
            if "BRAILLE_LED_READY" in line:
                print("Arduino ready!\n")
                return
        print("Warning: did not receive BRAILLE_LED_READY (continuing anyway)\n")

    def _send(self, cmd: str) -> str:
        """Send a command and return the response line."""
        self.ser.reset_input_buffer()
        self.ser.write(f"{cmd}\n".encode("ascii"))
        self.ser.flush()
        resp = self.ser.readline().decode("ascii", errors="replace").strip()
        return resp

    def send_pattern(self, pattern: int) -> bool:
        resp = self._send(f"P:{pattern:02X}")
        # Drain any visualization lines the Arduino sends
        while True:
            extra = self.ser.readline().decode("ascii", errors="replace").strip()
            if not extra:
                break
            if extra == "OK":
                return True
        return "OK" in resp

    def clear(self):
        self._send("CLEAR")

    def ping(self) -> bool:
        return self._send("PING") == "PONG"

    def test(self):
        print("Running LED sweep test ...")
        self._send("TEST")
        # Wait for sweep to finish
        time.sleep(2)
        print("Test complete.\n")

    def close(self):
        self.clear()
        self.ser.close()


def send_text(ab: ArduinoBraille, text: str, delay_ms: int = 600):
    """Convert text to braille and send each pattern to the Arduino."""
    cleaned = "".join(
        c for c in text if c == "\n" or (32 <= ord(c) <= 126)
    )
    if not cleaned.strip():
        print("(No printable text to send.)")
        return

    total = sum(1 for c in cleaned if c != "\n")
    idx = 0
    in_number_mode = False

    print("========================================")
    print("  SENDING BRAILLE TO LEDs")
    print("========================================")
    print(f"Text: \"{cleaned[:80]}{'...' if len(cleaned) > 80 else ''}\"")
    print(f"Characters: {total}  |  Delay: {delay_ms} ms")
    print("========================================\n")

    for c in cleaned:
        if c == "\n":
            print("  [newline] — clearing LEDs")
            ab.clear()
            time.sleep(delay_ms / 1000.0)
            continue

        is_digit = c.isdigit()
        if is_digit and not in_number_mode:
            print("  >> Number Indicator")
            print_cell(NUMBER_INDICATOR_PATTERN, "  #NUM")
            ab.send_pattern(NUMBER_INDICATOR_PATTERN)
            time.sleep(delay_ms / 1000.0)
            in_number_mode = True
        elif not is_digit and in_number_mode:
            in_number_mode = False

        idx += 1
        pattern = char_to_braille(c)
        label = f"'{c}'" if 33 <= ord(c) <= 126 else ("'SPACE'" if c == " " else f"0x{ord(c):02X}")

        print(f"  [{idx}/{total}] {label}  ->  0b{pattern:08b}  (0x{pattern:02X})")
        print_cell(pattern)

        ab.send_pattern(pattern)
        time.sleep(delay_ms / 1000.0)

    print("\n========================================")
    print("  DONE — clearing LEDs")
    print("========================================\n")
    ab.clear()


def interactive_mode(ab: ArduinoBraille, delay_ms: int):
    """REPL loop: type text, see it on the LEDs."""
    print("========================================")
    print("  INTERACTIVE BRAILLE LED MODE")
    print("========================================")
    print("Type text and press Enter to display on LEDs.")
    print("Commands:  /test  /clear  /ping  /quit")
    print("========================================\n")

    while True:
        try:
            text = input("braille> ").strip()
        except (EOFError, KeyboardInterrupt):
            print()
            break

        if not text:
            continue
        if text.lower() == "/quit":
            break
        if text.lower() == "/test":
            ab.test()
            continue
        if text.lower() == "/clear":
            ab.clear()
            print("LEDs cleared.\n")
            continue
        if text.lower() == "/ping":
            ok = ab.ping()
            print(f"{'PONG — connection OK' if ok else 'No response!'}\n")
            continue

        send_text(ab, text, delay_ms)


def main():
    parser = argparse.ArgumentParser(description="Send Braille patterns to Arduino LEDs over serial")
    parser.add_argument("text", nargs="?", help="Text to convert and send")
    parser.add_argument("--file", "-f", help="Read text from a file")
    parser.add_argument("--port", "-p", help="Serial port (auto-detected if omitted)")
    parser.add_argument("--baud", "-b", type=int, default=115200, help="Baud rate (default: 115200)")
    parser.add_argument("--delay", "-d", type=int, default=600, help="Delay between characters in ms (default: 600)")
    parser.add_argument("--list-ports", action="store_true", help="List available serial ports and exit")
    parser.add_argument("--test", action="store_true", help="Run LED sweep test and exit")
    args = parser.parse_args()

    if args.list_ports:
        list_ports()
        return

    port = args.port or find_arduino_port()
    if not port:
        print(
            "Error: No Arduino found. Use --port to specify, or --list-ports to see available ports.",
            file=sys.stderr,
        )
        sys.exit(1)

    ab = ArduinoBraille(port, args.baud)

    try:
        if args.test:
            ab.test()
            return

        if args.file:
            text = Path(args.file).read_text(encoding="utf-8")
            send_text(ab, text, args.delay)
        elif args.text:
            send_text(ab, args.text, args.delay)
        else:
            interactive_mode(ab, args.delay)
    except KeyboardInterrupt:
        print("\nInterrupted.")
    finally:
        ab.close()
        print("Serial port closed.")


if __name__ == "__main__":
    main()
