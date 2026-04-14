"""
udp_client.py
UDP socket wrapper used by MBL_QTC.py.

The ESP32 acts as a UDP server bound to TELEMETRY_PORT.
We send one dummy packet first so the ESP32 learns our IP address and
can start sending telemetry back to us.
"""

import socket
import json
import threading

ESP32_IP   = "192.168.4.1"   # Default ESP32 AP gateway IP
ESP32_PORT = 4444
BIND_PORT  = 4444            # Listen on same port
TIMEOUT    = 0.05            # Non-blocking receive timeout (seconds)


class UDPClient:
    def __init__(self):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.sock.settimeout(TIMEOUT)
        self.sock.bind(("", BIND_PORT))
        self.esp32_addr = (ESP32_IP, ESP32_PORT)
        self._lock = threading.Lock()
        self._running = False
        self._latest = {}
        self._callbacks = []

    def start(self):
        """Start background receive thread. Calls registered callbacks with each packet."""
        # Send hello so ESP32 learns our address
        self._send_raw(b'{"hello":1}\n')
        self._running = True
        t = threading.Thread(target=self._recv_loop, daemon=True)
        t.start()

    def stop(self):
        self._running = False
        self.sock.close()

    def on_data(self, callback):
        """Register a callback(dict) called on every received telemetry packet."""
        self._callbacks.append(callback)

    def send_pid(self, axis: str, kp: float, ki: float, kd: float):
        """Send a PID gain update to the ESP32."""
        pkt = json.dumps({"axis": axis, "kp": kp, "ki": ki, "kd": kd}) + "\n"
        self._send_raw(pkt.encode())

    def latest(self) -> dict:
        """Return the most recently received telemetry dict (thread-safe copy)."""
        with self._lock:
            return dict(self._latest)

    def _send_raw(self, data: bytes):
        try:
            self.sock.sendto(data, self.esp32_addr)
        except OSError as e:
            print(f"[UDP] Send error: {e}")

    def _recv_loop(self):
        while self._running:
            try:
                data, _ = self.sock.recvfrom(1024)
                text = data.decode("utf-8", errors="ignore").strip()
                parsed = json.loads(text)
                with self._lock:
                    self._latest = parsed
                for cb in self._callbacks:
                    try:
                        cb(parsed)
                    except Exception as e:
                        print(f"[UDP] Callback error: {e}")
            except socket.timeout:
                continue
            except json.JSONDecodeError:
                continue
            except Exception as e:
                if self._running:
                    print(f"[UDP] Recv error: {e}")
