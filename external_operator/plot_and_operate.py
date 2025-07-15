#!/usr/bin/env python3
"""Serial Plot Application  (v7 – modo *debug* + painel de envio)

Recursos principais
===================
* **Plot** em tempo real (eixo Y 0‑100) – inalterado.
* Thread **RX** continua exibindo dados.
* **Envio de comandos**:
  * Você pode **digitar direto no terminal** (como antes) **OU**<br>usar um **campo de texto** na janela do gráfico (ativado por padrão).
* Flag **--debug** (ou -d) controla verbosidade:
  * **Debug ON** (default) → mostra tudo: [RAW], [PARSE], [DATA], [WARN], [TX], etc.
  * **Debug OFF** (`--no-debug`) → mostra **apenas mensagens LOG**.

Uso rápido
----------
```bash
python plot_and_operate.py --port COM3            # debug ON, GUI‑TX ON
python plot_and_operate.py --port COM3 --no-debug # só logs
```
"""

from __future__ import annotations

import argparse
import queue
import re
import threading
from collections import deque
from pathlib import Path
import sys

import serial

# Matplotlib – definir backend **antes** de importar pyplot
import matplotlib

matplotlib.use("TkAgg")

from matplotlib import animation, pyplot as plt, style
import tkinter as tk

# ----------------------- Config / util ------------------------------ #

def parse_args():
    p = argparse.ArgumentParser(description="ESP32 serial plot + TX + debug flag")
    p.add_argument("--port", default="COM3", help="Serial port (e.g. COM3 or /dev/ttyUSB0)")
    p.add_argument("--baud", type=int, default=9600, help="Baud rate (default 9600)")
    dbg = p.add_mutually_exclusive_group()
    dbg.add_argument("--debug", dest="debug", action="store_true", help="Verbose output (default)")
    dbg.add_argument("--no-debug", dest="debug", action="store_false", help="Only LOG lines")
    p.set_defaults(debug=True)
    return p.parse_args()


def dprint(debug: bool, *args, **kwargs):
    """Imprime apenas se debug estiver True."""
    if debug:
        print(*args, **kwargs)


# ----------------------- Serial Reader Thread ----------------------- #
class SerialReader(threading.Thread):
    """Thread que lê linhas da serial em background e empilha na fila."""

    def __init__(self, port: str, baud: int, q: queue.Queue[str], debug: bool):
        super().__init__(daemon=True)
        self.queue = q
        self.debug = debug
        self._running = True
        try:
            self.ser = serial.Serial(port, baud, timeout=1)
            print(f"[INFO] Serial aberta em {port} @ {baud} bps")
        except serial.SerialException as exc:
            print(f"[FATAL] Não foi possível abrir a porta serial {port}: {exc}")
            raise

    def _readline(self) -> str:
        raw = self.ser.readline().decode(errors="ignore")
        dprint(self.debug, f"[RAW] {raw.rstrip()}")
        return raw

    def run(self):
        while self._running:
            try:
                line = self._readline()
                if line:
                    self.queue.put(line)
            except serial.SerialException as exc:
                self.queue.put(f"E (Serial): {exc}\n")
                self.stop()

    def stop(self):
        self._running = False
        if getattr(self, "ser", None) and self.ser.is_open:
            self.ser.close()
            print("[INFO] Porta serial fechada")


# ---------------------------- Parsing -------------------------------- #
_RE_DATA = re.compile(r"^DATA-(.+)$")
_RE_BOOT = re.compile(r"^(load:|entry |rst:|clk_ets |configsip:|mode:|q_drv:|d_drv:|Boot|ESP-ROM)", re.I)


def parse_line(line: str):
    """Classifica e interpreta UM fragmento de linha.

    Retorna (kind, payload) onde kind ∈ {log, error, data}.
    """

    line = line.strip()
    if not line:
        return "log", ""  # ignora vazios

    # DATA
    m = _RE_DATA.match(line)
    if m:
        nums = [p for p in m.group(1).split("-") if p]
        try:
            ints = list(map(int, nums))
        except ValueError:
            return "error", f"ERRO-Invalid DATA message: {line}"
        if len(ints) < 3:
            return "error", f"ERRO-Incomplete DATA message: {line}"
        y, z, k = ints[-3:]
        k = 0 if str(k).lower() in ("0", "false", "f") else 1
        return "data", (y, z, k)

    # Boot msgs -> log
    if _RE_BOOT.match(line):
        return "log", line

    # LOG explícito
    if line.lower().startswith("log-"):
        return "log", line.split("-", 1)[1]

    # Error do IDF
    if line.startswith("E ("):
        return "error", f"ERRO-{line}"

    if line.startswith("ERROR-"):
        return "error", line

    return "log", line


