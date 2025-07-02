"""Serial Plot Application

Receives messages from an ESP32 over the serial port and:
• Prints LOG messages to the console.
• Prints ERROR messages in red (requires `colorama`).
• Parses DATA-Y-Z-K messages and plots Y, Z, K in real time.

Dependencies:
    pip install pyserial matplotlib colorama  # colorama optional

Run with:
    python plot_and_operate.py --port COM3 --baud 115200

"""

import argparse
import serial
import threading
import queue
from collections import deque

import matplotlib.pyplot as plt
import matplotlib.animation as animation
from matplotlib import style

try:
    from colorama import init, Fore, Style as CStyle
    init(autoreset=True)
    COLORAMA = True
except ImportError:
    COLORAMA = False

# ----------------------- Serial Reader Thread ----------------------- #
class SerialReader(threading.Thread):
    """Background thread that continuously reads lines from a serial port
    and places them on a thread-safe queue."""

    def __init__(self, port: str, baud: int, q: queue.Queue):
        super().__init__(daemon=True)
        self.queue = q
        self.running = True
        try:
            self.ser = serial.Serial(port, baud, timeout=1)
        except serial.SerialException as exc:
            print(f"[FATAL] Could not open serial port {port}: {exc}")
            raise

    def run(self):
        while self.running:
            try:
                line = self.ser.readline().decode(errors="ignore")
                if line:
                    self.queue.put(line)
            except serial.SerialException as exc:
                self.queue.put(f"ERROR-Serial exception: {exc}")
                self.stop()

    def stop(self):
        self.running = False
        if self.ser and self.ser.is_open:
            self.ser.close()

# ---------------------------- Parsing -------------------------------- #

def parse_line(line: str):
    """Return a tuple (kind, payload). kind ∈ {log, error, data}."""
    line = line.strip()
    if line.startswith("LOG-"):
        return ("log", line[4:])
    if line.startswith("ERROR-"):
        return ("error", line)
    if line.startswith("DATA-"):
        try:
            _, y_str, z_str, k_str = line.split("-", 3)
            y = int(y_str)
            z = int(z_str)
            # Treat anything other than an explicit 0/'false' as True
            k = 1 if k_str.lower() not in ("0", "false", "f") else 0
            return ("data", (y, z, k))
        except ValueError:
            return ("error", f"ERROR-Invalid DATA message: {line}")
    return ("error", f"ERROR-Unrecognized message: {line}")

# ---------------------------- Plotting -------------------------------- #

def live_plot(port: str, baud: int, max_pts: int = 200):
    """Spawn SerialReader and update a live Matplotlib graph."""
    msg_q: queue.Queue[str] = queue.Queue()
    reader = SerialReader(port, baud, msg_q)
    reader.start()

    xs, ys1, ys2, ys3 = (deque(maxlen=max_pts) for _ in range(4))
    sample_idx = 0

    style.use("ggplot")
    fig, ax = plt.subplots()
    fig.canvas.manager.set_window_title("ESP32 Live Data Viewer")

    line_y, = ax.plot([], [], "r-", label="Y (red)")
    line_z, = ax.plot([], [], "g-", label="Z (green)")
    line_k, = ax.plot([], [], "y-", label="K (yellow)")

    ax.set_xlabel("Sample #")
    ax.set_ylabel("Value")
    ax.set_xlim(0, max_pts)
    ax.set_ylim(-10, 260)  # Adjust to your expected range
    ax.legend(loc="upper left")

    def update(_):
        nonlocal sample_idx
        # Drain the queue completely each frame
        while not msg_q.empty():
            raw = msg_q.get()
            kind, payload = parse_line(raw)
            if kind == "log":
                print(f"[LOG] {payload}")
            elif kind == "error":
                if COLORAMA:
                    print(Fore.RED + payload + CStyle.RESET_ALL)
                else:
                    print(payload)
            elif kind == "data":
                y, z, k = payload
                xs.append(sample_idx)
                ys1.append(y)
                ys2.append(z)
                ys3.append(k)
                sample_idx += 1

        # Update line data
        line_y.set_data(xs, ys1)
        line_z.set_data(xs, ys2)
        line_k.set_data(xs, ys3)
        if xs:
            ax.set_xlim(xs[0], xs[-1] + 1)
        return line_y, line_z, line_k

    ani = animation.FuncAnimation(fig, update, interval=100, blit=True)

    try:
        plt.show()
    finally:
        reader.stop()

# ----------------------------- CLI entry ------------------------------ #

def main():
    parser = argparse.ArgumentParser(description="ESP32 serial plot application")
    parser.add_argument("--port", default="COM3", help="Serial port (e.g. COM3 or /dev/ttyUSB0)")
    parser.add_argument("--baud", type=int, default=115200, help="Baud rate")
    args = parser.parse_args()
    live_plot(args.port, args.baud)

if __name__ == "__main__":
    main()
