"""
data_grabber.py — Capture everything the Quantum radar emits to disk.

Writes to a timestamped directory:
  data_YYYYMMDD_HHMMSS/
    session.json      — radar info + session totals
    events.jsonl      — every non-spoke event (settings, MARPA, alarms, features)
    spokes_NNNN.npz   — compressed spoke batches (one per FLUSH_SCANS rotations)

Each .npz contains arrays:
  bearings            uint16  (N,)       0-2047  (× 360/2048 = degrees)
  timestamps          float64 (N,)       Unix time (seconds)
  instrumented_ranges uint16  (N,)       max range in metres for this spoke
  channels            uint8   (N,)       0=normal, 1=second range
  data_lengths        uint16  (N,)       valid samples per spoke (<= 1024)
  spoke_data          uint8   (N, 1024)  intensity; 254=Doppler receding, 255=approaching

To reconstruct a PPI scan from a .npz file:
    import numpy as np
    d = np.load('spokes_0001.npz')
    scan = np.zeros((2048, 1024), dtype=np.uint8)
    for i in range(len(d['bearings'])):
        b = d['bearings'][i]
        n = d['data_lengths'][i]
        scan[b, :n] = d['spoke_data'][i, :n]

Run from PyQuantumBridge/:
    python examples\\data_grabber.py

Press Ctrl+C to stop.
"""

import sys
import os
import time
import threading
import queue
import json
from datetime import datetime
from pathlib import Path

sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "python"))

import numpy as np
import quantum_bridge as qb

# ---------------------------------------------------------------------------
# User-editable constants
# ---------------------------------------------------------------------------
FLUSH_SCANS      = 10    # write one .npz file every N complete rotations (~25 s)
TRANSMIT_ON_START = True  # automatically command TRANSMITTING when scanner found
WAIT_TIMEOUT_S   = 300   # seconds to wait for scanner before giving up

# ---------------------------------------------------------------------------
# Setting value extractor — maps each Setting enum to a lambda that returns
# a JSON-serialisable value from a SettingData object.
# ---------------------------------------------------------------------------
def _build_extractors():
    return {
        qb.Setting.RADAR_MODE:               lambda sd: str(sd.get_radar_mode()),
        qb.Setting.RANGE:                    lambda sd: sd.get_range_index(),
        qb.Setting.GAIN_MODE:                lambda sd: str(sd.get_gain_mode()),
        qb.Setting.GAIN_VALUE:               lambda sd: sd.get_gain_value(),
        qb.Setting.COLOUR_GAIN_MODE:         lambda sd: str(sd.get_colour_gain_mode()),
        qb.Setting.COLOUR_GAIN_VALUE:        lambda sd: sd.get_colour_gain_value(),
        qb.Setting.SEA_MODE:                 lambda sd: str(sd.get_sea_mode()),
        qb.Setting.SEA_VALUE:                lambda sd: sd.get_sea_value(),
        qb.Setting.RAIN_MODE:                lambda sd: str(sd.get_rain_mode()),
        qb.Setting.RAIN_VALUE:               lambda sd: sd.get_rain_value(),
        qb.Setting.INTERFERENCE_REJECTION:   lambda sd: str(sd.get_interference_rejection()),
        qb.Setting.TX_FREQUENCY:             lambda sd: str(sd.get_tx_frequency()),
        qb.Setting.BEARING_ALIGNMENT:        lambda sd: sd.get_bearing_alignment(),
        qb.Setting.PRESET_MODE:              lambda sd: str(sd.get_preset_mode()),
        qb.Setting.TARGET_EXPANSION:         lambda sd: str(sd.get_target_expansion()),
        qb.Setting.SEA_CLUTTER_CURVE:        lambda sd: str(sd.get_sea_clutter_curve()),
        qb.Setting.MAIN_BANG:                lambda sd: str(sd.get_main_bang()),
        qb.Setting.DOPPLER_MODE:             lambda sd: str(sd.get_doppler_mode()),
        qb.Setting.DOPPLER_ACTIVE:           lambda sd: str(sd.get_doppler_active()),
        qb.Setting.AUTO_ACQUIRE_MODE:        lambda sd: str(sd.get_auto_acquire_mode()),
        qb.Setting.GUARD_ZONE_SENSITIVITY:   lambda sd: sd.get_guard_zone_sensitivity(),
        qb.Setting.GUARD_ZONE_1:             lambda sd: sd.get_zone(),
        qb.Setting.GUARD_ZONE_2:             lambda sd: sd.get_zone(),
        qb.Setting.GUARD_ZONE_ENABLE:        lambda sd: sd.get_zone_state(),
        qb.Setting.TIMED_TRANSMIT:           lambda sd: sd.get_timed_transmit(),
        qb.Setting.TIMED_TRANSMIT_REMAINING: lambda sd: sd.get_timed_transmit_remaining(),
        qb.Setting.NAV_DATA:                 lambda sd: sd.get_nav_data(),
        qb.Setting.MARPA_SETUP:              lambda sd: sd.get_marpa_setup(),
        qb.Setting.CUSTOM_RANGES:            lambda sd: sd.get_custom_ranges(),
        qb.Setting.MAIN_SOFTWARE_VERSION:    lambda sd: sd.get_software_version(),
        qb.Setting.PSU_SOFTWARE_VERSION:     lambda sd: sd.get_software_version(),
        qb.Setting.FPGA_SOFTWARE_VERSION:    lambda sd: sd.get_software_version(),
        qb.Setting.WIFI_SOFTWARE_VERSION:    lambda sd: sd.get_software_version(),
        qb.Setting.W3_SOFTWARE_VERSION:      lambda sd: sd.get_software_version(),
    }