# ---------------------------- Plotting -------------------------------- #

def start_gui_tx(root, send_cb):
    """Adiciona um frame inferior com Entry e Button para TX."""
    frm = tk.Frame(root)
    frm.pack(side=tk.BOTTOM, fill=tk.X)
    ent = tk.Entry(frm)
    ent.pack(side=tk.LEFT, expand=True, fill=tk.X, padx=4, pady=2)

    def on_send(event=None):
        cmd = ent.get().strip()
        if cmd:
            send_cb(cmd)
            ent.delete(0, tk.END)
    btn = tk.Button(frm, text="Send", command=on_send)
    btn.pack(side=tk.RIGHT, padx=4)
    ent.bind("<Return>", on_send)
    return ent


def live_plot(port: str, baud: int, debug: bool, max_pts: int = 200):
    q_rx: queue.Queue[str] = queue.Queue()
    reader = SerialReader(port, baud, q_rx, debug)
    reader.start()

    stop_evt = threading.Event()

    # Função de envio usada pelo terminal e pelo GUI
    def serial_send(cmd: str):
        try:
            reader.ser.write((cmd + "\n").encode())
            dprint(debug, f"[TX] {cmd}")
        except serial.SerialException as exc:
            print(f"ERRO-Serial TX: {exc}")

    # Thread para ler stdin (terminal)
    def stdin_tx():
        while not stop_evt.is_set():
            try:
                cmd = input()
            except EOFError:
                break
            if cmd:
                serial_send(cmd)
    threading.Thread(target=stdin_tx, daemon=True).start()

    xs, y_vals, z_vals, k_vals = (deque(maxlen=max_pts) for _ in range(4))
    sample_idx = 0

    style.use("ggplot")
    fig, ax = plt.subplots()
    fig.canvas.manager.set_window_title("ESP32 Live Data Viewer")

    # Linhas visíveis
    line_y, = ax.plot([], [], "ro-", linewidth=2, label="Y (red)")
    line_z, = ax.plot([], [], "go-", linewidth=2, label="Z (green)")
    line_k, = ax.plot([], [], "yo-", linewidth=2, label="K (yellow)")

    ax.set_xlabel("Sample #")
    ax.set_ylabel("Value")
    ax.set_xlim(0, max_pts)
    ax.set_ylim(0, 100)
    ax.legend(loc="upper left")

    # Adiciona GUI para TX usando Tk (aproveita a mesma janela do Matplotlib)
    root = fig.canvas.manager.window  # Tk root
    start_gui_tx(root, serial_send)

    def update(_):
        nonlocal sample_idx
        while not q_rx.empty():
            raw = q_rx.get()
            for part in raw.replace("\\n", "\n").splitlines():
                kind, payload = parse_line(part)
                dprint(debug, f"[PARSE] kind={kind} payload={payload}")
                if kind == "log" and payload:
                    print(f"[LOG] {payload}")
                elif debug and kind == "error":
                    print(payload)
                elif kind == "data":
                    y, z, k = payload
                    if debug and (not 0 <= y <= 100 or not 0 <= z <= 100):
                        print(f"[WARN] Valor fora da faixa (0-100): y={y} z={z}")
                    xs.append(sample_idx)
                    y_vals.append(y)
                    z_vals.append(z)
                    k_vals.append(k)
                    if debug:
                        print(f"[DATA] idx={sample_idx} y={y} z={z} k={k}")
                    sample_idx += 1
        line_y.set_data(xs, y_vals)
        line_z.set_data(xs, z_vals)
        line_k.set_data(xs, k_vals)
        if xs:
            ax.set_xlim(xs[0], xs[-1] + 1)
        return line_y, line_z, line_k

    ani = animation.FuncAnimation(fig, update, interval=100, blit=False, save_count=max_pts)
    globals()["_ani"] = ani  # evitar GC

    try:
        plt.show()
    finally:
        stop_evt.set()
        reader.stop()
        print("[INFO] Saindo.")


# ----------------------------- CLI entry ------------------------------ #

def main():
    args = parse_args()
    if not Path(args.port).exists() and not args.port.upper().startswith("COM"):
        print(f"[WARN] A porta '{args.port}' não parece válida — verifique.")
    live_plot(args.port, args.baud, args.debug)


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        sys.exit(0)
