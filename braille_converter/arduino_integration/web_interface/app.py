#!/usr/bin/env python3
"""
Flask web server for the Braille ESP32 LED Controller.

Supports two connection modes:
  - Serial (USB): uses pyserial to talk to the ESP32
  - WiFi  (TCP):  connects to the ESP32's TCP server over the network

Usage:
    pip install -r requirements.txt
    python app.py
Then open http://localhost:5000 in your browser.
"""

from flask import Flask, render_template, jsonify, request, Response
import argparse
import os
import socket
import serial
import serial.tools.list_ports
import threading
import time
import json
import queue

app = Flask(__name__)

# Optional LAN passcode — set ACCESS_CODE=yourcode before starting Flask.
# Leave unset (or empty) to allow anyone on the network to send messages.
ACCESS_CODE = os.environ.get('ACCESS_CODE', '').strip()

# ── Braille mapping ───────────────────────────────────────────────────────────
BRAILLE_MAP = {
    ' ': 0x00,
    'a': 0x01, 'b': 0x03, 'c': 0x09, 'd': 0x19, 'e': 0x11,
    'f': 0x0B, 'g': 0x1B, 'h': 0x13, 'i': 0x0A, 'j': 0x1A,
    'k': 0x05, 'l': 0x07, 'm': 0x0D, 'n': 0x1D, 'o': 0x15,
    'p': 0x0F, 'q': 0x1F, 'r': 0x17, 's': 0x0E, 't': 0x1E,
    'u': 0x25, 'v': 0x27, 'w': 0x3A, 'x': 0x2D, 'y': 0x3D, 'z': 0x35,
    'A': 0x01|0x40, 'B': 0x03|0x40, 'C': 0x09|0x40, 'D': 0x19|0x40, 'E': 0x11|0x40,
    'F': 0x0B|0x40, 'G': 0x1B|0x40, 'H': 0x13|0x40, 'I': 0x0A|0x40, 'J': 0x1A|0x40,
    'K': 0x05|0x40, 'L': 0x07|0x40, 'M': 0x0D|0x40, 'N': 0x1D|0x40, 'O': 0x15|0x40,
    'P': 0x0F|0x40, 'Q': 0x1F|0x40, 'R': 0x17|0x40, 'S': 0x0E|0x40, 'T': 0x1E|0x40,
    'U': 0x25|0x40, 'V': 0x27|0x40, 'W': 0x3A|0x40, 'X': 0x2D|0x40, 'Y': 0x3D|0x40,
    'Z': 0x35|0x40,
    '1': 0x01|0x80, '2': 0x03|0x80, '3': 0x09|0x80, '4': 0x19|0x80, '5': 0x11|0x80,
    '6': 0x0B|0x80, '7': 0x1B|0x80, '8': 0x13|0x80, '9': 0x0A|0x80, '0': 0x1A|0x80,
    ',': 0x02, ';': 0x06, ':': 0x12, '.': 0x32, '?': 0x26,
    '!': 0x16, '-': 0x24, "'": 0x04, '"': 0x36, '(': 0x13, ')': 0x2C,
}


def mask_to_dots(mask):
    """Convert a bitmask to a sorted list of active dot numbers (1-8)."""
    return [i + 1 for i in range(8) if (mask >> i) & 1]


# ── Connection state ──────────────────────────────────────────────────────────
_ser = None                       # pyserial handle (serial mode)
_tcp_sock = None                  # TCP socket handle (WiFi mode)
_conn_lock = threading.Lock()
_connected = False
_connected_port = None            # serial port path or "wifi:<ip>:<port>"
_conn_mode = None                 # "serial" or "wifi"
_send_stop = threading.Event()

_clients = []
_clients_lock = threading.Lock()

ESP_KEYWORDS = ('cp210', 'ch340', 'ch341', 'ftdi', 'esp32', 'uart', 'silicon')

ESP32_TCP_PORT = 3333


def _check_access():
    """Return a 403 error response if a passcode is configured and wrong; else None."""
    if not ACCESS_CODE:
        return None
    data = request.json or {}
    provided = data.get('access_code', '') or request.headers.get('X-Access-Code', '')
    if provided != ACCESS_CODE:
        return jsonify({'status': 'error', 'message': 'Invalid access code'}), 403
    return None


def broadcast(data):
    """Push an event to every connected SSE client."""
    with _clients_lock:
        dead = []
        for q in _clients:
            try:
                q.put_nowait(data)
            except queue.Full:
                dead.append(q)
        for q in dead:
            _clients.remove(q)


# ── Background readers ────────────────────────────────────────────────────────

def _serial_reader():
    """Forward ESP32 serial responses to SSE clients."""
    global _ser
    while True:
        try:
            with _conn_lock:
                if _conn_mode == 'serial' and _ser and _ser.is_open and _ser.in_waiting:
                    line = _ser.readline().decode(errors='replace').strip()
                    if line:
                        broadcast({'type': 'esp32', 'message': line})
        except Exception:
            pass
        time.sleep(0.02)


