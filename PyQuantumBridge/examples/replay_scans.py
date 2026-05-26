"""
replay_scans.py — Browse captured Quantum radar data offline.

Loads a data directory produced by data_grabber.py and shows a
scan-by-scan PPI viewer with time-based navigation.

Usage:
    python examples\\replay_scans.py data_20260522_165316
    python examples\\replay_scans.py          # auto-selects most-recent data_* dir

Navigation:
    <- / ->   Previous / Next scan
    Slider    Scrub through the session timeline
    Text box  Type a time then press Enter or click Go
              Accepted formats: 16:37  16:37:00  4:37pm  4:37:00pm

Requires: numpy, matplotlib, scipy
Does NOT require a live radar or quantum_bridge.
"""

import sys
import os
import json
import bisect
from dataclasses import dataclass
from datetime import datetime, date
from pathlib import Path
from typing import Optional

import numpy as np
import matplotlib.pyplot as plt
import matplotlib.colors as mcolors
import matplotlib.gridspec as gridspec
from matplotlib.widgets import Button, Slider, TextBox
from scipy.ndimage import map_coordinates

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------
CART_SIZE         = 512
SPOKES_PER_SCAN   = 2048
SAMPLES_PER_SPOKE = 1024

# ---------------------------------------------------------------------------
# Radar colour map (identical to radar_image.py)
# ---------------------------------------------------------------------------
_cm = np.zeros((256, 4))
_cm[:, 3] = 1.0
_grey = np.linspace(0, 1, 253)
_cm[1:254, :3] = _grey[:, None]
_cm[254] = [0.0, 1.0, 0.0, 1.0]   # Doppler receding  → green
_cm[255] = [1.0, 0.2, 0.2, 1.0]   # Doppler approaching → red
RADAR_CMAP = mcolors.ListedColormap(_cm)

# ---------------------------------------------------------------------------
# Polar -> Cartesian (copied from python/quantum/scanner.py:to_cartesian)
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
# Data model
# ---------------------------------------------------------------------------
@dataclass
class ScanRecord:
    file:     Path
    start:    int    # first spoke index within the npz file
    end:      int    # one-past-last spoke index
    ts_start: float  # Unix timestamp of the first spoke

