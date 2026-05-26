"""
main.py — Quantum Radar Control Workstation

Combines live PPI display, point-and-click settings panel, and data recording
in a single window. No radar documentation or Python knowledge required.

Run from PyQuantumBridge/:
    python main.py

Requires: numpy, matplotlib, scipy, tkinter (stdlib)
"""

import sys
import os
import time
import threading
import queue
import json
from datetime import datetime
from pathlib import Path

import numpy as np
import matplotlib.pyplot as plt
import matplotlib.colors as mcolors
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from scipy.ndimage import map_coordinates

import tkinter as tk
from tkinter import ttk

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "python"))

import quantum_bridge as qb
from quantum.scanner import ScanBuffer

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------
FLUSH_SCANS        = 10
CART_SIZE          = 512
PPI_UPDATE_MS      = 250

# ---------------------------------------------------------------------------
# Colour maps
# ---------------------------------------------------------------------------
# Greyscale (original)
_cm_grey = np.zeros((256, 4))
_cm_grey[:, 3] = 1.0
_cm_grey[1:254, :3] = np.linspace(0, 1, 253)[:, None]
_cm_grey[254] = [0.0, 1.0, 0.0, 1.0]   # Doppler receding → green
_cm_grey[255] = [1.0, 0.2, 0.2, 1.0]   # Doppler approaching → red
RADAR_CMAP_GREY = mcolors.ListedColormap(_cm_grey)

# Colour: black → green → yellow → orange → red  (like QuantumClientPPI)
_cm_col = np.zeros((256, 4))
_cm_col[:, 3] = 1.0
_t = np.linspace(0, 1, 85)
_cm_col[1:86,   1] = 0.3 + 0.7 * _t    # G: 0.3 → 1.0  (green band)
_t = np.linspace(0, 1, 85)
_cm_col[86:171, 0] = _t                  # R: 0 → 1.0
_cm_col[86:171, 1] = 1.0                 # G: 1.0        (yellow band)
_t = np.linspace(0, 1, 83)
_cm_col[171:254, 0] = 1.0               # R: 1.0
_cm_col[171:254, 1] = 1.0 - _t         # G: 1.0 → 0    (red band)
_cm_col[254] = [0.0, 1.0, 1.0, 1.0]    # Doppler receding → cyan
_cm_col[255] = [1.0, 0.0, 1.0, 1.0]    # Doppler approaching → magenta
RADAR_CMAP_COLOUR = mcolors.ListedColormap(_cm_col)

RADAR_CMAP = RADAR_CMAP_COLOUR           # default

# ---------------------------------------------------------------------------
# Polar → Cartesian  (copied from python/quantum/scanner.py)
# ---------------------------------------------------------------------------
def polar_to_cartesian(scan: np.ndarray, size: int = CART_SIZE) -> np.ndarray:
    spokes, samples = scan.shape
    half = size / 2.0
    ys, xs = np.mgrid[0:size, 0:size]
    dx = (xs - half) / half
    dy = (half - ys) / half
    r     = np.sqrt(dx**2 + dy**2)
    theta = np.arctan2(dx, dy) % (2 * np.pi)
    spoke_coords  = theta / (2 * np.pi) * spokes
    sample_coords = np.clip(r * samples, 0, samples - 1)
    mask   = r > 1.0
    coords = np.array([spoke_coords.ravel(), sample_coords.ravel()])
    out    = map_coordinates(scan, coords, order=1, mode="wrap", prefilter=False)
    out    = out.reshape(size, size).astype(np.uint8)
    out[mask] = 0
    return out

# ---------------------------------------------------------------------------
# Enum option lists  [display label, qb enum value]
# ---------------------------------------------------------------------------
GAIN_MODES    = [("Auto",    qb.GainMode.AUTO),    ("Manual", qb.GainMode.MANUAL)]
SEA_MODES     = [("Auto",    qb.SeaMode.AUTO),     ("Manual", qb.SeaMode.MANUAL)]
RAIN_MODES    = [("Off",     qb.RainMode.OFF),     ("Manual", qb.RainMode.MANUAL)]
DOPPLER_MODES = [("Off",     qb.DopplerMode.OFF),  ("On",     qb.DopplerMode.ON)]
TARGET_EXP    = [("Off",     qb.TargetExpansion.OFF), ("On",  qb.TargetExpansion.ON)]
SEA_CURVES    = [("R4",      qb.SeaCurve.R4),      ("R5.5",  qb.SeaCurve.R5_5)]
MAIN_BANGS    = [("Off",     qb.MainBang.OFF),     ("On",    qb.MainBang.ON)]
IR_MODES      = [("Off",     qb.InterferenceRejection.OFF),
                 ("Low",     qb.InterferenceRejection.LEVEL_1),
                 ("Medium",  qb.InterferenceRejection.LEVEL_2),
                 ("High",    qb.InterferenceRejection.LEVEL_3),
                 ("Level 4", qb.InterferenceRejection.LEVEL_4),
                 ("Level 5", qb.InterferenceRejection.LEVEL_5)]
TX_FREQS      = [("Nominal", qb.TransmitFrequency.NOMINAL),
                 ("Low",     qb.TransmitFrequency.LOW),
                 ("High",    qb.TransmitFrequency.HIGH)]
PRESETS       = [("Harbour",  qb.Preset.HARBOUR),
                 ("Coastal",  qb.Preset.COASTAL),
                 ("Offshore", qb.Preset.OFFSHORE),
                 ("Weather",  qb.Preset.WEATHER)]
AUTO_ACQ      = [("Off",     qb.AutoAcquireMode.OFF),
                 ("Zone",    qb.AutoAcquireMode.ZONE),
                 ("Doppler", qb.AutoAcquireMode.DOPPLER)]

