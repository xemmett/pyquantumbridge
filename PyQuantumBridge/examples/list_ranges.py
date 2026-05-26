"""
list_ranges.py — Connect to the radar and print its available range steps.

Run from PyQuantumBridge/:
    python examples/list_ranges.py

No transmit needed — the radar sends range information from standby.
"""
import sys
import os
import threading

sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "python"))

import quantum_bridge as qb

NM_TO_M = 1852.0


class RangeClient(qb.INotify):
    """
    We subclass qb.INotify to receive callbacks from the radar.
    The SDK calls our methods automatically — we never call them ourselves.
    """

    def __init__(self):
        super().__init__()
        # Threading event so main() can block until the radar is found.
        self.scanner_ready = threading.Event()
        # Threading event so main() can block until ranges arrive.
        self.ranges_received = threading.Event()

    # ------------------------------------------------------------------
    # Called by the SDK whenever the list of visible scanners changes.
    # We use it to know when a radar has been found on the network.
    # ------------------------------------------------------------------
    def scanner_list_changed(self, count):
        if count > 0:
            details = qb.get_scanner_details(0)
            print(f"Radar found: {details.description}  serial=0x{details.serial_number:08X}")
            self.scanner_ready.set()
        else:
            print("Radar lost.")
            self.scanner_ready.clear()

    # ------------------------------------------------------------------
    # Called by the SDK every time a setting changes on the radar.
    # This fires for ALL settings, so we check which one arrived.
    #
    # CUSTOM_RANGES is special: the radar sends it automatically on
    # connect and again whenever the range list changes. There is no
    # command to request it — we just receive it here passively.
    #
    # sd (SettingData) is a C++ object that is only alive for the
    # duration of this callback. We must extract everything we need
    # before returning.
    # ------------------------------------------------------------------
    def setting_changed(self, sd):
        if sd.get_setting() != qb.Setting.CUSTOM_RANGES:
            return  # ignore everything else

        # get_custom_ranges() returns a list of up to 20 floats.
        # Each float is the radar range in nautical miles for that range index.
        # Index 0 = range step 0, index 1 = range step 1, etc.
        # Unused slots are 0.0.
        ranges = sd.get_custom_ranges()

        print("\nAvailable range steps:")
        print(ranges)

        print(f"\n{'Index':<6}  {'Nautical miles':>14}  {'Metres':>10}")
        print("-" * 35)
        for index, nm in enumerate(ranges):
            if nm > 0:
                metres = nm * NM_TO_M
                print(f"{index:<6}  {nm:>14.4f} nm  {metres:>8.0f} m")

        print()
        self.ranges_received.set()


def main():
    client = RangeClient()

    print("Opening Quantum library...")
    err = qb.open()
    if err != qb.ErrorCode.NO_ERROR:
        print(f"open() failed: {err}")
        sys.exit(1)

    # Register our client so the SDK knows where to send callbacks.
    err = qb.register_notifications(client)
    if err != qb.ErrorCode.NO_ERROR:
        print(f"register_notifications() failed: {err}")
        qb.close()
        sys.exit(1)

    print("Waiting for radar on the network (up to 30 s)...")
    if not client.scanner_ready.wait(timeout=30):
        print("No radar found. Check network / DHCP config.")
        qb.deregister_notifications(client)
        qb.close()
        sys.exit(1)

    # The radar automatically pushes CUSTOM_RANGES on connect.
    # We just wait for it to arrive in setting_changed().
    print("Waiting for range list from radar (up to 10 s)...")
    if not client.ranges_received.wait(timeout=10):
        print("Range list not received. The radar may not have sent it yet.")

    qb.deregister_notifications(client)
    qb.close()


if __name__ == "__main__":
    main()
