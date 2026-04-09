#!/usr/bin/env python3
"""
send_text_to_esp32.py — Interactive text sender for ESP32 Braille 8-LED test

Usage:
  python send_text_to_esp32.py               # auto-detect port
  python send_text_to_esp32.py /dev/ttyUSB0  # specify port explicitly
  python send_text_to_esp32.py COM3           # Windows

Requires:  pip install pyserial
"""

import sys
import time
import serial
import serial.tools.list_ports


BAUD_RATE = 115200


def find_esp32_port():
    """Auto-detect the most likely ESP32 serial port."""
    ports = serial.tools.list_ports.comports()
    esp_keywords = ("cp210", "ch340", "ch341", "ftdi", "esp32", "uart", "silicon")

    for p in ports:
        desc = (p.description or "").lower()
        mfr  = (p.manufacturer or "").lower()
        if any(k in desc or k in mfr for k in esp_keywords):
            return p.device

    # Fallback: return the first available port
    return ports[0].device if ports else None


def list_ports():
    ports = serial.tools.list_ports.comports()
    if not ports:
        print("No serial ports found.")
        return
    print("Available serial ports:")
    for p in ports:
        print(f"  {p.device}  —  {p.description}")


def read_available(ser):
    """Read all currently buffered lines from the serial port."""
    lines = []
    while ser.in_waiting:
        line = ser.readline().decode(errors="replace").strip()
        if line:
            lines.append(line)
    return lines


def send_command(ser, cmd):
    """Send a command and return any immediately available response lines."""
    ser.write((cmd + "\n").encode())
    ser.flush()
    time.sleep(0.1)
    return read_available(ser)


def wait_for_done(ser, timeout_s):
    """
    Block until 'ACK:TEXT_DONE' or 'ERROR' is received, or timeout expires.
    Prints each line from the ESP32 as it arrives.
    """
    deadline = time.time() + timeout_s
    while time.time() < deadline:
        if ser.in_waiting:
            line = ser.readline().decode(errors="replace").strip()
            if line:
                print(f"  ESP32> {line}")
            if "TEXT_DONE" in line or "ERROR" in line:
                return
        else:
            time.sleep(0.02)
    print("  [timeout — no TEXT_DONE received]")


def print_help():
    print(
        "\nCommands:"
        "\n  <text>          send text as braille (one LED cell per character)"
        "\n  delay <ms>      set display time per character in milliseconds (default 1000)"
        "\n  test            run 8-LED sweep test"
        "\n  dots <1,2,3>    manually set a dot pattern (dots 1-8)"
        "\n  clear           turn off all LEDs"
        "\n  ports           list available serial ports"
        "\n  help            show this message"
        "\n  quit / exit     disconnect and exit\n"
    )


def main():
    if "--list" in sys.argv or "--ports" in sys.argv:
        list_ports()
        return

    port = sys.argv[1] if len(sys.argv) > 1 else find_esp32_port()

    if not port:
        print("No serial port found. Plug in the ESP32 or specify the port as an argument.")
        print("Example: python send_text_to_esp32.py /dev/ttyUSB0")
        sys.exit(1)

    print(f"Connecting to {port} at {BAUD_RATE} baud ...")
    try:
        ser = serial.Serial(port, BAUD_RATE, timeout=2)
    except serial.SerialException as exc:
        print(f"Could not open {port}: {exc}")
        sys.exit(1)

    # Wait for the ESP32 to boot / reset after DTR toggle
    time.sleep(2)

    # Print startup messages
    print(f"Connected to {port}\n")
    for line in read_available(ser):
        print(f"  ESP32> {line}")

    print_help()

    delay_ms = 1000  # keep a local copy to estimate display time

    while True:
        try:
            user_input = input("Enter text > ").strip()
        except (EOFError, KeyboardInterrupt):
            print("\nDisconnecting.")
            break

        if not user_input:
            continue

        cmd = user_input.lower()

        # ── Built-in commands ──────────────────────────────────────────────
        if cmd in ("quit", "exit", "q"):
            print("Disconnecting.")
            break

        elif cmd == "help":
            print_help()

        elif cmd == "ports":
            list_ports()

        elif cmd == "test":
            print("Running LED sweep test ...")
            for line in send_command(ser, "TEST"):
                print(f"  ESP32> {line}")

        elif cmd == "clear":
            for line in send_command(ser, "DOTS:NONE"):
                print(f"  ESP32> {line}")

        elif cmd.startswith("delay "):
            parts = cmd.split()
            if len(parts) == 2 and parts[1].isdigit():
                delay_ms = int(parts[1])
                for line in send_command(ser, f"DELAY:{delay_ms}"):
                    print(f"  ESP32> {line}")
            else:
                print("Usage: delay <milliseconds>   example: delay 500")

        elif cmd.startswith("dots "):
            dot_str = user_input[5:].strip()
            for line in send_command(ser, f"DOTS:{dot_str}"):
                print(f"  ESP32> {line}")

        # ── Treat anything else as text to display ─────────────────────────
        else:
            char_count = len(user_input)
            est_secs   = (char_count * delay_ms) / 1000
            print(
                f"Sending {char_count} character(s) "
                f"(~{est_secs:.1f} s at {delay_ms} ms/char) ..."
            )
            ser.write((f"TEXT:{user_input}\n").encode())
            ser.flush()
            wait_for_done(ser, timeout_s=est_secs + 5)

    ser.close()
    print("Done.")


if __name__ == "__main__":
    main()