# Map Setting enum → lambda that extracts a safe Python value from SettingData.
# Values extracted here (enum objects, ints, floats, lists) are all Python-owned
# and safe to hold after the C++ SettingData object is gone.
_SETTING_GETTERS = {
    qb.Setting.RADAR_MODE:             lambda sd: sd.get_radar_mode(),
    qb.Setting.RANGE:                  lambda sd: sd.get_range_index(),
    qb.Setting.GAIN_MODE:              lambda sd: sd.get_gain_mode(),
    qb.Setting.GAIN_VALUE:             lambda sd: sd.get_gain_value(),
    qb.Setting.SEA_MODE:               lambda sd: sd.get_sea_mode(),
    qb.Setting.SEA_VALUE:              lambda sd: sd.get_sea_value(),
    qb.Setting.RAIN_MODE:              lambda sd: sd.get_rain_mode(),
    qb.Setting.RAIN_VALUE:             lambda sd: sd.get_rain_value(),
    qb.Setting.INTERFERENCE_REJECTION: lambda sd: sd.get_interference_rejection(),
    qb.Setting.DOPPLER_MODE:           lambda sd: sd.get_doppler_mode(),
    qb.Setting.TX_FREQUENCY:           lambda sd: sd.get_tx_frequency(),
    qb.Setting.TARGET_EXPANSION:       lambda sd: sd.get_target_expansion(),
    qb.Setting.SEA_CLUTTER_CURVE:      lambda sd: sd.get_sea_clutter_curve(),
    qb.Setting.MAIN_BANG:              lambda sd: sd.get_main_bang(),
    qb.Setting.AUTO_ACQUIRE_MODE:      lambda sd: sd.get_auto_acquire_mode(),
    qb.Setting.PRESET_MODE:            lambda sd: sd.get_preset_mode(),
    qb.Setting.BEARING_ALIGNMENT:      lambda sd: sd.get_bearing_alignment(),
    qb.Setting.CUSTOM_RANGES:          lambda sd: sd.get_custom_ranges(),
    qb.Setting.MAIN_SOFTWARE_VERSION:  lambda sd: sd.get_software_version(),
    qb.Setting.PSU_SOFTWARE_VERSION:   lambda sd: sd.get_software_version(),
    qb.Setting.FPGA_SOFTWARE_VERSION:  lambda sd: sd.get_software_version(),
    qb.Setting.WIFI_SOFTWARE_VERSION:  lambda sd: sd.get_software_version(),
    qb.Setting.W3_SOFTWARE_VERSION:    lambda sd: sd.get_software_version(),
}


def _label_for(options, enum_val) -> str | None:
    sv = str(enum_val)
    for label, ev in options:
        if str(ev) == sv:
            return label
    return None


# ---------------------------------------------------------------------------
# Colours
# ---------------------------------------------------------------------------
BG  = "#1a1a1a"
BG2 = "#2a2a2a"
BG3 = "#111111"
FG  = "#dddddd"
FG2 = "#777777"
SEL = "#336699"


