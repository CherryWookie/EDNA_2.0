"""
MBL_QTC.py  —  MBL Quadcopter Telemetry Console
================================================
Real-time telemetry display and PID tuner for the ESP32 drone.

Requirements:
    pip install tkinter matplotlib

Usage:
    1. Connect your laptop to the "esp32drone" WiFi network.
    2. Run:  python MBL_QTC.py
    3. Click "Connect" to start receiving data.
    4. Adjust PID sliders and click "Send Gains" to hot-update the drone.

Layout:
    Left panel  — live roll/pitch angle plot (last 5 seconds)
    Right panel — motor PWM bar graph
    Bottom      — PID gain sliders for pitch, roll, and yaw
    Status bar  — packet rate, connection state
"""

import tkinter as tk
from tkinter import ttk, messagebox
import threading
import time
from collections import deque

try:
    import matplotlib
    matplotlib.use("TkAgg")
    import matplotlib.pyplot as plt
    from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
    from matplotlib.animation import FuncAnimation
    HAS_MPL = True
except ImportError:
    HAS_MPL = False
    print("[QTC] matplotlib not found — plots disabled. pip install matplotlib")

from udp_client import UDPClient

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------
HISTORY_SECS  = 5       # Seconds of history shown in the angle plot
PLOT_RATE_MS  = 100     # Plot refresh interval (ms)
MAX_POINTS    = int(HISTORY_SECS * 1000 / PLOT_RATE_MS * 2)  # ~100 pts

AXES = ["pitch", "roll", "yaw"]

DEFAULT_GAINS = {
    "pitch": {"kp": 1.2, "ki": 0.01, "kd": 0.08},
    "roll":  {"kp": 1.2, "ki": 0.01, "kd": 0.08},
    "yaw":   {"kp": 2.0, "ki": 0.05, "kd": 0.00},
}