# ---------------------------------------------------------------------------
# DataGrabber
# ---------------------------------------------------------------------------
class DataGrabber(qb.INotify):
    def __init__(self, out_dir: Path):
        super().__init__()
        out_dir.mkdir(parents=True, exist_ok=True)
        self._out_dir = out_dir

        self.serial = None
        self.scanner_ready = threading.Event()

        # Spoke buffers — swapped atomically under _lock
        self._lock = threading.Lock()
        self._buf_bearings:   list = []
        self._buf_timestamps: list = []
        self._buf_ranges:     list = []
        self._buf_channels:   list = []
        self._buf_lengths:    list = []
        self._buf_data:       list = []   # list of uint8 numpy arrays (len <= 1024)
        self._last_bearing = -1
        self._scan_count   = 0
        self._total_spokes = 0
        self._file_index   = 0

        # Background writer thread
        self._write_queue: queue.Queue = queue.Queue()
        self._writer = threading.Thread(target=self._writer_thread, daemon=True)
        self._writer.start()

        # Event log (line-buffered so data lands on disk immediately)
        self._events_path = out_dir / "events.jsonl"
        self._events_file = open(self._events_path, "w", buffering=1, encoding="utf-8")
        self._events_lock = threading.Lock()

        # Session metadata
        self._session: dict = {
            "start_time": datetime.now().isoformat(),
            "flush_scans": FLUSH_SCANS,
        }

        self._extractors = _build_extractors()

    # ------------------------------------------------------------------
    # INotify callbacks
    # ------------------------------------------------------------------

    def scanner_list_changed(self, count):
        if count > 0:
            details = qb.get_scanner_details(0)
            self.serial = details.serial_number
            self._session["serial_number"] = f"0x{self.serial:08X}"
            self._session["description"]   = details.description
            self._log("scanner_found",
                      serial=f"0x{self.serial:08X}",
                      description=details.description)
            self.scanner_ready.set()
        else:
            self._log("scanner_lost")
            self.serial = None
            self.scanner_ready.clear()

    def spoke_data_received(self, serial, spoke):
        ts      = time.time()
        bearing = spoke.bearing
        data    = spoke.to_numpy().copy()   # copy: numpy view would alias freed C++ memory

        to_flush = None
        with self._lock:
            self._buf_bearings.append(bearing)
            self._buf_timestamps.append(ts)
            self._buf_ranges.append(spoke.instrumented_range)
            self._buf_channels.append(spoke.channel)
            self._buf_lengths.append(spoke.data_length)
            self._buf_data.append(data)
            self._total_spokes += 1

            if bearing < self._last_bearing:          # wrap-around = new scan
                self._scan_count += 1
                if self._scan_count % FLUSH_SCANS == 0:
                    to_flush = self._swap_buffers()
            self._last_bearing = bearing

        if to_flush is not None:
            self._write_queue.put(to_flush)

    def setting_changed(self, sd):
        setting = sd.get_setting()
        extractor = self._extractors.get(setting)
        if extractor is None:
            value = None
        else:
            try:
                value = extractor(sd)
            except Exception:
                value = None
        self._log("setting",
                  setting=str(setting),
                  value=value,
                  channel=str(sd.get_channel()))

    def feature_changed(self, serial, feature, supported):
        self._log("feature", feature=str(feature), supported=supported)

    def parameter_changed(self, serial, parameter, value):
        self._log("parameter", parameter=str(parameter), value=value)

    def marpa_data_changed(self, serial, data):
        self._log("marpa",
                  id=data.id,
                  valid=data.valid,
                  auto_acquired=data.auto_acquired,
                  state=str(data.state),
                  range_m=data.target_range_m,
                  bearing_degs=data.target_bearing_degs,
                  true_bearing_degs=data.target_true_bearing_degs,
                  rel_speed_mps=data.rel_target_speed_mps,
                  rel_course_degs=data.rel_target_course_degs,
                  true_speed_mps=data.true_target_speed_mps,
                  true_course_degs=data.true_target_course_degs,
                  cpa_m=data.closest_point_of_approach_m,
                  tcpa_secs=data.time_to_closest_point_secs,
                  heading_at_update_degs=data.heading_at_last_update_degs,
                  going_towards_cpa=data.target_going_towards_closest,
                  true_data_valid=data.target_true_data_valid)

    def alarm_data_changed(self, serial, alarm):
        self._log("alarm",
                  id=alarm.id,
                  alarm_type=str(alarm.type),
                  value=alarm.value)

    # ------------------------------------------------------------------
    # Internal helpers
    # ------------------------------------------------------------------

    def _log(self, event_type, **kwargs):
        entry = {"timestamp": datetime.now().isoformat(), "type": event_type, **kwargs}
        line  = json.dumps(entry, default=str) + "\n"
        with self._events_lock:
            self._events_file.write(line)

    def _swap_buffers(self):
        """Swap spoke buffers and return the old set. Called with _lock held."""
        batch = (
            self._buf_bearings,
            self._buf_timestamps,
            self._buf_ranges,
            self._buf_channels,
            self._buf_lengths,
            self._buf_data,
        )
        self._buf_bearings   = []
        self._buf_timestamps = []
        self._buf_ranges     = []
        self._buf_channels   = []
        self._buf_lengths    = []
        self._buf_data       = []
        return batch

    def _writer_thread(self):
        """Background thread: pops batches from queue and saves .npz files."""
        while True:
            batch = self._write_queue.get()
            if batch is None:           # sentinel — time to stop
                break
            self._save_batch(batch)

    def _save_batch(self, batch):
        bearings, timestamps, ranges, channels, lengths, data_list = batch
        if not bearings:
            return

        n = len(bearings)
        spoke_arr = np.zeros((n, 1024), dtype=np.uint8)
        for i, d in enumerate(data_list):
            spoke_arr[i, :len(d)] = d

        self._file_index += 1
        path = self._out_dir / f"spokes_{self._file_index:04d}.npz"
        np.savez_compressed(
            path,
            bearings            = np.array(bearings,    dtype=np.uint16),
            timestamps          = np.array(timestamps,  dtype=np.float64),
            instrumented_ranges = np.array(ranges,      dtype=np.uint16),
            channels            = np.array(channels,    dtype=np.uint8),
            data_lengths        = np.array(lengths,     dtype=np.uint16),
            spoke_data          = spoke_arr,
        )
        print(f"\r  Saved {path.name}  ({n} spokes)                    ")

    def flush_and_close(self):
        """Flush remaining spoke buffer, stop writer thread, write session.json."""
        with self._lock:
            remaining = self._swap_buffers()
            total_scans  = self._scan_count
            total_spokes = self._total_spokes

        if remaining[0]:                        # any leftover spokes?
            self._write_queue.put(remaining)

        self._write_queue.put(None)             # signal writer to stop
        self._writer.join()

        self._session["end_time"]     = datetime.now().isoformat()
        self._session["total_scans"]  = total_scans
        self._session["total_spokes"] = total_spokes
        self._session["spoke_files"]  = self._file_index

        session_path = self._out_dir / "session.json"
        with open(session_path, "w", encoding="utf-8") as f:
            json.dump(self._session, f, indent=2)

        self._events_file.close()

        print(f"\nOutput written to: {self._out_dir}")
        print(f"  session.json   — radar info + totals")
        print(f"  events.jsonl   — {sum(1 for _ in open(self._events_path))} events logged")
        print(f"  spokes_*.npz   — {self._file_index} file(s), {total_scans} scans, {total_spokes} spokes")

    # ------------------------------------------------------------------
    # Live stats (called from main loop)
    # ------------------------------------------------------------------
    def stats(self):
        with self._lock:
            return self._scan_count, self._total_spokes, self._file_index


