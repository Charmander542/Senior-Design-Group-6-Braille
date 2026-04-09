"""
Quick TCP test client for the ESP32-S3 8-dot Braille sketch.

Usage:
  1. Flash arduino_8dot_braille.ino and open the Serial Monitor.
  2. Note the IP address printed after "WIFI:Connected IP=".
  3. Run:  python wifi_test.py <IP_ADDRESS>
     e.g.  python wifi_test.py 10.0.0.42

You'll get an interactive prompt to type commands:
  PING, HELP, TEST, DOTS:1,2,3,7, DOTS:NONE, etc.
Type 'quit' or Ctrl-C to exit.
"""

import socket
import sys
import threading

ESP32_PORT = 3333
TIMEOUT = 5

def receiver(sock):
    """Print everything the ESP32 sends back."""
    try:
        while True:
            data = sock.recv(1024)
            if not data:
                print("\n[Disconnected from ESP32]")
                break
            print(data.decode("utf-8", errors="replace"), end="")
    except (OSError, ConnectionResetError):
        pass

def main():
    if len(sys.argv) < 2:
        print("Usage: python wifi_test.py <ESP32_IP>")
        print("  Find the IP in the Serial Monitor after flashing.")
        sys.exit(1)

    host = sys.argv[1]
    print(f"Connecting to {host}:{ESP32_PORT} ...")

    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.settimeout(TIMEOUT)
    try:
        sock.connect((host, ESP32_PORT))
    except (socket.timeout, ConnectionRefusedError) as e:
        print(f"Could not connect: {e}")
        sys.exit(1)

    sock.settimeout(None)
    print("Connected! Type commands (PING, HELP, TEST, DOTS:1,2,3, DOTS:NONE)")
    print("Type 'quit' to exit.\n")

    rx = threading.Thread(target=receiver, args=(sock,), daemon=True)
    rx.start()

    try:
        while True:
            cmd = input(">>> ")
            if cmd.strip().lower() == "quit":
                break
            sock.sendall((cmd + "\n").encode("utf-8"))
    except (KeyboardInterrupt, EOFError):
        print()
    finally:
        sock.close()
        print("Disconnected.")

if __name__ == "__main__":
    main()
