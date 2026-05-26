"""
quantum — high-level Python interface to the Raymarine Quantum Radar.

Quick-start
-----------
    import sys
    sys.path.insert(0, r"C:\\Raymarine\\QuantumSDK\\PyQuantumBridge")

    from quantum import INotify, SettingData, RadarMode, open, close
    from quantum import register_notifications, deregister_notifications, new_setting
    from quantum.scanner import ScanBuffer
    import time

    class MyRadar(INotify):
        def __init__(self):
            super().__init__()
            self.serial = None
            self.scan_buffer = ScanBuffer()

        def scanner_list_changed(self, count):
            if count > 0:
                details = get_scanner_details(0)
                self.serial = details.serial_number
                print(f"Scanner: {details.description}  serial=0x{self.serial:08X}")
                cmd = SettingData.radar_mode(self.serial, RadarMode.TRANSMITTING)
                new_setting(cmd)

        def spoke_data_received(self, serial, spoke):
            self.scan_buffer.add_spoke(spoke)

    radar = MyRadar()
    open()
    register_notifications(radar)
    time.sleep(30)
    deregister_notifications(radar)
    close()
"""

# Re-export everything from the compiled extension so users can do
# `from quantum import *` without knowing about quantum_bridge.
from quantum_bridge import (
    # Callbacks
    INotify,
    # Data classes
    SpokeData,
    ScannerDetails,
    MarpaData,
    AlarmData,
    SettingData,
    # Enums
    RadarMode,
    InterferenceRejection,
    TransmitFrequency,
    Channel,
    Preset,
    GainMode,
    ColourGainMode,
    SeaMode,
    RainMode,
    TargetExpansion,
    SeaCurve,
    MainBang,
    DopplerMode,
    AutoAcquireMode,
    DopplerActive,
    TargetState,
    AlarmType,
    Feature,
    Parameter,
    Setting,
    ErrorCode,
    # API functions
    open,
    close,
    register_notifications,
    deregister_notifications,
    new_setting,
    get_scanner_details,
    # Constants
    SAMPLES_PER_SPOKE,
    SPOKES_PER_SCAN,
    MAX_CUSTOM_RANGES,
    DOPPLER_APPROACHING,
    DOPPLER_RECEDING,
)

from quantum.scanner import ScanBuffer

__all__ = [
    "INotify",
    "SpokeData", "ScannerDetails", "MarpaData", "AlarmData", "SettingData",
    "RadarMode", "InterferenceRejection", "TransmitFrequency", "Channel",
    "Preset", "GainMode", "ColourGainMode", "SeaMode", "RainMode",
    "TargetExpansion", "SeaCurve", "MainBang", "DopplerMode",
    "AutoAcquireMode", "DopplerActive", "TargetState", "AlarmType",
    "Feature", "Parameter", "Setting", "ErrorCode",
    "open", "close", "register_notifications", "deregister_notifications",
    "new_setting", "get_scanner_details",
    "SAMPLES_PER_SPOKE", "SPOKES_PER_SCAN", "MAX_CUSTOM_RANGES",
    "DOPPLER_APPROACHING", "DOPPLER_RECEDING",
    "ScanBuffer",
]