# ---------------------------------------------------------------------------
# main
# ---------------------------------------------------------------------------
def main():
    out_dir = Path(f"data_{datetime.now().strftime('%Y%m%d_%H%M%S')}")
    grabber = DataGrabber(out_dir)

    print(f"Output directory : {out_dir}")
    print(f"Spoke flush every: {FLUSH_SCANS} complete scans")
    print("Opening Quantum library…")

    err = qb.open()
    if err != qb.ErrorCode.NO_ERROR:
        print(f"open() failed: {err}")
        sys.exit(1)

    qb.register_notifications(grabber)

    print(f"Waiting for scanner (up to {WAIT_TIMEOUT_S} s)…")
    if not grabber.scanner_ready.wait(timeout=WAIT_TIMEOUT_S):
        print("No scanner found. Check network / DHCP config.")
        qb.deregister_notifications(grabber)
        qb.close()
        grabber.flush_and_close()
        sys.exit(1)

    serial = grabber.serial

    if TRANSMIT_ON_START:
        print("Setting radar to TRANSMIT…")
        err = qb.new_setting(qb.SettingData.radar_mode(serial, qb.RadarMode.TRANSMITTING))
        if err != qb.ErrorCode.NO_ERROR:
            print(f"  Warning: new_setting(TRANSMIT) returned {err}")

    print("Capturing — press Ctrl+C to stop.\n")

    try:
        while True:
            time.sleep(1)
            scans, spokes, files = grabber.stats()
            print(
                f"\r  Scans: {scans:6d}  |  Spokes: {spokes:9d}  |  Files: {files:4d}",
                end="", flush=True,
            )
    except KeyboardInterrupt:
        print("\nStopping…")
    finally:
        if serial is not None:
            qb.new_setting(qb.SettingData.radar_mode(serial, qb.RadarMode.STANDBY))
            time.sleep(1)
        qb.deregister_notifications(grabber)
        qb.close()
        grabber.flush_and_close()


if __name__ == "__main__":
    main()
