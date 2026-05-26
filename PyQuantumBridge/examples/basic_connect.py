"""
basic_connect.py — Connect to the radar, transmit for 10 seconds, print spoke info.

Run from PyQuantumBridge/:
    python examples/basic_connect.py
"""
import sys
import os
import time
import threading

# Add the PyQuantumBridge directory so 'quantum' and 'quantum_bridge' are importable
sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "python"))

import quantum_bridge as qb

TRANSMIT_SECONDS = 10


class BasicClient(qb.INotify):
    def __init__(self):
        super().__init__()
        self.serial = None
        self.spoke_count = 0
        self.scanner_ready = threading.Event()

    def scanner_list_changed(self, count):
        print(f"[scanner_list_changed] count={count}")
        if count > 0:
            details = qb.get_scanner_details(0)
            self.serial = details.serial_number
            print(f"  Scanner: {details.description}  serial=0x{self.serial:08X}")
            self.scanner_ready.set()
        else:
            self.serial = None
            self.scanner_ready.clear()

    def spoke_data_received(self, serial, spoke):
        self.spoke_count += 1
        if self.spoke_count % 2048 == 1:  # print once per full scan
            bearing_deg = spoke.bearing * 360 / qb.SPOKES_PER_SCAN
            print(f"  Spoke bearing={bearing_deg:.1f}°  "
                  f"data_length={spoke.data_length}  "
                  f"channel={spoke.channel}  "
                  f"instrumented_range={spoke.instrumented_range}")

    def setting_changed(self, sd):
        setting = sd.get_setting()
        if setting == qb.Setting.RADAR_MODE:
            mode = sd.get_radar_mode()
            print(f"[setting_changed] RadarMode -> {mode}")
        elif setting == qb.Setting.RANGE:
            idx = sd.get_range_index()
            print(f"[setting_changed] Range index -> {idx}")
        elif setting in (qb.Setting.MAIN_SOFTWARE_VERSION,
                         qb.Setting.PSU_SOFTWARE_VERSION,
                         qb.Setting.FPGA_SOFTWARE_VERSION):
            ver = sd.get_software_version()
            if ver is not None:
                print(f"[setting_changed] {setting} -> v{ver // 100}.{ver % 100:02d}")

    def feature_changed(self, serial, feature, supported):
        print(f"[feature_changed] {feature}  supported={supported}")

    def parameter_changed(self, serial, parameter, value):
        print(f"[parameter_changed] {parameter}  value={value}")


def main():
    client = BasicClient()

    print("Opening Quantum library…")
    err = qb.open()
    if err != qb.ErrorCode.NO_ERROR:
        print(f"open() failed: {err}")
        sys.exit(1)

    print("Registering notifications…")
    err = qb.register_notifications(client)
    if err != qb.ErrorCode.NO_ERROR:
        print(f"register_notifications() failed: {err}")
        qb.close()
        sys.exit(1)

    print("Waiting for scanner (up to 300 s)…")
    if not client.scanner_ready.wait(timeout=300):
        print("No scanner found within 300 seconds. Check network / DHCP config.")
        qb.deregister_notifications(client)
        qb.close()
        sys.exit(1)

    serial = client.serial
    print(f"Setting radar to TRANSMIT for {TRANSMIT_SECONDS} seconds…")
    cmd = qb.SettingData.radar_mode(serial, qb.RadarMode.TRANSMITTING)
    err = qb.new_setting(cmd)
    if err != qb.ErrorCode.NO_ERROR:
        print(f"new_setting(TRANSMIT) failed: {err}")

    time.sleep(TRANSMIT_SECONDS)
    print(f"Total spokes received: {client.spoke_count}")

    print("Setting radar to STANDBY…")
    cmd = qb.SettingData.radar_mode(serial, qb.RadarMode.STANDBY)
    qb.new_setting(cmd)

    time.sleep(1)
    qb.deregister_notifications(client)
    qb.close()
    print("Done.")


if __name__ == "__main__":
    main()