# ---------------------------------------------------------------------------
# ScanIndex — lightweight index over a captured data directory
# ---------------------------------------------------------------------------
class ScanIndex:
    def __init__(self, data_dir: Path):
        self._dir = data_dir

        with open(data_dir / "session.json", encoding="utf-8") as f:
            self.session: dict = json.load(f)

        events_path = data_dir / "events.jsonl"
        self.events: list[dict] = []
        if events_path.exists():
            with open(events_path, encoding="utf-8") as f:
                for line in f:
                    line = line.strip()
                    if line:
                        self.events.append(json.loads(line))

        self._event_ts: list[float] = [
            datetime.fromisoformat(e["timestamp"]).timestamp()
            for e in self.events
        ]

        self._scans: list[ScanRecord] = []
        for npz_path in sorted(data_dir.glob("spokes_*.npz")):
            print(f"  Indexing {npz_path.name}…", end=" ", flush=True)
            d  = np.load(npz_path)
            bs = d["bearings"]
            ts = d["timestamps"]
            n  = len(bs)
            start = 0
            for i in range(1, n):
                if bs[i] < bs[i - 1]:
                    self._scans.append(ScanRecord(npz_path, start, i, float(ts[start])))
                    start = i
            if start < n:
                self._scans.append(ScanRecord(npz_path, start, n, float(ts[start])))
            print(f"{len(self._scans)} scans so far")

        self._ts_list: list[float] = [r.ts_start for r in self._scans]
        self._session_date: date   = datetime.fromisoformat(self.session["start_time"]).date()

    def __len__(self) -> int:
        return len(self._scans)

    def get_scan_image(self, idx: int) -> tuple:
        """Return (polar_scan, instrumented_range_metres).

        instrumented_range_metres is 0 if the NPZ predates range recording.
        """
        rec  = self._scans[idx]
        d    = np.load(rec.file)
        sl   = slice(rec.start, rec.end)
        bs   = d["bearings"][sl]
        ls   = d["data_lengths"][sl]
        sd   = d["spoke_data"][sl]
        irs  = d["instrumented_ranges"][sl] if "instrumented_ranges" in d else None
        scan = np.zeros((SPOKES_PER_SCAN, SAMPLES_PER_SPOKE), dtype=np.uint8)
        for i in range(len(bs)):
            n = int(ls[i])
            scan[int(bs[i]), :n] = sd[i, :n]
        ir = int(np.median(irs)) if irs is not None and len(irs) > 0 else 0
        return scan, ir

    def get_scan_ts(self, idx: int) -> float:
        return self._scans[idx].ts_start

    def find_scan_by_time(self, time_str: str) -> int:
        ts = self._parse_time(time_str.strip())
        if ts is None:
            print(f"  Could not parse time: {time_str!r}", flush=True)
            return 0
        pos = bisect.bisect_left(self._ts_list, ts)
        if pos == 0:
            return 0
        if pos >= len(self._scans):
            return len(self._scans) - 1
        d_before = abs(self._ts_list[pos - 1] - ts)
        d_after  = abs(self._ts_list[pos]     - ts)
        return (pos - 1) if d_before <= d_after else pos

    def events_up_to(self, ts: float, n: int = 15) -> list[dict]:
        pos = bisect.bisect_right(self._event_ts, ts)
        return self.events[max(0, pos - n): pos]

    def _parse_time(self, s: str) -> Optional[float]:
        d = self._session_date
        for fmt in ("%I:%M%p", "%I:%M:%S%p", "%I:%M %p", "%I:%M:%S %p"):
            try:
                t = datetime.strptime(s.upper(), fmt.upper())
                return datetime(d.year, d.month, d.day, t.hour, t.minute, t.second).timestamp()
            except ValueError:
                pass
        for fmt in ("%H:%M", "%H:%M:%S"):
            try:
                t = datetime.strptime(s, fmt)
                return datetime(d.year, d.month, d.day, t.hour, t.minute, t.second).timestamp()
            except ValueError:
                pass
        return None

