#!/usr/bin/env python3
"""Serial Plot Application (v10 – 4 curvas: Desejado, Sensor1, Sensor2, Mixer)

• Painel de LOG só mostra lines 'log'.  
• Parâmetro --debug continua igual.  
• Agora interpreta DATA‑<des‑val>‑<s1>‑<s2>‑<mixer>.  
"""

from __future__ import annotations
import argparse, queue, re, threading, sys
from collections import deque
from pathlib import Path
import serial, tkinter as tk
from tkinter import scrolledtext
import matplotlib

matplotlib.use("TkAgg")
from matplotlib import pyplot as plt, style, animation

# ---------- CLI / util --------------------------------------------------
def parse_args():
    ap = argparse.ArgumentParser(description="ESP32 serial plot + TX + painel LOG")
    ap.add_argument("--port", default="COM3")
    ap.add_argument("--baud", type=int, default=9600)
    g = ap.add_mutually_exclusive_group()
    g.add_argument("--debug",    dest="debug", action="store_true")
    g.add_argument("--no-debug", dest="debug", action="store_false")
    ap.set_defaults(debug=True); return ap.parse_args()

def dprint(debug: bool, *a, **k):   # helper
    if debug: print(*a, **k)

# ---------- Serial thread ----------------------------------------------
class SerialReader(threading.Thread):
    def __init__(self, port:str, baud:int, q:queue.Queue[str], debug:bool):
        super().__init__(daemon=True); self.q=q; self.debug=debug
        self.ser=serial.Serial(port,baud,timeout=1)
        print(f"[INFO] Serial aberta em {port} @ {baud} bps"); self._run=True
    def run(self):
        while self._run:
            try:
                ln=self.ser.readline().decode(errors="ignore")
                if ln: self.q.put(ln); dprint(self.debug,f"[RAW] {ln.rstrip()}")
            except serial.SerialException as e:
                self.q.put(f"E (Serial): {e}\n"); self.stop()
    def stop(self):
        self._run=False
        if self.ser.is_open: self.ser.close(); print("[INFO] Porta serial fechada")

# ---------- parsing -----------------------------------------------------
_RE_DATA=re.compile(r"^DATA-(.+)$")
_RE_BOOT=re.compile(r"^(load:|entry |rst:|clk_ets |configsip:|mode:|q_drv:|d_drv:|Boot|ESP-ROM)", re.I)

def parse_line(line:str):
    line=line.strip()
    if not line: return "log",""
    if (m:=_RE_DATA.match(line)):
        parts=[p for p in m.group(1).split("-") if p]
        try: vals=list(map(int,parts))
        except ValueError: return "error",f"ERRO-Invalid DATA: {line}"
        if len(vals)<4:    return "error",f"ERRO-Incomplete DATA: {line}"
        desejo,s1,s2,mix=vals[-4:]     # garante 4 valores
        mix = 0 if str(mix).lower() in ("0","false","f") else 1
        return "data",(desejo,s1,s2,mix)
    if _RE_BOOT.match(line):            return "log",line
    if line.lower().startswith("log-"): return "log",line.split("-",1)[1]
    if line.startswith("E ("):          return "error",f"ERRO-{line}"
    if line.startswith("ERROR-"):       return "error",line
    return "log",line

# ---------- GUI helpers -------------------------------------------------
def build_tx_entry(root, send_cb):
    f=tk.Frame(root); f.pack(side=tk.BOTTOM,fill=tk.X)
    e=tk.Entry(f); e.pack(side=tk.LEFT,expand=True,fill=tk.X,padx=4,pady=2)
    def _snd(ev=None): cmd=e.get().strip(); send_cb(cmd) if cmd else None; e.delete(0,tk.END)
    e.bind("<Return>",_snd); tk.Button(f,text="Send",command=_snd).pack(side=tk.RIGHT,padx=4)

def build_log_panel(root):
    st=scrolledtext.ScrolledText(root,height=8,state="disabled",wrap="word")
    st.pack(side=tk.BOTTOM,fill=tk.BOTH)
    def append(msg:str):
        st.config(state="normal"); st.insert("end",msg+"\n"); st.see("end"); st.config(state="disabled")
    return append

# ---------- main plot ---------------------------------------------------
def live_plot(port:str, baud:int, debug:bool, max_pts=200):
    rx_q=queue.Queue()
    reader=SerialReader(port,baud,rx_q,debug); reader.start()

    def serial_send(cmd:str):
        if cmd: reader.ser.write((cmd+"\n").encode()); dprint(debug,f"[TX] {cmd}")
    threading.Thread(target=lambda:[serial_send(l.strip()) for l in sys.stdin],daemon=True).start()

    xs,des,s1,s2,mix=(deque(maxlen=max_pts) for _ in range(5)); idx=0
    style.use("ggplot"); fig,ax=plt.subplots()
    ax.set(xlabel="Sample #",ylabel="Value",xlim=(0,max_pts),ylim=(0,100))
    lineD,=ax.plot([],"b-",lw=2,label="DESEJADO")        # azul
    lineS1,=ax.plot([],"r-",lw=2,label="SENSOR1")        # vermelho
    lineS2,=ax.plot([],"g-",lw=2,label="SENSOR2")        # verde
    lineM,=ax.plot([],"y-",lw=2,label="MIXER_STATUS")    # amarelo
    ax.legend(loc="upper left")

    root=fig.canvas.manager.window
    log_append=build_log_panel(root)
    build_tx_entry(root,serial_send)

    splitter=re.compile(r"(?:\\n|\n|/n)")

    def update(_):
        nonlocal idx
        while not rx_q.empty():
            for part in splitter.split(rx_q.get()):
                if not part: continue
                kind,pay=parse_line(part); dprint(debug,f"[PARSE] {kind}:{pay}")
                if kind=="log" and pay:
                    log_append(pay); print(f"[LOG] {pay}") if debug else None
                elif kind=="error" and debug:
                    print(pay)
                elif kind=="data":
                    desejo,s1v,s2v,mixv=pay
                    des.append(desejo); s1.append(s1v); s2.append(s2v); mix.append(mixv)
                    xs.append(idx); idx+=1
                    dprint(debug,f"[DATA] {pay}")
        lineD.set_data(xs,des); lineS1.set_data(xs,s1)
        lineS2.set_data(xs,s2); lineM.set_data(xs,mix)
        if xs: ax.set_xlim(xs[0], xs[-1]+1)
        return lineD,lineS1,lineS2,lineM

    ani=animation.FuncAnimation(fig,update,interval=100,blit=False,cache_frame_data=False)
    globals()['_ani']=ani
    try: plt.show()
    finally: reader.stop(); print("[INFO] Saindo.")

# ---------- entry -------------------------------------------------------
def main():
    a=parse_args()
    if not Path(a.port).exists() and not a.port.upper().startswith("COM"):
        print(f"[WARN] Porta '{a.port}' não parece válida — verifique.")
    live_plot(a.port,a.baud,a.debug)

if __name__=="__main__":
    try: main()
    except KeyboardInterrupt: sys.exit(0)