# ---------------------------------------------------------------------------
# RadarController  — INotify backend, owns scan buffer + recording pipeline
# ---------------------------------------------------------------------------
class RadarController(qb.INotify):

    def __init__(self, app: "RadarApp"):
        super().__init__()
        self._app        = app
        self.serial      = None
        self.updating_ui = False   # True while pushing radar values into UI vars
        self.scan_buffer = ScanBuffer()

        # Recording state — reset per session
        self._recording  = False
        self._rec_lock   = threading.Lock()
        self._out_dir    = None
        self._events_file   = None
        self._events_lock   = threading.Lock()
        self._rec_buf_bearings   = []
        self._rec_buf_timestamps = []
        self._rec_buf_ranges     = []
        self._rec_buf_channels   = []
        self._rec_buf_lengths    = []
        self._rec_buf_data       = []
        self._rec_last_bearing   = -1
        self._rec_scan_count     = 0
        self._rec_total_spokes   = 0
        self._rec_file_index     = 0
        self._rec_start_time     = ""
        self._write_queue        = None
        self._writer             = None

    # ------------------------------------------------------------------
    # INotify callbacks  (DLL thread — no tkinter access here)
    # ------------------------------------------------------------------

    def scanner_list_changed(self, count):
        if count > 0:
            details = qb.get_scanner_details(0)
            self.serial = details.serial_number
            self._app.root.after(0, lambda d=details: self._app.on_scanner_found(d))
        else:
            self.serial = None
            self._app.root.after(0, self._app.on_scanner_lost)

    def spoke_data_received(self, serial, spoke):
        self.scan_buffer.add_spoke(spoke)
        if self._recording:
            self._record_spoke(spoke)

    def setting_changed(self, sd):
        setting = sd.get_setting()
        # Extract values while sd is still valid (C++ object)
        getter = _SETTING_GETTERS.get(setting)
        value  = None
        try:
            if getter:
                value = getter(sd)
        except Exception:
            pass
        if self._recording:
            self._log_event("setting",
                setting=str(setting),
                value=str(value),
                channel=str(sd.get_channel()))
        if sd.get_setting() == qb.Setting.CUSTOM_RANGES:
            ranges = sd.get_custom_ranges()
            valid = [(i, r) for i, r in enumerate(ranges) if r > 0]
            print("Available ranges:", valid)

        self._app.root.after(
            0, lambda s=setting, v=value: self._app.apply_setting_value(s, v))

    def feature_changed(self, serial, feature, supported):
        if self._recording:
            self._log_event("feature", feature=str(feature), supported=supported)

    def parameter_changed(self, serial, parameter, value):
        if self._recording:
            self._log_event("parameter", parameter=str(parameter), value=value)

    def marpa_data_changed(self, serial, data):
        if self._recording:
            self._log_event("marpa",
                id=data.id, valid=data.valid,
                state=str(data.state),
                range_m=data.target_range_m,
                bearing_degs=data.target_bearing_degs,
                cpa_m=data.closest_point_of_approach_m,
                tcpa_secs=data.time_to_closest_point_secs)

    def alarm_data_changed(self, serial, alarm):
        if self._recording:
            self._log_event("alarm", id=alarm.id,
                            alarm_type=str(alarm.type), value=alarm.value)
        msg = f"Alarm: {str(alarm.type).replace('AlarmType.', '')} (id={alarm.id})"
        self._app.root.after(0, lambda m=msg: self._app.set_status(m))

    # ------------------------------------------------------------------
    # Recording
    # ------------------------------------------------------------------

    def start_recording(self, out_dir: Path):
        out_dir.mkdir(parents=True, exist_ok=True)
        self._out_dir          = out_dir
        self._rec_scan_count   = 0
        self._rec_total_spokes = 0
        self._rec_file_index   = 0
        self._rec_last_bearing = -1
        with self._rec_lock:
            self._rec_buf_bearings   = []
            self._rec_buf_timestamps = []
            self._rec_buf_ranges     = []
            self._rec_buf_channels   = []
            self._rec_buf_lengths    = []
            self._rec_buf_data       = []

        self._rec_start_time = datetime.now().isoformat()
        self._events_file = open(out_dir / "events.jsonl", "w",
                                  buffering=1, encoding="utf-8")
        # Log session start
        self._log_event("session_start",
            serial=f"0x{self.serial:08X}" if self.serial else "?")

        self._write_queue = queue.Queue()
        self._writer = threading.Thread(target=self._writer_thread, daemon=True)
        self._writer.start()
        self._recording = True

    def stop_recording(self) -> Path:
        self._recording = False
        with self._rec_lock:
            remaining    = self._swap_rec_buffers()
            total_scans  = self._rec_scan_count
            total_spokes = self._rec_total_spokes

        if remaining[0]:
            self._write_queue.put(remaining)
        self._write_queue.put(None)   # stop sentinel for writer thread
        self._writer.join(timeout=60) # wait for all saves to finish

        session = {
            "start_time":   self._rec_start_time,
            "end_time":     datetime.now().isoformat(),
            "serial_number": f"0x{self.serial:08X}" if self.serial else "?",
            "total_scans":  total_scans,
            "total_spokes": total_spokes,
            "spoke_files":  self._rec_file_index,
            "flush_scans":  FLUSH_SCANS,
        }

        self._events_file.close()
        self._events_file = None

        with open(self._out_dir / "session.json", "w", encoding="utf-8") as f:
            json.dump(session, f, indent=2)

        out = self._out_dir
        self._out_dir = None
        return out

    def flush_recording(self):
        if self._recording:
            try:
                self.stop_recording()
            except Exception:
                pass

    def rec_stats(self):
        with self._rec_lock:
            return self._rec_scan_count, self._rec_total_spokes, self._rec_file_index

    def _record_spoke(self, spoke):
        ts      = time.time()
        bearing = spoke.bearing
        data    = spoke.to_numpy().copy()
        to_flush = None
        with self._rec_lock:
            self._rec_buf_bearings.append(bearing)
            self._rec_buf_timestamps.append(ts)
            self._rec_buf_ranges.append(spoke.instrumented_range)
            self._rec_buf_channels.append(spoke.channel)
            self._rec_buf_lengths.append(spoke.data_length)
            self._rec_buf_data.append(data)
            self._rec_total_spokes += 1
            if bearing < self._rec_last_bearing:
                self._rec_scan_count += 1
                if self._rec_scan_count % FLUSH_SCANS == 0:
                    to_flush = self._swap_rec_buffers()
            self._rec_last_bearing = bearing
        if to_flush is not None:
            self._write_queue.put(to_flush)

    def _swap_rec_buffers(self):
        batch = (self._rec_buf_bearings, self._rec_buf_timestamps,
                 self._rec_buf_ranges,   self._rec_buf_channels,
                 self._rec_buf_lengths,  self._rec_buf_data)
        self._rec_buf_bearings   = []
        self._rec_buf_timestamps = []
        self._rec_buf_ranges     = []
        self._rec_buf_channels   = []
        self._rec_buf_lengths    = []
        self._rec_buf_data       = []
        return batch

    def _writer_thread(self):
        while True:
            batch = self._write_queue.get()
            if batch is None:
                break
            self._save_batch(batch)

    def _save_batch(self, batch):
        bearings, timestamps, ranges, channels, lengths, data_list = batch
        if not bearings or self._out_dir is None:
            return
        n = len(bearings)
        spoke_arr = np.zeros((n, 1024), dtype=np.uint8)
        for i, d in enumerate(data_list):
            spoke_arr[i, :len(d)] = d
        self._rec_file_index += 1
        path = self._out_dir / f"spokes_{self._rec_file_index:04d}.npz"
        np.savez_compressed(
            path,
            bearings            = np.array(bearings,   dtype=np.uint16),
            timestamps          = np.array(timestamps, dtype=np.float64),
            instrumented_ranges = np.array(ranges,     dtype=np.uint16),
            channels            = np.array(channels,   dtype=np.uint8),
            data_lengths        = np.array(lengths,    dtype=np.uint16),
            spoke_data          = spoke_arr,
        )

    def _log_event(self, event_type, **kwargs):
        if not self._events_file:
            return
        entry = {"timestamp": datetime.now().isoformat(), "type": event_type, **kwargs}
        line  = json.dumps(entry, default=str) + "\n"
        with self._events_lock:
            self._events_file.write(line)


