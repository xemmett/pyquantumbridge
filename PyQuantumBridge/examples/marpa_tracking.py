"""
marpa_tracking.py — Acquire and display MARPA target data in a live table.

MARPA requires:
  1. Radar in TRANSMIT mode.
  2. Own-ship heading sent via SettingData.nav_data() for relative tracking.
     SOG and COG also needed for true vectors.
  3. Targets designated via SettingData.marpa_designate(serial, range_m, bearing_degs).

Run from PyQuantumBridge/:
    python examples/marpa_tracking.py

Press Ctrl+C to stop.
"""
import sys
import os
import time
import threading

sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "python"))

import quantum_bridge as qb

# Edit these to test target designation
TARGET_RANGE_M    = 500.0
TARGET_BEARING_DEG = 90.0
OWN_HEADING_DEG   = 0.0    # own vessel heading (degrees true north)
OWN_SOG_MPS       = 0.0    # own vessel speed over ground (m/s)
OWN_COG_DEG       = 0.0    # own vessel course over ground (degrees)

_STATE_NAMES = {
    qb.TargetState.ACQUIRING: "ACQUIRING",
    qb.TargetState.SAFE:      "SAFE",
    qb.TargetState.DANGEROUS: "DANGEROUS",
    qb.TargetState.LOST:      "LOST",
}


class MarpaClient(qb.INotify):
    def __init__(self):
        super().__init__()
        self.serial = None
        self.targets: dict[int, qb.MarpaData] = {}
        self._lock = threading.Lock()
        self.scanner_ready = threading.Event()

    def scanner_list_changed(self, count):
        if count > 0:
            details = qb.get_scanner_details(0)
            self.serial = details.serial_number
            print(f"Scanner: {details.description}  0x{self.serial:08X}")
            self.scanner_ready.set()

            # Start transmitting
            qb.new_setting(qb.SettingData.radar_mode(self.serial, qb.RadarMode.TRANSMITTING))
            time.sleep(2)

            # Send own-ship nav data (required for MARPA)
            nav = qb.SettingData.nav_data(
                self.serial,
                True, OWN_HEADING_DEG,
                True, OWN_COG_DEG,
                True, OWN_SOG_MPS,
            )
            qb.new_setting(nav)

            # Designate a test target
            print(f"Designating target at {TARGET_RANGE_M} m, bearing {TARGET_BEARING_DEG}°…")
            des = qb.SettingData.marpa_designate(self.serial, TARGET_RANGE_M, TARGET_BEARING_DEG)
            qb.new_setting(des)
        else:
            self.serial = None
            self.scanner_ready.clear()

    def marpa_data_changed(self, serial, data):
        with self._lock:
            if data.valid:
                self.targets[data.id] = data
            elif data.id in self.targets:
                del self.targets[data.id]

    def alarm_data_changed(self, serial, alarm):
        names = {
            qb.AlarmType.GUARD_ZONE_TARGET_ENTERING: "GUARD ZONE BREACH",
            qb.AlarmType.MARPA_DANGEROUS_TARGET:     "DANGEROUS TARGET",
            qb.AlarmType.MARPA_LOST_TARGET:          "MARPA TARGET LOST",
            qb.AlarmType.AUTO_ACQUIRE_LIST_FULL:     "AUTO-ACQUIRE LIST FULL",
        }
        print(f"\n*** ALARM: {names.get(alarm.type, str(alarm.type))} "
              f"id={alarm.id} value={alarm.value} ***\n")

    def setting_changed(self, sd):
        if sd.get_setting() == qb.Setting.RADAR_MODE:
            print(f"RadarMode -> {sd.get_radar_mode()}")

    def print_table(self):
        with self._lock:
            targets = dict(self.targets)

        if not targets:
            print("  (no targets)")
            return

        print(f"  {'ID':>3}  {'State':>10}  {'Range(m)':>9}  {'Bearing(°)':>10}  "
              f"{'Rel Spd(m/s)':>13}  {'CPA(m)':>7}  {'TCPA(s)':>7}")
        print("  " + "-" * 72)
        for t in sorted(targets.values(), key=lambda x: x.id):
            state = _STATE_NAMES.get(t.state, str(t.state))
            print(f"  {t.id:>3}  {state:>10}  {t.target_range_m:>9.1f}  "
                  f"{t.target_bearing_degs:>10.1f}  {t.rel_target_speed_mps:>13.2f}  "
                  f"{t.closest_point_of_approach_m:>7.1f}  {t.time_to_closest_point_secs:>7}")


def main():
    client = MarpaClient()

    err = qb.open()
    if err != qb.ErrorCode.NO_ERROR:
        print(f"open() failed: {err}"); sys.exit(1)
    qb.register_notifications(client)

    print("Waiting for scanner…")
    if not client.scanner_ready.wait(timeout=15):
        print("No scanner found."); qb.deregister_notifications(client); qb.close(); sys.exit(1)

    print("Tracking targets — Ctrl+C to stop.\n")
    try:
        while True:
            print(f"\r--- MARPA Targets @ {time.strftime('%H:%M:%S')} ---")
            client.print_table()
            time.sleep(2)
    except KeyboardInterrupt:
        pass
    finally:
        print("\nStopping…")
        if client.serial is not None:
            qb.new_setting(qb.SettingData.radar_mode(client.serial, qb.RadarMode.STANDBY))
            time.sleep(1)
        qb.deregister_notifications(client)
        qb.close()
        print("Done.")


if __name__ == "__main__":
    main()
