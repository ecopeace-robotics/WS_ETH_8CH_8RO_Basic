#!/usr/bin/env python3
"""
Keyboard control for ESP32 rover (eth_motor project).
Independent RC mode: CH1 = left motor, CH2 = right motor.

Works in any terminal (VSCode integrated terminal, SSH, etc.) — no pynput needed.
Uses raw stdin + key-repeat timing to detect hold/release.

Usage:
  python tools/keyboard_ctrl.py [ESP32_IP]

Keys: W/↑=forward  S/↓=back  A/←=left  D/→=right  Space=stop  Q/Esc=quit

Tuning:
  RAMP_STEP    : µs added/removed per tick (larger = snappier)
  TICK         : control loop period in seconds (default 50 ms = 20 Hz)
  HOLD_TIMEOUT : seconds without a key repeat before treating it as released
                 (must be longer than terminal key-repeat interval, ~30 ms)
"""

import os
import select
import socket
import sys
import termios
import threading
import time
import tty

# ── Configuration ──────────────────────────────────────────────────────────────
HOST         = "192.168.0.197"  # ESP32 IP — override via command-line arg
PORT         = 8080
RAMP_STEP    = 20               # µs per tick
TICK         = 0.05             # 50 ms = 20 Hz control loop
HOLD_TIMEOUT = 0.15             # seconds — key considered released after this gap
NEUTRAL      = 1500
MIN_US       = 1000
MAX_US       = 2000

# ── Raw terminal key reader ────────────────────────────────────────────────────

def read_key():
    """
    Non-blocking raw key read.
    Returns a string token or None:
      'up' 'down' 'left' 'right'   ← arrow keys
      'w' 's' 'a' 'd' ' ' 'q'     ← letter / space
      'esc'                         ← Escape (bare, not part of arrow sequence)
    """
    if not select.select([sys.stdin], [], [], 0)[0]:
        return None

    ch = os.read(sys.stdin.fileno(), 1)

    if ch == b'\x1b':
        # Peek for arrow key escape sequence: ESC [ A/B/C/D
        if select.select([sys.stdin], [], [], 0.02)[0]:
            ch2 = os.read(sys.stdin.fileno(), 1)
            if ch2 == b'[' and select.select([sys.stdin], [], [], 0.02)[0]:
                ch3 = os.read(sys.stdin.fileno(), 1)
                return {'A': 'up', 'B': 'down', 'C': 'right', 'D': 'left'}.get(ch3.decode(), None)
        return 'esc'

    try:
        c = ch.decode()
    except UnicodeDecodeError:
        return None

    if c == ' ':  return 'space'
    if c == '\r': return None       # ignore Enter
    return c.lower()


# ── Active-key tracking (hold via key-repeat + timeout) ───────────────────────

class KeyState:
    """
    Tracks which keys are 'held' based on how recently they were seen.
    Terminal key-repeat fires every ~30 ms when a key is held; we consider
    a key released if we haven't seen it for HOLD_TIMEOUT seconds.
    """
    def __init__(self):
        self._last_seen = {}   # key → timestamp

    def update(self, key):
        if key is not None:
            self._last_seen[key] = time.monotonic()

    def active(self):
        now = time.monotonic()
        return {k for k, t in self._last_seen.items() if now - t < HOLD_TIMEOUT}


# ── Control math ───────────────────────────────────────────────────────────────

FORWARD_KEYS  = {'up',  'w'}
BACKWARD_KEYS = {'down', 's'}
LEFT_KEYS     = {'left', 'a'}
RIGHT_KEYS    = {'right', 'd'}
STOP_KEYS     = {'space'}
QUIT_KEYS     = {'q', 'esc'}


def clamp(v, lo, hi):
    return max(lo, min(hi, v))


def compute_targets(active):
    throttle = +1 if active & FORWARD_KEYS  else (-1 if active & BACKWARD_KEYS else 0)
    turn     = +1 if active & RIGHT_KEYS    else (-1 if active & LEFT_KEYS     else 0)
    l = clamp(NEUTRAL + throttle * 500 - turn * 500, MIN_US, MAX_US)
    r = clamp(NEUTRAL + throttle * 500 + turn * 500, MIN_US, MAX_US)
    return l, r


# ── TCP helpers ────────────────────────────────────────────────────────────────

def send_pwm(sock, ch_num, us):
    sock.sendall(f"PWM {ch_num} {us}\n".encode())


def drain_responses(sock):
    """Background thread — discard ESP32 OK/ERROR replies."""
    while True:
        try:
            if not sock.recv(256):
                break
        except OSError:
            break


# ── Main control loop ──────────────────────────────────────────────────────────

def control_loop(sock):
    ch = [NEUTRAL, NEUTRAL]
    ks = KeyState()

    threading.Thread(target=drain_responses, args=(sock,), daemon=True).start()

    print("Ready. Hold keys to drive, release to coast to stop.")
    print("  W / ↑   forward        S / ↓   backward")
    print("  A / ←   pivot left     D / →   pivot right")
    print("  W+A / W+D   curve      Space   emergency stop")
    print("  Q / Esc     quit")
    print()

    while True:
        key = read_key()
        ks.update(key)
        active = ks.active()

        if active & QUIT_KEYS:
            break

        if active & STOP_KEYS:
            ch[0] = NEUTRAL
            ch[1] = NEUTRAL
        else:
            l_t, r_t = compute_targets(active)
            for i, target in enumerate([l_t, r_t]):
                if ch[i] < target:
                    ch[i] = min(ch[i] + RAMP_STEP, target)
                elif ch[i] > target:
                    ch[i] = max(ch[i] - RAMP_STEP, target)

        send_pwm(sock, 1, ch[0])
        send_pwm(sock, 2, ch[1])

        bar_l = int((ch[0] - MIN_US) / (MAX_US - MIN_US) * 20)
        bar_r = int((ch[1] - MIN_US) / (MAX_US - MIN_US) * 20)
        print(
            f"\r  L {ch[0]:4d}µs [{'#' * bar_l:<20}]"
            f"   R {ch[1]:4d}µs [{'#' * bar_r:<20}]  ",
            end="", flush=True
        )

        time.sleep(TICK)

    send_pwm(sock, 1, NEUTRAL)
    send_pwm(sock, 2, NEUTRAL)
    print("\nMotors stopped. Goodbye.")


# ── Entry point ────────────────────────────────────────────────────────────────

def main():
    host = sys.argv[1] if len(sys.argv) > 1 else HOST

    print(f"Connecting to {host}:{PORT} ...")
    try:
        sock = socket.create_connection((host, PORT), timeout=5)
    except OSError as e:
        print(f"Connection failed: {e}")
        sys.exit(1)

    sock.recv(256)  # drain ESP32 welcome banner

    # Switch terminal to raw mode (no line buffer, no echo)
    fd = sys.stdin.fileno()
    old_settings = termios.tcgetattr(fd)
    try:
        tty.setraw(fd)
        control_loop(sock)
    finally:
        termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)
        sock.close()
        print()  # newline after raw mode restored


if __name__ == "__main__":
    main()