def _tcp_reader():
    """Forward ESP32 TCP responses to SSE clients."""
    global _tcp_sock, _connected, _conn_mode
    buf = ''
    while True:
        try:
            with _conn_lock:
                sock = _tcp_sock if _conn_mode == 'wifi' else None
            if sock is None:
                time.sleep(0.1)
                continue

            sock.settimeout(0.1)
            try:
                data = sock.recv(1024)
            except socket.timeout:
                continue
            except (OSError, ConnectionResetError):
                with _conn_lock:
                    if _conn_mode == 'wifi' and _connected:
                        _connected = False
                        _connected_port = None
                        _conn_mode = None
                        _tcp_sock = None
                broadcast({'type': 'disconnected'})
                buf = ''
                continue

            if not data:
                with _conn_lock:
                    if _conn_mode == 'wifi' and _connected:
                        _connected = False
                        _connected_port = None
                        _conn_mode = None
                        _tcp_sock = None
                broadcast({'type': 'disconnected'})
                buf = ''
                continue

            buf += data.decode('utf-8', errors='replace')
            while '\n' in buf:
                line, buf = buf.split('\n', 1)
                line = line.strip()
                if line:
                    broadcast({'type': 'esp32', 'message': line})
        except Exception:
            time.sleep(0.1)


threading.Thread(target=_serial_reader, daemon=True).start()
threading.Thread(target=_tcp_reader, daemon=True).start()


# ── Routes ────────────────────────────────────────────────────────────────────

@app.route('/')
def index():
    return render_template('index.html')


@app.route('/api/ports')
def list_ports():
    result = []
    for p in serial.tools.list_ports.comports():
        desc = (p.description or '').lower()
        mfr  = (p.manufacturer or '').lower()
        result.append({
            'device':      p.device,
            'description': p.description or 'Unknown',
            'likely_esp32': any(k in desc or k in mfr for k in ESP_KEYWORDS),
        })
    return jsonify(result)


@app.route('/api/config')
def config():
    return jsonify({'passcode_required': bool(ACCESS_CODE)})


@app.route('/api/status')
def status():
    return jsonify({
        'connected': _connected,
        'port': _connected_port,
        'mode': _conn_mode,
    })


@app.route('/api/connect', methods=['POST'])
def connect():
    global _ser, _tcp_sock, _connected, _connected_port, _conn_mode
    data = request.json or {}
    mode = data.get('mode', 'serial')

    # Disconnect any existing connection first
    _close_all()

    if mode == 'wifi':
        ip   = data.get('ip', '').strip()
        port = int(data.get('tcp_port', ESP32_TCP_PORT))
        if not ip:
            return jsonify({'status': 'error', 'message': 'IP address is required'}), 400

        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.settimeout(5)
            sock.connect((ip, port))
            sock.settimeout(None)
        except (socket.timeout, ConnectionRefusedError, OSError) as e:
            return jsonify({'status': 'error', 'message': f'TCP connect failed: {e}'}), 500

        with _conn_lock:
            _tcp_sock = sock
            _connected = True
            _connected_port = f'{ip}:{port}'
            _conn_mode = 'wifi'

        broadcast({'type': 'connected', 'port': f'WiFi {ip}:{port}'})

        # Give the ESP32 a moment, then drain the welcome message
        time.sleep(0.5)
        return jsonify({'status': 'connected', 'port': f'{ip}:{port}', 'mode': 'wifi'})

    else:
        # Serial mode
        port = data.get('port')
        baud = int(data.get('baud', 115200))

        with _conn_lock:
            try:
                _ser = serial.Serial(port, baud, timeout=2)
            except Exception as e:
                return jsonify({'status': 'error', 'message': str(e)}), 500

            _connected = True
            _connected_port = port
            _conn_mode = 'serial'

        broadcast({'type': 'connected', 'port': port})

        time.sleep(2)

        with _conn_lock:
            while _ser.in_waiting:
                line = _ser.readline().decode(errors='replace').strip()
                if line:
                    broadcast({'type': 'esp32', 'message': line})

        return jsonify({'status': 'connected', 'port': port, 'mode': 'serial'})


def _close_all():
    """Disconnect whatever is currently open."""
    global _ser, _tcp_sock, _connected, _connected_port, _conn_mode
    _send_stop.set()
    with _conn_lock:
        if _ser and _ser.is_open:
            try:
                _ser.close()
            except Exception:
                pass
            _ser = None
        if _tcp_sock:
            try:
                _tcp_sock.close()
            except Exception:
                pass
            _tcp_sock = None
        _connected = False
        _connected_port = None
        _conn_mode = None


@app.route('/api/disconnect', methods=['POST'])
def disconnect():
    _close_all()
    broadcast({'type': 'disconnected'})
    return jsonify({'status': 'disconnected'})