# ---------------------------------------------------------------------------
# RadarApp  — tkinter window
# ---------------------------------------------------------------------------
class RadarApp:

    def __init__(self, root: tk.Tk):
        self.root = root
        self._setup_theme()
        self._range_labels = [f"Range {i}" for i in range(21)]

        # ---- Main layout ----
        main = tk.Frame(root, bg=BG)
        main.pack(fill="both", expand=True)

        ppi_frame = tk.Frame(main, bg=BG3, bd=0)
        ppi_frame.pack(side="left", fill="both", expand=True, padx=(6, 3), pady=6)
        self._build_ppi(ppi_frame)

        right = tk.Frame(main, bg=BG, width=320)
        right.pack(side="right", fill="y", padx=(3, 6), pady=6)
        right.pack_propagate(False)
        self._build_settings(right)

        # Status bar
        self._status_var = tk.StringVar(value="Opening radar library...")
        bar = tk.Frame(root, bg=BG3, height=22)
        bar.pack(fill="x")
        bar.pack_propagate(False)
        tk.Label(bar, textvariable=self._status_var, bg=BG3, fg=FG2,
                 font=("Consolas", 8), anchor="w").pack(fill="x", padx=8, pady=2)

        # Controller
        self._ctrl = RadarController(self)

        # Start loops
        self._update_ppi()
        self._update_rec_stats()

        # Connect in background
        threading.Thread(target=self._connect, daemon=True).start()
        root.protocol("WM_DELETE_WINDOW", self._on_close)

    # ------------------------------------------------------------------
    # Theme
    # ------------------------------------------------------------------

    def _setup_theme(self):
        s = ttk.Style()
        s.theme_use("clam")
        s.configure(".",         background=BG,  foreground=FG)
        s.configure("TFrame",    background=BG)
        s.configure("TLabel",    background=BG,  foreground=FG)
        s.configure("TButton",   background=BG2, foreground=FG, borderwidth=1)
        s.configure("TCombobox", fieldbackground=BG2, foreground=FG,
                    selectbackground=SEL, selectforeground=FG)
        s.configure("TScrollbar", background=BG2, troughcolor=BG3, arrowcolor=FG2)
        s.map("TCombobox",
              fieldbackground=[("readonly", BG2), ("disabled", BG3)],
              foreground=[("disabled", FG2)])
        s.map("TButton", background=[("active", BG2)])

    # ------------------------------------------------------------------
    # PPI panel
    # ------------------------------------------------------------------

    def _build_ppi(self, parent):
        fig, ax = plt.subplots(figsize=(5.5, 5.5), dpi=100)
        fig.patch.set_facecolor(BG3)
        ax.set_facecolor(BG3)
        ax.set_xticks([]); ax.set_yticks([])
        for sp in ax.spines.values():
            sp.set_visible(False)
        ax.set_title("Live PPI", color=FG2, fontsize=9, pad=4)

        blank = np.zeros((CART_SIZE, CART_SIZE), dtype=np.uint8)
        self._im = ax.imshow(blank, cmap=RADAR_CMAP, vmin=0, vmax=255,
                             origin="upper", aspect="equal", interpolation="nearest")

        self._vrm_angles = np.linspace(0, 2 * np.pi, 129)
        self._vrm = 1024 * 2 / 3
        (self._vrm_line,) = ax.plot([], [], color='yellow', linewidth=1.0, alpha=0.85)

        self._fig    = fig
        self._canvas = FigureCanvasTkAgg(fig, master=parent)
        self._canvas.get_tk_widget().pack(fill="both", expand=True)

        self._use_colour = True
        self._btn_cmap = tk.Button(
            parent, text="Switch to Greyscale",
            bg=BG2, fg=FG2, font=("Arial", 8), bd=0, relief="flat",
            command=self._toggle_cmap)
        self._btn_cmap.pack(side="bottom", anchor="se", padx=6, pady=2)

    def _toggle_cmap(self):
        self._use_colour = not self._use_colour
        self._im.set_cmap(RADAR_CMAP_COLOUR if self._use_colour else RADAR_CMAP_GREY)
        self._btn_cmap.configure(
            text="Switch to Greyscale" if self._use_colour else "Switch to Colour")
        self._canvas.draw_idle()

    # ------------------------------------------------------------------
    # Settings panel
    # ------------------------------------------------------------------

    def _build_settings(self, parent):
        # Scrollable canvas pattern
        scroll_canvas = tk.Canvas(parent, bg=BG, highlightthickness=0, bd=0)
        vsb = ttk.Scrollbar(parent, orient="vertical", command=scroll_canvas.yview)
        inner = tk.Frame(scroll_canvas, bg=BG)
        scroll_canvas.create_window((0, 0), window=inner, anchor="nw")
        scroll_canvas.configure(yscrollcommand=vsb.set)
        inner.bind("<Configure>",
                   lambda e: scroll_canvas.configure(
                       scrollregion=scroll_canvas.bbox("all")))
        scroll_canvas.bind_all(
            "<MouseWheel>",
            lambda e: scroll_canvas.yview_scroll(-1 * (e.delta // 120), "units"))
        vsb.pack(side="right", fill="y")
        scroll_canvas.pack(side="left", fill="both", expand=True)

        self._setting_widgets = []  # for bulk enable/disable

        # ---- OPERATION ----
        self._section(inner, "OPERATION")
        btn_row = tk.Frame(inner, bg=BG)
        btn_row.pack(fill="x", padx=8, pady=(4, 2))
        self._btn_tx = tk.Button(btn_row, text="  TRANSMIT  ",
                                  bg="#1a3a1a", fg="#33ee55",
                                  activebackground="#264426", activeforeground="#33ee55",
                                  font=("Arial", 9, "bold"), bd=1, relief="raised",
                                  command=self._on_transmit)
        self._btn_tx.pack(side="left", padx=(0, 4))
        self._btn_sb = tk.Button(btn_row, text="  STANDBY  ",
                                  bg="#3a1a1a", fg="#ee4444",
                                  activebackground="#442626", activeforeground="#ee4444",
                                  font=("Arial", 9, "bold"), bd=1, relief="raised",
                                  command=self._on_standby)
        self._btn_sb.pack(side="left")
        self._setting_widgets += [self._btn_tx, self._btn_sb]

        self._range_var = tk.StringVar(value="Range 0")
        self._last_valid_range_label = "Range 0"
        row = self._row(inner, "Range")
        self._range_combo = self._combo(row, self._range_var, self._range_labels)
        self._range_var.trace_add("write", self._on_range)
        self._setting_widgets.append(self._range_combo)

        # ---- NAVIGATION DATA ----
        self._section(inner, "NAVIGATION DATA")

        def _nav_entry(label, default):
            f = tk.Frame(inner, bg=BG)
            f.pack(fill="x", padx=8, pady=1)
            tk.Label(f, text=label + ":", bg=BG, fg=FG2,
                     font=("Arial", 8), width=14, anchor="w").pack(side="left")
            var = tk.StringVar(value=default)
            e = tk.Entry(f, textvariable=var, width=8, bg=BG2, fg=FG,
                         insertbackground=FG, relief="flat", font=("Consolas", 9))
            e.pack(side="left", padx=(0, 4))
            return var, e

        self._nav_heading_var, _nav_hdg_e = _nav_entry("Heading (°)", "0.0")
        self._nav_sog_var,     _nav_sog_e = _nav_entry("SOG (knots)", "0.0")
        self._nav_cog_var,     _nav_cog_e = _nav_entry("COG (°)",     "0.0")
        self._nav_btn = tk.Button(inner, text="Send Nav Data",
                                   bg="#1a2a2a", fg="#44cccc",
                                   activebackground="#263636", activeforeground="#66dddd",
                                   font=("Arial", 8, "bold"), bd=1,
                                   command=self._on_nav_data)
        self._nav_btn.pack(fill="x", padx=8, pady=(3, 1))
        tk.Label(inner,
                 text="  Heading sets radar north. SOG = 0 when stationary.",
                 bg=BG, fg=FG2, font=("Arial", 7), anchor="w").pack(fill="x", padx=8)
        self._setting_widgets += [_nav_hdg_e, _nav_sog_e, _nav_cog_e, self._nav_btn]

        # ---- GAIN ----
        self._section(inner, "GAIN")
        self._gain_mode_var  = tk.StringVar(value="Auto")
        self._gain_value_var = tk.IntVar(value=50)
        row = self._row(inner, "Mode")
        self._gain_mode_cb = self._combo(row, self._gain_mode_var, [l for l,_ in GAIN_MODES])
        self._gain_mode_var.trace_add("write", self._on_gain_mode)
        self._gain_sl = self._slider(inner, "Value", self._gain_value_var, 0, 100,
                                     self._on_gain_value)
        self._setting_widgets += [self._gain_mode_cb, self._gain_sl]

        # ---- SEA CLUTTER ----
        self._section(inner, "SEA CLUTTER")
        self._sea_mode_var  = tk.StringVar(value="Manual")
        self._sea_value_var = tk.IntVar(value=0)
        row = self._row(inner, "Mode")
        self._sea_mode_cb = self._combo(row, self._sea_mode_var, [l for l,_ in SEA_MODES])
        self._sea_mode_var.trace_add("write", self._on_sea_mode)
        self._sea_sl = self._slider(inner, "Value", self._sea_value_var, 0, 100,
                                    self._on_sea_value)
        self._setting_widgets += [self._sea_mode_cb, self._sea_sl]

        # ---- RAIN ----
        self._section(inner, "RAIN")
        self._rain_mode_var  = tk.StringVar(value="Off")
        self._rain_value_var = tk.IntVar(value=0)
        row = self._row(inner, "Mode")
        self._rain_mode_cb = self._combo(row, self._rain_mode_var, [l for l,_ in RAIN_MODES])
        self._rain_mode_var.trace_add("write", self._on_rain_mode)
        self._rain_sl = self._slider(inner, "Value", self._rain_value_var, 0, 100,
                                     self._on_rain_value)
        self._setting_widgets += [self._rain_mode_cb, self._rain_sl]

        # ---- SIGNAL PROCESSING ----
        self._section(inner, "SIGNAL PROCESSING")
        self._ir_var       = tk.StringVar(value="Off")
        self._doppler_var  = tk.StringVar(value="Off")
        self._txfreq_var   = tk.StringVar(value="Nominal")
        self._tgtexp_var   = tk.StringVar(value="Off")
        self._seacurve_var = tk.StringVar(value="R4")
        self._mainbang_var = tk.StringVar(value="Off")
        self._autoacq_var  = tk.StringVar(value="Off")

        for lbl, var, opts, cb in [
            ("Interference",  self._ir_var,       [l for l,_ in IR_MODES],      self._on_ir),
            ("Doppler Mode",  self._doppler_var,  [l for l,_ in DOPPLER_MODES], self._on_doppler),
            ("TX Frequency",  self._txfreq_var,   [l for l,_ in TX_FREQS],      self._on_txfreq),
            ("Target Expand", self._tgtexp_var,   [l for l,_ in TARGET_EXP],    self._on_tgtexp),
            ("Sea Curve",     self._seacurve_var, [l for l,_ in SEA_CURVES],    self._on_seacurve),
            ("Main Bang",     self._mainbang_var, [l for l,_ in MAIN_BANGS],    self._on_mainbang),
            ("Auto-Acquire",  self._autoacq_var,  [l for l,_ in AUTO_ACQ],      self._on_autoacq),
        ]:
            row = self._row(inner, lbl)
            combo = self._combo(row, var, opts)
            var.trace_add("write", cb)
            self._setting_widgets.append(combo)

        # ---- DISPLAY / CALIBRATION ----
        self._section(inner, "DISPLAY / CALIBRATION")
        self._preset_var  = tk.StringVar(value="Harbour")
        self._bearing_var = tk.IntVar(value=0)
        row = self._row(inner, "Preset")
        self._preset_cb = self._combo(row, self._preset_var, [l for l,_ in PRESETS])
        self._preset_var.trace_add("write", self._on_preset)
        self._bearing_sl = self._slider(inner, "Bearing (×0.5°)", self._bearing_var,
                                        -360, 360, self._on_bearing)
        self._setting_widgets += [self._preset_cb, self._bearing_sl]

        # ---- RECORDING ----
        self._section(inner, "RECORDING")
        self._rec_btn = tk.Button(inner, text="Start Recording",
                                   bg="#1a2a3a", fg="#4488ff",
                                   activebackground="#263646", activeforeground="#66aaff",
                                   font=("Arial", 9, "bold"), bd=1,
                                   command=self._toggle_recording)
        self._rec_btn.pack(fill="x", padx=8, pady=(4, 1))
        self._rec_dir_var   = tk.StringVar(value="(not recording)")
        self._rec_stats_var = tk.StringVar(value="")
        tk.Label(inner, textvariable=self._rec_dir_var, bg=BG, fg=FG2,
                 font=("Consolas", 7), anchor="w", wraplength=290,
                 justify="left").pack(fill="x", padx=8)
        tk.Label(inner, textvariable=self._rec_stats_var, bg=BG, fg=FG2,
                 font=("Consolas", 8), anchor="w").pack(fill="x", padx=8, pady=(0, 10))

        self._set_enabled(False)

    # ------------------------------------------------------------------
    # Layout helpers
    # ------------------------------------------------------------------

    def _section(self, parent, title: str):
        f = tk.Frame(parent, bg=SEL, pady=2)
        f.pack(fill="x", padx=5, pady=(10, 2))
        tk.Label(f, text=title, bg=SEL, fg="white",
                 font=("Arial", 8, "bold")).pack(side="left", padx=6)

    def _row(self, parent, label: str) -> tk.Frame:
        f = tk.Frame(parent, bg=BG)
        f.pack(fill="x", padx=8, pady=1)
        tk.Label(f, text=label + ":", bg=BG, fg=FG2,
                 font=("Arial", 8), width=14, anchor="w").pack(side="left")
        return f

    def _combo(self, parent: tk.Frame, var: tk.StringVar,
               values: list, width: int = 13) -> ttk.Combobox:
        cb = ttk.Combobox(parent, textvariable=var, values=values,
                          state="readonly", width=width, font=("Arial", 8))
        cb.pack(side="left")
        return cb

    def _slider(self, parent, label: str, var: tk.IntVar,
                from_: int, to: int, command) -> tk.Scale:
        f = tk.Frame(parent, bg=BG)
        f.pack(fill="x", padx=8, pady=1)
        tk.Label(f, text=label + ":", bg=BG, fg=FG2,
                 font=("Arial", 8), width=14, anchor="w").pack(side="left")
        val_lbl = tk.Label(f, textvariable=var, bg=BG, fg=FG,
                           font=("Consolas", 8), width=4, anchor="e")
        val_lbl.pack(side="right")
        s = tk.Scale(f, variable=var, from_=from_, to=to, orient="horizontal",
                     command=command, showvalue=False, length=120,
                     bg=BG, fg=FG, troughcolor=BG2, highlightthickness=0,
                     activebackground=SEL, sliderlength=12, bd=0)
        s.pack(side="left", fill="x", expand=True)
        return s

    def _set_enabled(self, enabled: bool):
        for w in self._setting_widgets:
            try:
                if isinstance(w, ttk.Combobox):
                    w.configure(state="readonly" if enabled else "disabled")
                else:
                    w.configure(state="normal" if enabled else "disabled")
            except tk.TclError:
                pass

    # ------------------------------------------------------------------
    # Update loops  (main thread)
    # ------------------------------------------------------------------

    def _compute_vrm_xy(self):
        half = CART_SIZE / 2.0
        dr = (self._vrm / 1024.0) * half
        xs = dr * np.sin(self._vrm_angles) + half
        ys = half - dr * np.cos(self._vrm_angles)
        return xs, ys

    def _update_ppi(self):
        scan = self._ctrl.scan_buffer.get_scan()
        if scan is not None:
            try:
                cart = polar_to_cartesian(scan)
                self._im.set_data(cart)
                xs, ys = self._compute_vrm_xy()
                self._vrm_line.set_data(xs, ys)
                self._canvas.draw_idle()
            except Exception:
                pass
        self.root.after(PPI_UPDATE_MS, self._update_ppi)

    def _update_rec_stats(self):
        if self._ctrl._recording:
            scans, spokes, files = self._ctrl.rec_stats()
            self._rec_stats_var.set(
                f"Scans: {scans:,}   Spokes: {spokes:,}   Files: {files}")
        self.root.after(1000, self._update_rec_stats)

    # ------------------------------------------------------------------
    # Radar connection  (background thread → main thread via root.after)
    # ------------------------------------------------------------------

    def _connect(self):
        err = qb.open()
        if err != qb.ErrorCode.NO_ERROR:
            self.root.after(0, lambda: self.set_status(f"ERROR: qb.open() -> {err}"))
            return
        qb.register_notifications(self._ctrl)
        self.root.after(0, lambda: self.set_status("Searching for radar..."))

    def on_scanner_found(self, details):
        sn = f"0x{details.serial_number:08X}"
        self.set_status(f"Connected  —  {details.description}   SN: {sn}")
        self._set_enabled(True)

    def on_scanner_lost(self):
        self.set_status("Scanner lost — searching...")
        self._set_enabled(False)

    # ------------------------------------------------------------------
    # Apply settings received from radar → update widgets
    # ------------------------------------------------------------------

    def apply_setting_value(self, setting, value):
        ctrl = self._ctrl
        ctrl.updating_ui = True
        try:
            s = setting
            if   s == qb.Setting.RADAR_MODE:              self._apply_radar_mode(value)
            elif s == qb.Setting.GAIN_MODE:                self._sv(self._gain_mode_var, GAIN_MODES, value)
            elif s == qb.Setting.GAIN_VALUE:               self._gain_value_var.set(int(value))
            elif s == qb.Setting.SEA_MODE:                 self._sv(self._sea_mode_var, SEA_MODES, value)
            elif s == qb.Setting.SEA_VALUE:                self._sea_value_var.set(int(value))
            elif s == qb.Setting.RAIN_MODE:                self._sv(self._rain_mode_var, RAIN_MODES, value)
            elif s == qb.Setting.RAIN_VALUE:               self._rain_value_var.set(int(value))
            elif s == qb.Setting.INTERFERENCE_REJECTION:   self._sv(self._ir_var, IR_MODES, value)
            elif s == qb.Setting.DOPPLER_MODE:             self._sv(self._doppler_var, DOPPLER_MODES, value)
            elif s == qb.Setting.TX_FREQUENCY:             self._sv(self._txfreq_var, TX_FREQS, value)
            elif s == qb.Setting.TARGET_EXPANSION:         self._sv(self._tgtexp_var, TARGET_EXP, value)
            elif s == qb.Setting.SEA_CLUTTER_CURVE:        self._sv(self._seacurve_var, SEA_CURVES, value)
            elif s == qb.Setting.MAIN_BANG:                self._sv(self._mainbang_var, MAIN_BANGS, value)
            elif s == qb.Setting.AUTO_ACQUIRE_MODE:        self._sv(self._autoacq_var, AUTO_ACQ, value)
            elif s == qb.Setting.PRESET_MODE:              self._sv(self._preset_var, PRESETS, value)
            elif s == qb.Setting.BEARING_ALIGNMENT:        self._bearing_var.set(int(value))
            elif s == qb.Setting.RANGE:                    self._apply_range(int(value))
            elif s == qb.Setting.CUSTOM_RANGES:            self._apply_custom_ranges(value)
        except Exception:
            pass
        finally:
            ctrl.updating_ui = False

        # Sync dependent slider states after update
        self._sync_slider_states()

    def _sv(self, var: tk.StringVar, options: list, enum_val):
        lbl = _label_for(options, enum_val)
        if lbl:
            var.set(lbl)

    def _apply_radar_mode(self, mode):
        is_tx = (str(mode) == str(qb.RadarMode.TRANSMITTING))
        self._btn_tx.configure(relief="sunken" if is_tx     else "raised",
                                bg="#2a5a2a"   if is_tx     else "#1a3a1a")
        self._btn_sb.configure(relief="sunken" if not is_tx else "raised",
                                bg="#5a1a1a"   if not is_tx else "#3a1a1a")

    def _apply_range(self, idx: int):
        if 0 <= idx < len(self._range_labels):
            lbl = self._range_labels[idx]
            if lbl:
                self._last_valid_range_label = lbl
            self._range_var.set(lbl or self._last_valid_range_label)

    def _apply_custom_ranges(self, custom_ranges):
        try:
            if not custom_ranges:
                return
            labels = []
            combo_values = []
            for r in custom_ranges:
                if isinstance(r, (int, float)) and r > 0:
                    nm = r  # already in nautical miles
                    lbl = (f"{nm:.2f} nm" if nm < 1 else
                           f"{nm:.1f} nm" if nm < 10 else
                           f"{nm:.0f} nm")
                    labels.append(lbl)
                    combo_values.append(lbl)
                else:
                    labels.append("")  # preserve index alignment; not shown in dropdown
            if combo_values:
                self._range_labels = labels
                self._range_combo.configure(values=combo_values)
        except Exception:
            pass

    def _sync_slider_states(self):
        gain_manual = (self._gain_mode_var.get() == "Manual")
        sea_manual  = (self._sea_mode_var.get()  == "Manual")
        rain_manual = (self._rain_mode_var.get() == "Manual")
        self._gain_sl.configure(state="normal" if gain_manual else "disabled")
        self._sea_sl.configure( state="normal" if sea_manual  else "disabled")
        self._rain_sl.configure(state="normal" if rain_manual else "disabled")

    # ------------------------------------------------------------------
    # Send setting to radar
    # ------------------------------------------------------------------

    def _send(self, factory_fn, *args):
        if self._ctrl.updating_ui or not self._ctrl.serial:
            return True
        try:
            err = qb.new_setting(factory_fn(self._ctrl.serial, *args))
            if err != qb.ErrorCode.NO_ERROR:
                self.set_status(f"Warning: {factory_fn.__name__} -> {err}")
                return False
            self.set_status("")
            return True
        except Exception as e:
            self.set_status(f"Error sending setting: {e}")
            return False

    def _enum_val(self, options: list, label: str):
        return {l: v for l, v in options}.get(label)

    # ------------------------------------------------------------------
    # Widget callbacks
    # ------------------------------------------------------------------

    def _on_transmit(self):
        self._send(qb.SettingData.radar_mode, qb.RadarMode.TRANSMITTING)

    def _on_standby(self):
        self._send(qb.SettingData.radar_mode, qb.RadarMode.STANDBY)

    def _on_range(self, *_):
        if self._ctrl.updating_ui or not self._ctrl.serial:
            return
        label = self._range_var.get()
        try:
            idx = self._range_labels.index(label)
        except ValueError:
            return
        if not self._send(qb.SettingData.range_index, idx):
            self._ctrl.updating_ui = True
            try:
                self._range_var.set(self._last_valid_range_label)
            finally:
                self._ctrl.updating_ui = False

    def _on_gain_mode(self, *_):
        v = self._enum_val(GAIN_MODES, self._gain_mode_var.get())
        if v is not None:
            self._send(qb.SettingData.gain_mode, v)
            self._sync_slider_states()

    def _on_gain_value(self, *_):
        self._send(qb.SettingData.gain_value, self._gain_value_var.get())

    def _on_sea_mode(self, *_):
        v = self._enum_val(SEA_MODES, self._sea_mode_var.get())
        if v is not None:
            self._send(qb.SettingData.sea_mode, v)
            self._sync_slider_states()

    def _on_sea_value(self, *_):
        self._send(qb.SettingData.sea_value, self._sea_value_var.get())

    def _on_rain_mode(self, *_):
        v = self._enum_val(RAIN_MODES, self._rain_mode_var.get())
        if v is not None:
            self._send(qb.SettingData.rain_mode, v)
            self._sync_slider_states()

    def _on_rain_value(self, *_):
        self._send(qb.SettingData.rain_value, self._rain_value_var.get())

    def _on_ir(self, *_):
        v = self._enum_val(IR_MODES, self._ir_var.get())
        if v is not None:
            self._send(qb.SettingData.interference_rejection, v)

    def _on_doppler(self, *_):
        v = self._enum_val(DOPPLER_MODES, self._doppler_var.get())
        if v is not None:
            self._send(qb.SettingData.doppler_mode, v)

    def _on_txfreq(self, *_):
        v = self._enum_val(TX_FREQS, self._txfreq_var.get())
        if v is not None:
            self._send(qb.SettingData.tx_frequency, v)

    def _on_tgtexp(self, *_):
        v = self._enum_val(TARGET_EXP, self._tgtexp_var.get())
        if v is not None:
            self._send(qb.SettingData.target_expansion, v)

    def _on_seacurve(self, *_):
        v = self._enum_val(SEA_CURVES, self._seacurve_var.get())
        if v is not None:
            self._send(qb.SettingData.sea_clutter_curve, v)

    def _on_mainbang(self, *_):
        v = self._enum_val(MAIN_BANGS, self._mainbang_var.get())
        if v is not None:
            self._send(qb.SettingData.main_bang, v)

    def _on_autoacq(self, *_):
        v = self._enum_val(AUTO_ACQ, self._autoacq_var.get())
        if v is not None:
            self._send(qb.SettingData.auto_acquire_mode, v)

    def _on_preset(self, *_):
        v = self._enum_val(PRESETS, self._preset_var.get())
        if v is not None:
            self._send(qb.SettingData.preset_mode, v)

    def _on_bearing(self, *_):
        self._send(qb.SettingData.bearing_alignment, self._bearing_var.get())

    def _on_nav_data(self):
        try:
            hdg     = float(self._nav_heading_var.get())
            sog_kts = float(self._nav_sog_var.get())
            cog     = float(self._nav_cog_var.get())
        except ValueError:
            self.set_status("Nav data: enter valid numbers (e.g. 270.0)")
            return
        sog_mps = sog_kts * 0.5144
        self._send(qb.SettingData.nav_data,
                   True, hdg,      # heading_valid, heading_degs
                   True, cog,      # cog_valid, cog_degs
                   True, sog_mps)  # sog_valid, sog_mps
        self.set_status(
            f"Nav data sent — Heading: {hdg:.1f}°  SOG: {sog_kts:.1f} kts")

    # ------------------------------------------------------------------
    # Recording
    # ------------------------------------------------------------------

    def _toggle_recording(self):
        if not self._ctrl._recording:
            out_dir = Path(f"data_{datetime.now().strftime('%Y%m%d_%H%M%S')}")
            self._ctrl.start_recording(out_dir)
            self._rec_btn.configure(text="Stop Recording",
                                     bg="#3a1a1a", fg="#ff6666")
            self._rec_dir_var.set(str(out_dir))
        else:
            out = self._ctrl.stop_recording()
            self._rec_btn.configure(text="Start Recording",
                                     bg="#1a2a3a", fg="#4488ff")
            self._rec_dir_var.set(f"Saved: {out.name}")
            self._rec_stats_var.set("")

    # ------------------------------------------------------------------
    # Utilities
    # ------------------------------------------------------------------

    def set_status(self, msg: str):
        self._status_var.set(msg)

    def _on_close(self):
        serial = self._ctrl.serial
        if serial:
            try:
                qb.new_setting(qb.SettingData.radar_mode(serial, qb.RadarMode.STANDBY))
                time.sleep(0.5)
            except Exception:
                pass
        try:
            qb.deregister_notifications(self._ctrl)
            qb.close()
        except Exception:
            pass
        self._ctrl.flush_recording()
        plt.close("all")
        self.root.destroy()


# ---------------------------------------------------------------------------
# main
# ---------------------------------------------------------------------------
def main():
    root = tk.Tk()
    root.title("Quantum Radar Control")
    root.geometry("1200x760")
    root.configure(bg=BG)
    RadarApp(root)
    root.mainloop()


if __name__ == "__main__":
    main()