# ---------------------------------------------------------------------------
# Main application window
# ---------------------------------------------------------------------------
class QTCApp:
    def __init__(self, root: tk.Tk):
        self.root = root
        self.root.title("MBL Quadcopter Telemetry Console")
        self.root.geometry("1100x700")
        self.root.resizable(True, True)

        self.client = UDPClient()
        self.connected = False
        self.pkt_count = 0
        self.pkt_rate  = 0
        self._rate_ts  = time.time()

        # Data history for plots
        self.t_history     = deque(maxlen=MAX_POINTS)
        self.pitch_history = deque(maxlen=MAX_POINTS)
        self.roll_history  = deque(maxlen=MAX_POINTS)
        self.t0 = time.time()

        self._build_ui()
        self.client.on_data(self._on_packet)

        # Start plot animation
        if HAS_MPL:
            self._anim = FuncAnimation(
                self.fig, self._update_plot,
                interval=PLOT_RATE_MS, blit=False, cache_frame_data=False
            )

        self.root.protocol("WM_DELETE_WINDOW", self._on_close)

    # -----------------------------------------------------------------------
    # UI construction
    # -----------------------------------------------------------------------
    def _build_ui(self):
        # Top toolbar
        toolbar = tk.Frame(self.root, pady=4)
        toolbar.pack(side=tk.TOP, fill=tk.X, padx=8)

        self.btn_connect = tk.Button(toolbar, text="Connect",
                                     command=self._toggle_connect,
                                     width=10, bg="#4CAF50", fg="white")
        self.btn_connect.pack(side=tk.LEFT, padx=4)

        self.lbl_status = tk.Label(toolbar, text="Disconnected",
                                   fg="gray", font=("Courier", 10))
        self.lbl_status.pack(side=tk.LEFT, padx=8)

        self.lbl_rate = tk.Label(toolbar, text="0 pkt/s",
                                 font=("Courier", 10), fg="gray")
        self.lbl_rate.pack(side=tk.RIGHT, padx=8)

        # Main content area
        content = tk.Frame(self.root)
        content.pack(fill=tk.BOTH, expand=True, padx=8, pady=4)

        # Left: angle plot
        left = tk.Frame(content)
        left.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)

        if HAS_MPL:
            self.fig, self.ax = plt.subplots(figsize=(6, 3.5))
            self.fig.patch.set_facecolor("#f8f8f8")
            self.ax.set_facecolor("#f8f8f8")
            self.ax.set_title("Pitch & Roll  (degrees)", fontsize=11)
            self.ax.set_ylim(-45, 45)
            self.ax.set_xlabel("time (s)")
            self.ax.set_ylabel("degrees")
            self.ax.axhline(0, color="#aaa", linewidth=0.5)
            self.ax.grid(True, linestyle=":", linewidth=0.5)
            self.line_pitch, = self.ax.plot([], [], label="pitch", color="#E24B4A", linewidth=1.5)
            self.line_roll,  = self.ax.plot([], [], label="roll",  color="#378ADD", linewidth=1.5)
            self.ax.legend(loc="upper right", fontsize=9)
            self.fig.tight_layout()
            canvas = FigureCanvasTkAgg(self.fig, master=left)
            canvas.get_tk_widget().pack(fill=tk.BOTH, expand=True)
            self.mpl_canvas = canvas
        else:
            tk.Label(left, text="Install matplotlib for live plots\n(pip install matplotlib)",
                     fg="gray").pack(expand=True)

        # Right: live values + motor bars
        right = tk.Frame(content, width=260)
        right.pack(side=tk.RIGHT, fill=tk.Y, padx=(8, 0))
        right.pack_propagate(False)

        tk.Label(right, text="Live values", font=("Helvetica", 11, "bold")).pack(anchor=tk.W)
        self.live_vars = {}
        for field in ["pitch", "roll", "yaw_rate", "throttle",
                       "m_fl", "m_fr", "m_rl", "m_rr"]:
            row = tk.Frame(right)
            row.pack(fill=tk.X, pady=1)
            tk.Label(row, text=f"{field}:", width=12, anchor=tk.W,
                     font=("Courier", 10)).pack(side=tk.LEFT)
            v = tk.StringVar(value="—")
            tk.Label(row, textvariable=v, font=("Courier", 10),
                     anchor=tk.E, width=8).pack(side=tk.RIGHT)
            self.live_vars[field] = v

        # Separator
        ttk.Separator(right, orient="horizontal").pack(fill=tk.X, pady=6)

        # Motor PWM bars
        tk.Label(right, text="Motor PWM (µs)", font=("Helvetica", 10, "bold")).pack(anchor=tk.W)
        self.motor_bars = {}
        self.motor_bar_vars = {}
        for m in ["m_fl", "m_fr", "m_rl", "m_rr"]:
            row = tk.Frame(right)
            row.pack(fill=tk.X, pady=2)
            tk.Label(row, text=m, width=5, font=("Courier", 9)).pack(side=tk.LEFT)
            bar = ttk.Progressbar(row, length=160, maximum=1000, value=0)
            bar.pack(side=tk.LEFT, padx=4)
            self.motor_bars[m] = bar

        # Bottom: PID sliders
        pid_frame = ttk.LabelFrame(self.root, text="PID Gains", padding=8)
        pid_frame.pack(fill=tk.X, padx=8, pady=4)

        self.gain_vars = {}
        for col, axis in enumerate(AXES):
            grp = tk.Frame(pid_frame)
            grp.grid(row=0, column=col, padx=16, sticky=tk.N)
            tk.Label(grp, text=axis.upper(),
                     font=("Helvetica", 10, "bold")).grid(row=0, column=0,
                                                           columnspan=3, pady=(0, 4))
            self.gain_vars[axis] = {}
            for r, g in enumerate(["kp", "ki", "kd"]):
                tk.Label(grp, text=g, width=3, font=("Courier", 10)).grid(
                    row=r+1, column=0, sticky=tk.E)
                v = tk.DoubleVar(value=DEFAULT_GAINS[axis][g])
                self.gain_vars[axis][g] = v
                step = 0.001 if g in ("ki", "kd") else 0.01
                scale = tk.Scale(grp, variable=v, from_=0.0, to=5.0,
                                  resolution=step, orient=tk.HORIZONTAL,
                                  length=160, showvalue=True,
                                  font=("Courier", 9))
                scale.grid(row=r+1, column=1, padx=4)

        # Send button
        btn_row = tk.Frame(pid_frame)
        btn_row.grid(row=0, column=len(AXES), padx=16, sticky=tk.N+tk.S)
        tk.Button(btn_row, text="Send\nGains", command=self._send_gains,
                  width=8, height=3, bg="#378ADD", fg="white",
                  font=("Helvetica", 10, "bold")).pack(expand=True)

        # Status bar
        self.lbl_statusbar = tk.Label(self.root, text="Not connected.",
                                       bd=1, relief=tk.SUNKEN, anchor=tk.W,
                                       font=("Courier", 9), fg="gray")
        self.lbl_statusbar.pack(side=tk.BOTTOM, fill=tk.X)

        # Periodic UI refresh
        self.root.after(200, self._update_rate_label)

    # -----------------------------------------------------------------------
    # Connection
    # -----------------------------------------------------------------------
    def _toggle_connect(self):
        if not self.connected:
            try:
                self.client.start()
                self.connected = True
                self.btn_connect.config(text="Disconnect", bg="#E24B4A")
                self.lbl_status.config(text="Connected to esp32drone", fg="green")
                self.lbl_statusbar.config(text="UDP listening on port 4444. Waiting for first packet...")
            except Exception as e:
                messagebox.showerror("Connection error", str(e))
        else:
            self.client.stop()
            self.connected = False
            self.btn_connect.config(text="Connect", bg="#4CAF50")
            self.lbl_status.config(text="Disconnected", fg="gray")

    # -----------------------------------------------------------------------
    # Packet handler (runs in UDP receive thread)
    # -----------------------------------------------------------------------
    def _on_packet(self, d: dict):
        self.pkt_count += 1
        t = time.time() - self.t0

        # Update history for plot (thread-safe: deque appends are atomic)
        self.t_history.append(t)
        self.pitch_history.append(d.get("pitch", 0))
        self.roll_history.append(d.get("roll", 0))

        # Update live value labels on main thread
        self.root.after(0, self._refresh_live, d)

    def _refresh_live(self, d: dict):
        for field, v in self.live_vars.items():
            val = d.get(field)
            if val is None:
                v.set("—")
            elif isinstance(val, float):
                v.set(f"{val:+.2f}")
            else:
                v.set(str(val))
        # Motor bars: value is µs 1000–2000 → bar 0–1000
        for m in ["m_fl", "m_fr", "m_rl", "m_rr"]:
            val = d.get(m, 1000)
            self.motor_bars[m]["value"] = max(0, int(val) - 1000)

        # Update gain sliders if drone reports its current gains
        for axis in AXES:
            for g in ["kp", "ki", "kd"]:
                key = f"{g}_{axis}"
                if key in d:
                    self.gain_vars[axis][g].set(round(d[key], 4))

        self.lbl_statusbar.config(
            text=f"pitch:{d.get('pitch',0):+.1f}°  roll:{d.get('roll',0):+.1f}°  "
                 f"thr:{d.get('throttle',0)} µs  "
                 f"m_fl:{d.get('m_fl',0)} m_fr:{d.get('m_fr',0)} "
                 f"m_rl:{d.get('m_rl',0)} m_rr:{d.get('m_rr',0)}",
            fg="black"
        )

    # -----------------------------------------------------------------------
    # Matplotlib animation callback
    # -----------------------------------------------------------------------
    def _update_plot(self, _frame):
        if not HAS_MPL:
            return
        t  = list(self.t_history)
        if not t:
            return
        p  = list(self.pitch_history)
        r  = list(self.roll_history)
        self.line_pitch.set_data(t, p)
        self.line_roll.set_data(t, r)
        window = HISTORY_SECS
        self.ax.set_xlim(max(0, t[-1] - window), t[-1] + 0.5)
        self.mpl_canvas.draw_idle()

    # -----------------------------------------------------------------------
    # PID send
    # -----------------------------------------------------------------------
    def _send_gains(self):
        if not self.connected:
            messagebox.showwarning("Not connected", "Connect to the drone first.")
            return
        for axis in AXES:
            kp = self.gain_vars[axis]["kp"].get()
            ki = self.gain_vars[axis]["ki"].get()
            kd = self.gain_vars[axis]["kd"].get()
            self.client.send_pid(axis, kp, ki, kd)
        self.lbl_statusbar.config(text="PID gains sent.", fg="blue")

    # -----------------------------------------------------------------------
    # Misc
    # -----------------------------------------------------------------------
    def _update_rate_label(self):
        now = time.time()
        elapsed = now - self._rate_ts
        if elapsed >= 1.0:
            self.pkt_rate  = self.pkt_count / elapsed
            self.pkt_count = 0
            self._rate_ts  = now
        self.lbl_rate.config(text=f"{self.pkt_rate:.1f} pkt/s")
        self.root.after(200, self._update_rate_label)

    def _on_close(self):
        if self.connected:
            self.client.stop()
        self.root.destroy()


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------
if __name__ == "__main__":
    root = tk.Tk()
    app = QTCApp(root)
    root.mainloop()