def _write(cmd):
    """Send a newline-terminated command to the ESP32 (serial or TCP)."""
    global _tcp_sock, _connected, _connected_port, _conn_mode
    broadcast({'type': 'tx', 'message': cmd})
    tcp_dropped = False
    with _conn_lock:
        if _conn_mode == 'serial' and _ser and _ser.is_open:
            try:
                _ser.write((cmd + '\n').encode())
                _ser.flush()
            except Exception as e:
                broadcast({'type': 'error', 'message': f'Serial write error: {e}'})
        elif _conn_mode == 'wifi' and _tcp_sock:
            try:
                _tcp_sock.sendall((cmd + '\n').encode())
            except (OSError, BrokenPipeError):
                try:
                    _tcp_sock.close()
                except Exception:
                    pass
                _tcp_sock = None
                _connected = False
                _connected_port = None
                _conn_mode = None
                _send_stop.set()
                tcp_dropped = True
    if tcp_dropped:
        broadcast({'type': 'disconnected'})


MAX_TEXT_LEN = 200

@app.route('/api/send', methods=['POST'])
def send_text():
    err = _check_access()
    if err:
        return err

    if not _connected:
        return jsonify({'status': 'error', 'message': 'Not connected to ESP32'}), 400

    data     = request.json or {}
    raw_text = data.get('text', '')
    delay_ms = max(100, min(5000, int(data.get('delay_ms', 1000))))

    # Strip control characters (keep printable ASCII + space)
    text = ''.join(c for c in raw_text if c == ' ' or (0x20 <= ord(c) <= 0x7E))
    if not text:
        return jsonify({'status': 'error', 'message': 'Empty text'}), 400
    if len(text) > MAX_TEXT_LEN:
        return jsonify({
            'status': 'error',
            'message': f'Text too long ({len(text)} chars). Maximum is {MAX_TEXT_LEN}.',
        }), 400

    _send_stop.clear()
    broadcast({'type': 'sending_start', 'text': text, 'total': len(text)})

    def worker():
        for idx, ch in enumerate(text):
            if _send_stop.is_set():
                broadcast({'type': 'stopped', 'index': idx})
                break

            mask = BRAILLE_MAP.get(ch, 0xFF)
            dots = mask_to_dots(mask)
            cmd  = ('DOTS:' + ','.join(map(str, dots))) if dots else 'DOTS:NONE'
            _write(cmd)

            broadcast({
                'type':  'char',
                'index': idx,
                'char':  ch,
                'dots':  dots,
                'total': len(text),
            })
            time.sleep(delay_ms / 1000.0)

        if not _send_stop.is_set():
            _write('DOTS:NONE')
            broadcast({'type': 'done'})

    threading.Thread(target=worker, daemon=True).start()
    return jsonify({'status': 'sending', 'chars': len(text)})


@app.route('/api/stop', methods=['POST'])
def stop():
    _send_stop.set()
    _write('DOTS:NONE')
    broadcast({'type': 'stopped', 'index': -1})
    return jsonify({'status': 'stopping'})


@app.route('/api/clear', methods=['POST'])
def clear():
    _write('DOTS:NONE')
    broadcast({'type': 'cleared'})
    return jsonify({'status': 'cleared'})


@app.route('/api/test', methods=['POST'])
def test():
    _write('TEST')
    broadcast({'type': 'test_started'})
    return jsonify({'status': 'test_started'})


@app.route('/api/stream')
def stream():
    """Server-Sent Events endpoint — one queue per client."""
    q = queue.Queue(maxsize=200)
    with _clients_lock:
        _clients.append(q)

    def generate():
        try:
            while True:
                try:
                    event = q.get(timeout=15)
                    yield 'data: {}\n\n'.format(json.dumps(event))
                except queue.Empty:
                    yield 'data: {"type":"ping"}\n\n'
        finally:
            with _clients_lock:
                if q in _clients:
                    _clients.remove(q)

    return Response(
        generate(),
        mimetype='text/event-stream',
        headers={'Cache-Control': 'no-cache', 'X-Accel-Buffering': 'no'},
    )


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Braille ESP32 web interface')
    parser.add_argument('--port', type=int, default=None, help='Web server port (default: 5000)')
    args = parser.parse_args()

    requested_port = args.port if args.port is not None else int(os.environ.get('PORT', '5000'))
    port = requested_port

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        while sock.connect_ex(('127.0.0.1', port)) == 0:
            port += 1

    print('=' * 50)
    print('  Braille ESP32 Web Interface')
    print('  Supports Serial (USB) and WiFi (TCP) modes')
    if port != requested_port:
        print(f'  Port {requested_port} busy, using {port}')
    print(f'  Open http://localhost:{port}')
    print('=' * 50)
    app.run(debug=False, threaded=True, host='0.0.0.0', port=port)