# ---------------------------------------------------------------------------
# ReplayViewer — matplotlib GUI
# ---------------------------------------------------------------------------
class ReplayViewer:
    def __init__(self, index: ScanIndex):
        self._index          = index
        self._idx            = 0
        self._n              = len(index)
        self._slider_lock    = False   # prevent feedback loop between slider and _goto

        # ---- Figure --------------------------------------------------------
        fig = plt.figure(figsize=(14, 8.5))
        fig.patch.set_facecolor("black")
        self._fig = fig

        # Top area: PPI (left) + info panel (right)
        # Bottom strip: slider + buttons
        gs_top = gridspec.GridSpec(
            1, 2,
            figure=fig,
            left=0.03, right=0.97,
            top=0.95, bottom=0.22,
            wspace=0.08,
        )

        # ---- PPI axes -------------------------------------------------------
        ax_ppi = fig.add_subplot(gs_top[0, 0])
        ax_ppi.set_facecolor("black")
        ax_ppi.set_xticks([]); ax_ppi.set_yticks([])
        for sp in ax_ppi.spines.values():
            sp.set_edgecolor("#444444")
        ax_ppi.set_title("Quantum Radar — PPI", color="white", fontsize=10, pad=4)

        blank = np.zeros((CART_SIZE, CART_SIZE), dtype=np.uint8)
        self._im = ax_ppi.imshow(
            blank, cmap=RADAR_CMAP, vmin=0, vmax=255,
            origin="upper", aspect="equal", interpolation="nearest",
        )
        self._ax_ppi        = ax_ppi
        self._vrm_angles = np.linspace(0, 2 * np.pi, 129)
        self._vrm = 1024 * 2 / 3
        (self._vrm_line,) = ax_ppi.plot([], [], color='yellow', linewidth=1.0, alpha=0.85)

        # ---- Info / events panel --------------------------------------------
        ax_info = fig.add_subplot(gs_top[0, 1])
        ax_info.set_facecolor("black")
        ax_info.set_xticks([]); ax_info.set_yticks([])
        for sp in ax_info.spines.values():
            sp.set_visible(False)
        self._info_text = ax_info.text(
            0.02, 0.99, "",
            transform=ax_info.transAxes,
            color="white", fontsize=7.5, va="top", ha="left",
            fontfamily="monospace",
        )

        # ---- Slider ---------------------------------------------------------
        ax_slider = fig.add_axes([0.08, 0.13, 0.84, 0.04])
        ax_slider.set_facecolor("#111111")
        self._slider = Slider(
            ax_slider, "Scan",
            0, max(self._n - 1, 1),
            valinit=0, valstep=1, color="#336699",
        )
        self._slider.label.set_color("white")
        self._slider.valtext.set_color("white")
        self._slider.on_changed(self._on_slider)

        # ---- Navigation buttons + time box ----------------------------------
        btn_h  = 0.06
        btn_y  = 0.04
        ax_prev = fig.add_axes([0.08,  btn_y, 0.11, btn_h])
        ax_next = fig.add_axes([0.20,  btn_y, 0.11, btn_h])
        ax_tb   = fig.add_axes([0.60,  btn_y, 0.22, btn_h])
        ax_go   = fig.add_axes([0.83,  btn_y, 0.09, btn_h])

        self._btn_prev = Button(ax_prev, "← Prev", color="#1a1a1a", hovercolor="#333333")
        self._btn_next = Button(ax_next, "Next →", color="#1a1a1a", hovercolor="#333333")
        self._btn_go   = Button(ax_go,   "Go",          color="#1a2a3a", hovercolor="#336699")

        for btn in (self._btn_prev, self._btn_next, self._btn_go):
            btn.label.set_color("white")

        # Label for the text box
        fig.text(0.60, btn_y + btn_h + 0.005, "Jump to time (e.g. 16:37 or 4:37pm):",
                 color="#888888", fontsize=7.5, va="bottom")

        self._textbox = TextBox(ax_tb, "", initial="HH:MM:SS",
                                color="#111111", hovercolor="#1a1a1a")
        self._textbox.label.set_color("white")
        self._textbox.text_disp.set_color("white")

        # ---- Wire up callbacks ---------------------------------------------
        self._btn_prev.on_clicked(self._on_prev)
        self._btn_next.on_clicked(self._on_next)
        self._btn_go.on_clicked(self._on_go_click)
        self._textbox.on_submit(self._on_go_submit)
        fig.canvas.mpl_connect("key_press_event", self._on_key)

        # ---- Initial render -------------------------------------------------
        self._goto(0)

    # ------------------------------------------------------------------
    # Navigation callbacks
    # ------------------------------------------------------------------

    def _on_prev(self, _event):
        self._goto(self._idx - 1)

    def _on_next(self, _event):
        self._goto(self._idx + 1)

    def _on_slider(self, val):
        if not self._slider_lock:
            self._goto(int(round(val)), update_slider=False)

    def _on_go_click(self, _event):
        self._jump_to_text()

    def _on_go_submit(self, text):
        self._jump_to_text()

    def _on_key(self, event):
        if event.key == "left":
            self._goto(self._idx - 1)
        elif event.key == "right":
            self._goto(self._idx + 1)

    def _jump_to_text(self):
        text = self._textbox.text.strip()
        if text and text != "HH:MM:SS":
            target = self._index.find_scan_by_time(text)
            self._goto(target)

    # ------------------------------------------------------------------
    # Core render
    # ------------------------------------------------------------------

    def _compute_vrm_xy(self):
        half = CART_SIZE / 2.0
        dr = (self._vrm / 1024.0) * half
        xs = dr * np.sin(self._vrm_angles) + half
        ys = half - dr * np.cos(self._vrm_angles)
        return xs, ys

    def _goto(self, idx: int, update_slider: bool = True):
        if self._n == 0:
            return
        idx = max(0, min(idx, self._n - 1))
        self._idx = idx

        # Load and convert scan
        polar, ir = self._index.get_scan_image(idx)
        cart      = polar_to_cartesian(polar)
        self._im.set_data(cart)
        xs, ys = self._compute_vrm_xy()
        self._vrm_line.set_data(xs, ys)

        # Update slider without triggering its callback
        if update_slider:
            self._slider_lock = True
            self._slider.set_val(idx)
            self._slider_lock = False

        # Build info text
        ts      = self._index.get_scan_ts(idx)
        dt_str  = datetime.fromtimestamp(ts).strftime("%Y-%m-%d  %H:%M:%S")
        session = self._index.session
        events  = self._index.events_up_to(ts)

        lines = [
            f"SESSION",
            f"  {session.get('description', 'Unknown')}",
            f"  Serial: {session.get('serial_number', '?')}",
            f"  Recorded: {session.get('start_time', '?')[:19]}",
            f"  Duration: {self._session_duration(session)}",
            f"  Total scans: {session.get('total_scans', '?')}",
            f"",
            f"CURRENT SCAN",
            f"  Index:  {idx + 1} / {self._n}",
            f"  Time:   {dt_str}",
            f"",
            f"EVENTS (up to this scan)",
        ]

        if events:
            for ev in reversed(events):
                ev_dt = datetime.fromisoformat(ev["timestamp"]).strftime("%H:%M:%S")
                ev_type = ev.get("type", "?")
                detail = self._format_event(ev)
                lines.append(f"  {ev_dt}  {ev_type:<12} {detail}")
        else:
            lines.append("  (none)")

        self._info_text.set_text("\n".join(lines))
        self._fig.canvas.draw_idle()

    # ------------------------------------------------------------------
    # Helpers
    # ------------------------------------------------------------------

    @staticmethod
    def _session_duration(session: dict) -> str:
        try:
            t0 = datetime.fromisoformat(session["start_time"])
            t1 = datetime.fromisoformat(session["end_time"])
            secs = int((t1 - t0).total_seconds())
            return f"{secs // 60}m {secs % 60}s"
        except (KeyError, ValueError):
            return "?"

    @staticmethod
    def _format_event(ev: dict) -> str:
        t = ev.get("type", "")
        if t == "setting":
            name = ev.get("setting", "").replace("Setting.", "")
            val  = ev.get("value", "")
            return f"{name} = {val}"
        if t == "feature":
            return f"{ev.get('feature', '').replace('Feature.', '')} supported={ev.get('supported')}"
        if t == "parameter":
            return f"{ev.get('parameter', '').replace('Parameter.', '')} = {ev.get('value')}"
        if t == "alarm":
            return f"id={ev.get('id')} type={ev.get('alarm_type', '').replace('AlarmType.', '')} val={ev.get('value')}"
        if t == "marpa":
            return f"id={ev.get('id')} state={ev.get('state', '').replace('TargetState.', '')}"
        if t == "scanner_found":
            return ev.get("description", "")
        return ""

    def show(self):
        plt.show()


# ---------------------------------------------------------------------------
# main
# ---------------------------------------------------------------------------
def main():
    if len(sys.argv) > 1:
        data_dir = Path(sys.argv[1])
        if not data_dir.exists():
            sys.exit(f"Directory not found: {data_dir}")
    else:
        candidates = sorted(Path(".").glob("data_????????_??????"))
        if not candidates:
            sys.exit("No data_* directory found. Pass a directory as argument or run from PyQuantumBridge/.")
        data_dir = candidates[-1]
        print(f"Auto-selected: {data_dir}")

    print(f"Loading {data_dir}…")
    index = ScanIndex(data_dir)
    if len(index) == 0:
        sys.exit("No complete scans found in data directory.")

    print(f"\nLoaded {len(index)} scans.")
    print("Controls: <- / -> keys, slider, or type a time and click Go.\n")

    viewer = ReplayViewer(index)
    viewer.show()


if __name__ == "__main__":
    main()
