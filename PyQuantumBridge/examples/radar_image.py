"""
radar_image.py — Display a live radar PPI image using matplotlib.

Run from PyQuantumBridge/:
    python examples/radar_image.py

Controls:
  Close the window to stop.
  Press 'c' while focused on the window to toggle Cartesian / polar view.

Requirements:
    pip install numpy scipy matplotlib
"""
import sys
import os
import time
import threading

sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "python"))

import numpy as np
import matplotlib.pyplot as plt
import matplotlib.colors as mcolors
import quantum_bridge as qb
from quantum.scanner import ScanBuffer

# --- Doppler colour map ---
# 0 = black, 1-253 = grey scale, 254 = green (receding), 255 = red (approaching)
_cmap_data = np.zeros((256, 4))
_cmap_data[:, 3] = 1.0                                    # alpha
grey = np.linspace(0, 1, 253)
_cmap_data[1:254, 0] = grey                               # R
_cmap_data[1:254, 1] = grey                               # G
_cmap_data[1:254, 2] = grey                               # B
_cmap_data[254] = [0.0, 1.0, 0.0, 1.0]                   # Doppler receding = green
_cmap_data[255] = [1.0, 0.2, 0.2, 1.0]                   # Doppler approaching = red
RADAR_CMAP = mcolors.ListedColormap(_cmap_data)

DISPLAY_CARTESIAN = True
CART_SIZE = 512


class RadarImageClient(qb.INotify):
    def __init__(self, buf: ScanBuffer):
        super().__init__()
        self.serial = None
        self.buf = buf
        self.scanner_ready = threading.Event()

    def scanner_list_changed(self, count):
        if count > 0:
            details = qb.get_scanner_details(0)
            self.serial = details.serial_number
            print(f"Scanner: {details.description}  0x{self.serial:08X}")
            self.scanner_ready.set()
            cmd = qb.SettingData.radar_mode(self.serial, qb.RadarMode.TRANSMITTING)
            qb.new_setting(cmd)
        else:
            self.serial = None
            self.scanner_ready.clear()

    def spoke_data_received(self, serial, spoke):
        self.buf.add_spoke(spoke)

    def setting_changed(self, sd):
        if sd.get_setting() == qb.Setting.RADAR_MODE:
            print(f"RadarMode -> {sd.get_radar_mode()}")


def main():
    global DISPLAY_CARTESIAN

    buf = ScanBuffer()
    client = RadarImageClient(buf)

    err = qb.open()
    if err != qb.ErrorCode.NO_ERROR:
        print(f"open() failed: {err}"); sys.exit(1)
    qb.register_notifications(client)

    print("Waiting for scanner…")
    if not client.scanner_ready.wait(timeout=15):
        print("No scanner found."); qb.deregister_notifications(client); qb.close(); sys.exit(1)
    print("Transmitting — building PPI image…")

    # Set up matplotlib interactive window
    plt.ion()
    fig, ax = plt.subplots(figsize=(7, 7))
    fig.patch.set_facecolor("black")
    ax.set_facecolor("black")
    ax.set_title("Quantum Radar — PPI (press C to toggle Cartesian/polar)", color="white")
    ax.tick_params(colors="white")

    placeholder = np.zeros((qb.SPOKES_PER_SCAN, qb.SAMPLES_PER_SPOKE), dtype=np.uint8)
    im = ax.imshow(placeholder, cmap=RADAR_CMAP, vmin=0, vmax=255,
                   origin="upper", aspect="auto", interpolation="none")
    plt.colorbar(im, ax=ax, label="Intensity")
    fig.tight_layout()

    def on_key(event):
        global DISPLAY_CARTESIAN
        if event.key == 'c':
            DISPLAY_CARTESIAN = not DISPLAY_CARTESIAN
            mode = "Cartesian" if DISPLAY_CARTESIAN else "polar"
            print(f"Display mode: {mode}")

    fig.canvas.mpl_connect("key_press_event", on_key)

    try:
        while plt.fignum_exists(fig.number):
            scan = buf.get_scan()
            if scan is not None:
                if DISPLAY_CARTESIAN:
                    try:
                        display = buf.to_cartesian(size=CART_SIZE)
                        if display is not None:
                            im.set_data(display)
                            if im.get_extent() != [0, CART_SIZE, CART_SIZE, 0]:
                                im.set_extent([0, CART_SIZE, CART_SIZE, 0])
                    except RuntimeError as e:
                        print(f"Cartesian conversion: {e}")
                        DISPLAY_CARTESIAN = False
                else:
                    im.set_data(scan)
                    if im.get_extent() != [0, qb.SAMPLES_PER_SPOKE, qb.SPOKES_PER_SCAN, 0]:
                        im.set_extent([0, qb.SAMPLES_PER_SPOKE, qb.SPOKES_PER_SCAN, 0])
                ax.set_title(
                    f"Quantum Radar — {'Cartesian' if DISPLAY_CARTESIAN else 'polar (C to toggle)'}",
                    color="white")
                fig.canvas.draw_idle()
            plt.pause(0.2)
    except KeyboardInterrupt:
        pass
    finally:
        print("Stopping…")
        if client.serial is not None:
            qb.new_setting(qb.SettingData.radar_mode(client.serial, qb.RadarMode.STANDBY))
            time.sleep(1)
        qb.deregister_notifications(client)
        qb.close()
        print("Done.")


if __name__ == "__main__":
    main()
