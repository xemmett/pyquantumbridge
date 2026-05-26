# PyQuantumBridge — Complete Technical Documentation

> **Note:** This is a *radar*, not a lidar. Both are distance-sensing technologies, but they work
> differently. A lidar uses laser light pulses; a radar (like this Raymarine Quantum) uses
> radio waves (microwave frequency, ~9.4 GHz). The principles discussed below are specific
> to marine radar.

---

## Table of Contents

1. [What Is a Marine Radar?](#1-what-is-a-marine-radar)
2. [The Raymarine Quantum Radar](#2-the-raymarine-quantum-radar)
3. [How the Radar Communicates With a Computer](#3-how-the-radar-communicates-with-a-computer)
4. [Project Structure](#4-project-structure)
5. [The C++ Extension: quantum_bridge.pyd](#5-the-c-extension-quantum_bridgepyd)
6. [The Python Layer: quantum/](#6-the-python-layer-quantum)
7. [ScanBuffer: Assembling a Full Radar Image](#7-scanbuffer-assembling-a-full-radar-image)
8. [Example Scripts](#8-example-scripts)
9. [Radar Settings Reference](#9-radar-settings-reference)
   - [QuantumClientDialog Command Reference](#quantumclientdialog-command-reference)
   - [Python API — Sending a Command](#python-api--sending-a-command)
10. [Concepts Glossary](#10-concepts-glossary)
11. [Network Setup](#11-network-setup)
12. [Build Instructions](#12-build-instructions)

---

## 1. What Is a Marine Radar?

A marine radar works by spinning an antenna and sending out short bursts of radio energy.
When that energy hits a solid object (a boat, a buoy, a coastline, rain) it bounces back.
The radar measures the time it takes for the echo to return and converts that into a distance.
It also knows which direction the antenna was pointing when it fired, so it knows both the
*range* and the *bearing* (compass direction) of every object it detected.

### Key Terms

- **Antenna / Radome:** The physical spinning unit mounted on a mast. It rotates continuously
  at ~24 RPM (one full 360° sweep roughly every 2.5 seconds).
- **Pulse:** Each brief burst of radio energy the radar fires outward.
- **Echo / Return:** The reflected radio energy that comes back after hitting something.
- **Range:** Distance from the antenna to a detected object.
- **Bearing:** The compass direction (0–360°) from the antenna to the object.
- **PPI (Plan Position Indicator):** The classic circular radar display. The antenna is
  at the centre; the circle represents the maximum range; objects appear as bright spots
  at their correct bearing and range.

### What the Radar Measures Per Pulse

For every direction the antenna points, the radar fires a pulse and records how much energy
comes back at every distance. This produces one **spoke** of data — a 1D array of intensity
values from close (index 0) to far (index 1023). Think of it like a single scan line on a
pie chart, measured once per degree of rotation.

---

## 2. The Raymarine Quantum Radar

The Quantum is a solid-state FMCW (Frequency Modulated Continuous Wave) marine radome.
Key characteristics:

| Property | Value |
|---|---|
| Frequency | ~9.4 GHz (X-band) |
| Rotation speed | ~24 RPM (one sweep ≈ 2.5 s) |
| Spokes per full rotation | 2048 |
| Samples per spoke | 1024 |
| Communication | UDP over Ethernet |
| IP range required | 10.0.x.x – 10.31.x.x |

The radar has several operating modes:
- **SLEEP** — Powered down. Set to STANDBY to wake it.
- **STANDBY** — Powered up, antenna not spinning, not transmitting.
- **TRANSMITTING** — Antenna spinning, firing pulses, sending spoke data.
- **POWER_DOWN** — In the process of shutting down.
- **TIMED_TX** — Transmits for a set number of scans, then goes back to standby automatically.

The radar also supports optional **Doppler** processing. When enabled, moving targets get
special marker values in the spoke data (254 = moving away, 255 = moving toward you).

---

## 3. How the Radar Communicates With a Computer

The Quantum radar communicates over **Ethernet using UDP packets**. The radar does not
use TCP — it broadcasts and multicasts UDP messages on a fixed set of ports.

### Network Requirements

- The PC must have a network adapter configured with an IP address in the range
  `10.0.x.x` to `10.31.x.x` (subnet mask `255.224.0.0`).
- The PC must run a **DHCP server** on that adapter. The radar obtains its own IP
  address from this DHCP server. Without DHCP, the radar cannot be found.
- In this project, tftpd32 is used as the DHCP server. The radar appears at `10.0.0.3`.

### Discovery

The Quantum library (`QuantumLib.dll`) handles all low-level UDP communication internally.
When you call `open()`, the DLL opens sockets, binds to the appropriate network interface,
and begins listening for the radar's UDP discovery broadcasts. When the radar announces
itself, the DLL calls your `scanner_list_changed(count)` callback to tell you a scanner
was found.

---

## 4. Project Structure

```
PyQuantumBridge/
│
├── quantum_bridge.pyd          ← Compiled C++ extension (the bridge itself)
├── QuantumLib.dll              ← Raymarine's official radar SDK (binary, not our code)
├── QuantumLibDebug.cfg         ← Optional: enables SDK debug logging
│
├── src/
│   └── quantum_bridge.cpp      ← C++ source that wraps QuantumLib for Python
│
├── python/
│   └── quantum/
│       ├── __init__.py         ← Re-exports everything from quantum_bridge for convenience
│       └── scanner.py          ← ScanBuffer: assembles spokes into a full radar image
│
├── examples/
│   ├── basic_connect.py        ← Connect, transmit, print spoke statistics
│   ├── radar_image.py          ← Live circular PPI display using matplotlib
│   └── marpa_tracking.py       ← Track moving targets with MARPA
│
└── CMakeLists.txt              ← Build system for quantum_bridge.cpp → .pyd
```

### Dependency Chain

```
Your Python script
    └── imports quantum_bridge   (quantum_bridge.pyd — our C++ extension)
            └── loads QuantumLib.dll   (Raymarine's closed-source SDK)
                    └── talks to the radar over UDP/Ethernet
```

---

## 5. The C++ Extension: quantum_bridge.pyd

### Why Does This Exist?

Raymarine only provides `QuantumLib.dll`, a Windows C++ library. Python cannot call C++
DLLs directly. The file `src/quantum_bridge.cpp` is a thin C++ wrapper written using
**pybind11** that:

1. Exposes the DLL's C++ classes and functions as Python-callable objects.
2. Handles the **GIL** (Python's Global Interpreter Lock) — the radar's callbacks arrive
   on the DLL's internal thread, and Python objects can only be touched by one thread at
   a time. Every callback acquires the GIL before calling into Python.
3. Copies callback data into Python-owned objects so you can safely store them.

### The Callback System: INotify

The core design pattern is the **observer/callback pattern**. You create a Python class
that inherits from `quantum_bridge.INotify` and overrides the methods you care about.
Then you register it with the library.

```python
import quantum_bridge as qb

class MyHandler(qb.INotify):
    def scanner_list_changed(self, count):
        # Called when a radar is found (count=1) or disconnected (count=0)
        pass

    def spoke_data_received(self, serial_number, spoke):
        # Called ~2048 times per second (once per spoke) while transmitting
        pass

    def setting_changed(self, setting_data):
        # Called when any radar setting changes (mode, range, gain, etc.)
        pass

    def feature_changed(self, serial_number, feature, supported):
        # Called once on connect for each optional feature the radar supports
        pass

    def parameter_changed(self, serial_number, parameter, value):
        # Called once on connect for min/max range index and max MARPA targets
        pass

    def marpa_data_changed(self, serial_number, marpa_data):
        # Called when a MARPA-tracked target updates
        pass

    def alarm_data_changed(self, serial_number, alarm_data):
        # Called when a guard zone breach or MARPA alarm fires
        pass
```

### The SpokeData Object

Each call to `spoke_data_received` delivers one `SpokeData` object:

| Field | Type | Meaning |
|---|---|---|
| `bearing` | int (0–2047) | Which direction the antenna was pointing. 0 = north, 1024 = south. Multiply by 360/2048 to get degrees. |
| `data_length` | int | Number of valid samples in this spoke (up to 1024). |
| `instrumented_range` | int | The range (in metres) that corresponds to sample index 1023. Tells you how far out the radar is currently set. |
| `samples_per_spoke` | int | Always 1024 for this radar. |
| `spokes_per_scan` | int | Always 2048 for this radar. |
| `channel` | int | 0 = normal, 1 = second range (Dual Range mode only). |
| `spoke.to_numpy()` | uint8 array | The actual intensity data as a NumPy array. Values 0–253 = signal intensity; 254 = Doppler receding; 255 = Doppler approaching. |

### API Functions

```python
qb.open()                          # Initialize the DLL and open network sockets
qb.close()                         # Close sockets and release resources
qb.register_notifications(handler) # Start receiving callbacks on your handler
qb.deregister_notifications(handler) # Stop receiving callbacks
qb.new_setting(setting_data)       # Send a command to the radar
qb.get_scanner_details(index)      # Get serial number and description of found scanner
```

---

## 6. The Python Layer: quantum/

### `quantum/__init__.py`

This file just re-exports everything from `quantum_bridge` so you can write:

```python
from quantum import INotify, RadarMode, open, close
```

instead of:

```python
from quantum_bridge import INotify, RadarMode, open, close
```

Both work identically — it's purely a convenience wrapper.

### `quantum/scanner.py` — The ScanBuffer Class

The radar sends 2048 individual spokes per full rotation. Each spoke is one "slice" of the
full picture. `ScanBuffer` collects those slices and assembles them into a complete image.

#### How Spokes Build an Image

Imagine the radar face like a clock. The antenna starts at 12 o'clock (bearing 0) and
rotates clockwise. At each small angular step (~0.176°) it fires a pulse and records a
spoke. After 2048 steps it has gone all the way around (360°) and the full image is
complete.

The internal buffer is a 2D NumPy array of shape `(2048, 1024)`:

```
             Range (distance) →
             0        512       1023
Bearing  0   [  ][  ][  ]...[  ]    ← north (12 o'clock)
(angle)  1   [  ][  ][  ]...[  ]
↓        2   [  ][  ][  ]...[  ]
        ...
       2047  [  ][  ][  ]...[  ]    ← just before north again
```

Each cell holds a `uint8` intensity value (0 = no return, 253 = maximum return).

#### Scan Completion Detection

The buffer knows a full rotation has completed when the bearing number wraps back around
from high to low (e.g., bearing 2047 followed by bearing 0). When this wrap is detected,
a snapshot of the buffer is saved as the "latest complete scan". `get_scan()` always
returns this snapshot (or `None` before the first complete rotation).

#### Polar vs. Cartesian Display

The raw `(2048, 1024)` array is in **polar coordinates** — rows are angles, columns are
distances. To display this as the familiar circular radar image, it must be converted to
**Cartesian coordinates** (a normal x/y grid).

`to_cartesian(size=512)` does this conversion:

1. Creates a 512×512 pixel output grid.
2. For each output pixel, calculates its distance and direction from the centre.
3. Looks up the corresponding value in the polar buffer.
4. Pixels outside the radar's circular range are set to black.

The result is a square image where the radar antenna is at the centre, north is up, and
objects appear as bright dots in their correct geographic positions relative to the antenna.

---

## 7. ScanBuffer: Assembling a Full Radar Image

Here is the complete data flow from radar hardware to a displayed pixel:

```
Radar hardware
    │
    │  ~2048 UDP packets/second (one per spoke)
    ▼
QuantumLib.dll (internal C++ threads)
    │
    │  Calls SpokeDataReceived() on the DLL's thread
    ▼
quantum_bridge.cpp: PyINotify::SpokeDataReceived()
    │  Acquires Python GIL
    │  Copies raw SpokeData_t → SpokeDataPy (Python-owned)
    │  Calls Python spoke_data_received(serial, spoke)
    ▼
Your Python spoke_data_received() method
    │  Calls buf.add_spoke(spoke)
    ▼
ScanBuffer.add_spoke()
    │  Extracts bearing (0-2047) and intensity array
    │  Writes to _buf[bearing, :data_length]
    │  Detects wrap-around → snapshots _completed
    ▼
ScanBuffer.get_scan()  ← called by display loop every 0.2s
    │  Returns latest complete (2048, 1024) polar array, or None
    ▼
ScanBuffer.to_cartesian(size=512)
    │  Polar → Cartesian coordinate transform (scipy)
    │  Masks outside-circle pixels to black
    ▼
matplotlib imshow()
    │  Renders 512×512 pixels with custom colour map
    ▼
Display window: circular PPI image
```

---

## 8. Example Scripts

### `examples/basic_connect.py`

The simplest possible example. It:

1. Opens the library and registers for callbacks.
2. Waits up to 5 minutes for a scanner to be found.
3. Commands the radar to `TRANSMITTING` mode.
4. Waits 10 seconds, printing one line of spoke statistics per full scan.
5. Commands the radar back to `STANDBY`.
6. Cleans up and exits.

**What to look for in the output:**
- `[scanner_list_changed] count=1` — radar found.
- `[setting_changed] RadarMode -> RadarMode.TRANSMITTING` — radar confirmed transmitting.
- `Spoke bearing=...` lines — raw data arriving correctly.
- `Total spokes received: ~20480` — 10 seconds × 2048 spokes/rotation × ~1 rotation/2.5s ≈ 8192 spokes. More if the rotation is faster.

**Run it:**
```powershell
python examples\basic_connect.py
```

---

### `examples/radar_image.py`

A live graphical PPI display. It:

1. Connects and starts transmitting (same as basic_connect).
2. Opens a matplotlib window.
3. Every 0.2 seconds, fetches the latest complete scan from `ScanBuffer`.
4. Displays it as a circular image using the custom Doppler colour map.
5. Pressing `C` toggles between circular (Cartesian) and flat strip (polar) views.
6. Closing the window or pressing Ctrl+C sets the radar to STANDBY and exits.

**Colour map:**

| Value | Colour | Meaning |
|---|---|---|
| 0 | Black | No return (open water, air) |
| 1–253 | White→Grey scale | Signal intensity (brighter = stronger echo) |
| 254 | Green | Doppler: target moving away (receding) |
| 255 | Red | Doppler: target moving toward you (approaching) |

**Display modes:**

| Mode | How it looks | Toggle |
|---|---|---|
| Cartesian (default) | Circular PPI, north up, antenna at centre | Press C |
| Polar | Flat 2048×1024 rectangle, bearing on Y axis, range on X | Press C |

**Run it:**
```powershell
python examples\radar_image.py
```

---

### `examples/marpa_tracking.py`

MARPA stands for **Mini Automatic Radar Plotting Aid**. It is a system for tracking
moving targets — other boats, for example. The radar calculates where each tracked target
is, how fast it's moving, and whether it is on a collision course.

This script:

1. Connects and starts transmitting.
2. Sends your own vessel's heading, speed, and course to the radar (so it can calculate
   true rather than relative target motion).
3. Designates one target at a fixed range and bearing for the radar to start tracking.
4. Prints a live table of all tracked targets every 2 seconds.
5. Fires alarm messages if a target becomes dangerous.

**MARPA target states:**

| State | Meaning |
|---|---|
| ACQUIRING | Radar just started tracking; needs ~10 spoke hits to confirm |
| SAFE | Tracking normally; not a collision risk |
| DANGEROUS | Target's closest point of approach (CPA) is within your configured safety distance, and it will get there within the configured time |
| LOST | Radar lost the target (it stopped returning echoes) |

**What MARPA needs to work:**
- Radar in TRANSMITTING mode.
- Your own vessel's heading (compass bearing, degrees true north).
- Optionally: SOG (speed over ground, m/s) and COG (course over ground, degrees) for
  true-vector calculations. Without these, only relative motion is available.

**To designate a target manually**, edit these variables at the top of the script:

```python
TARGET_RANGE_M     = 500.0   # how far away the target is (metres)
TARGET_BEARING_DEG = 90.0    # which direction (degrees, 0=north, 90=east)
```

**Run it:**
```powershell
python examples\marpa_tracking.py
```

---

## 9. Radar Settings Reference

### QuantumClientDialog Command Reference

`QuantumClientDialog.exe` accepts commands in the format:

```
<scanner_index>, <setting_number>, <value(s)>
```

`scanner_index` is always `0` unless you have multiple radars. The setting numbers
and values map directly to the SDK enumerations.

#### Radar Mode (Setting 1)

| Command | Effect |
|---|---|
| `0,1,0` | **Standby** — powered up, antenna spinning stopped |
| `0,1,1` | **Transmitting** — antenna spinning, actively scanning |
| `0,1,3` | **Power down** — shuts the radar off |
| `0,1,4` | **Timed transmit** — transmits for a set number of scans then auto-standby |

#### Range (Setting 20)

| Command | Effect |
|---|---|
| `0,20,0` | Minimum range (closest in) |
| `0,20,N` | Range index N — higher = further out. The radar reports its max index via `parameter_changed(MAX_RANGE)`. The exact distance per index is reported via the `CUSTOM_RANGES` notification on connect. |

#### Display Presets (Setting 21)

Presets apply a preset combination of gain, sea, and rain values in one command.
You can override individual settings afterwards.

| Command | Preset | Best used for |
|---|---|---|
| `0,21,0` | **Harbour** | Busy marinas — high gain, strong clutter suppression, short range |
| `0,21,1` | **Coastal** | Inshore sailing — balanced settings |
| `0,21,2` | **Offshore** | Open water — longer range, less aggressive clutter filtering |
| `0,21,3` | **Weather** | Showing rain and weather systems — interference rejection drops to level 1 so weather echoes aren't filtered out |

#### Gain (Settings 22–23)

Gain controls how sensitive the radar is. Higher gain = more returns, but also more
background noise. Auto gain lets the radar decide.

| Command | Effect |
|---|---|
| `0,22,0` | Gain mode **Manual** — you control the level |
| `0,22,1` | Gain mode **Auto** — radar adjusts automatically |
| `0,23,N` | Gain value 0–100% (only applies in Manual mode) |

#### Colour Gain (Settings 24–25)

Colour gain affects the colour intensity of returns (relevant in Doppler mode).

| Command | Effect |
|---|---|
| `0,24,0` | Colour gain **Manual** |
| `0,24,1` | Colour gain **Auto** |
| `0,25,N` | Colour gain value 0–100% |

#### Sea Clutter (Settings 26–27, 31)

Sea clutter is the radar return from wave surfaces. Without filtering, choppy water
fills the display with noise. Too much filtering and you lose real targets.

| Command | Effect |
|---|---|
| `0,26,0` | Sea clutter **Manual** |
| `0,26,1` | Sea clutter **Auto** — adapts to current sea state |
| `0,27,N` | Sea clutter value 0–100% (manual only) — raise if wave noise is cluttering the display |
| `0,31,0` | Sea curve **R^4** — gentler, removes less near-centre clutter |
| `0,31,1` | Sea curve **R^5.5** — steeper, more aggressively removes clutter close to the antenna. Useful if antenna is mounted high or sea state is rough. |

#### Rain Clutter (Settings 28–29)

Rain shows up as a large diffuse mass of returns. Rain filter reduces this.

| Command | Effect |
|---|---|
| `0,28,0` | Rain filter **Off** |
| `0,28,1` | Rain filter **Manual** |
| `0,29,N` | Rain filter value 0–100% (manual only) — raise to reduce rain return, but be aware real targets inside rain will also dim |

#### Target Expansion (Setting 30)

Makes small targets physically larger on screen so they are easier to spot. Uses a
longer pulse width, which slightly reduces range resolution.

| Command | Effect |
|---|---|
| `0,30,0` | Target expansion **Off** |
| `0,30,1` | Target expansion **On** |

#### Main Bang Suppression (Setting 32)

The "main bang" is a bright blob at the very centre of the PPI caused by the antenna's
own transmission leaking into the receiver. Suppression blanks it out.

| Command | Effect |
|---|---|
| `0,32,0` | Main bang suppression **Off** |
| `0,32,1` | Main bang suppression **On** (default) |

#### Interference Rejection (Setting 2)

Filters out interference from other nearby radars, which appears as spiral streaks
on the display.

| Command | Effect |
|---|---|
| `0,2,0` | **Off** — no filtering |
| `0,2,1` | Level 1 — lightest filtering |
| `0,2,2` | Level 2 |
| `0,2,3` | Level 3 (default) |
| `0,2,4` | Level 4 |
| `0,2,5` | Level 5 — most aggressive; may remove some real targets if overused |

#### Transmit Frequency (Setting 5)

Shifting frequency is another tool to reduce interference from a nearby radar operating
on the same nominal frequency.

| Command | Effect |
|---|---|
| `0,5,0` | **Nominal** — standard 9.4 GHz |
| `0,5,1` | **Low** — slightly below nominal |
| `0,5,2` | **High** — slightly above nominal |

#### Bearing Alignment (Setting 3)

Corrects a compass offset so that 0° on the radar matches true north. Value is in
0.5° steps, range −359 to +360 (i.e. −179.5° to +180.0°).

| Command | Example effect |
|---|---|
| `0,3,0` | No correction |
| `0,3,10` | +5° correction (adds 5° to all bearings) |
| `0,3,-20` | −10° correction |

#### Doppler (Setting 40)

When Doppler is on, moving targets are highlighted: approaching targets appear red
(value 255 in spoke data), receding targets appear green (value 254). Requires own-ship
SOG to separate your motion from the target's motion. Automatically suspended at ranges
≥ 12 nm, or ≥ 8 nm with target expansion on or in Weather preset.

| Command | Effect |
|---|---|
| `0,40,0` | Doppler **Off** |
| `0,40,1` | Doppler **On** |

#### MARPA Target Tracking (Settings 62–63)

| Command | Effect |
|---|---|
| `0,62,<range_m>,<bearing_deg>` | Designate a target to track — e.g. `0,62,750,175` tracks a target 750 m away at bearing 175° |
| `0,63,<target_id>` | Stop tracking a specific target by its ID (0–9) |

---

### Python API — Sending a Command

Settings are sent and received using `SettingData` objects. You create one using a
factory method, then pass it to `qb.new_setting()`.

### Sending a Command

```python
# Set radar mode
cmd = qb.SettingData.radar_mode(serial, qb.RadarMode.TRANSMITTING)
qb.new_setting(cmd)

# Set range (index 5 = approximately 0.5 nm, depending on custom ranges)
cmd = qb.SettingData.range_index(serial, 5)
qb.new_setting(cmd)

# Set gain to manual, value 75%
cmd = qb.SettingData.gain_mode(serial, qb.GainMode.MANUAL)
qb.new_setting(cmd)
cmd = qb.SettingData.gain_value(serial, 75)
qb.new_setting(cmd)
```

### Receiving Notifications

Every setting change (including ones you commanded) triggers `setting_changed()`:

```python
def setting_changed(self, sd):
    setting = sd.get_setting()

    if setting == qb.Setting.RADAR_MODE:
        mode = sd.get_radar_mode()       # returns a RadarMode enum value
        print(f"Mode is now: {mode}")

    elif setting == qb.Setting.RANGE:
        index = sd.get_range_index()     # returns uint8 index
        print(f"Range index: {index}")

    elif setting == qb.Setting.GAIN_VALUE:
        gain = sd.get_gain_value()       # returns 0-100
        print(f"Gain: {gain}%")
```

### Available Settings

| Setting name | Command? | Notify? | What it controls |
|---|---|---|---|
| `RADAR_MODE` | Yes | Yes | Standby / Transmitting / etc. |
| `RANGE` | Yes | Yes | Range index (0 = min, max from `parameter_changed`) |
| `GAIN_MODE` | Yes | Yes | Auto or Manual gain |
| `GAIN_VALUE` | Yes | Yes | Manual gain level 0–100% |
| `SEA_MODE` | Yes | Yes | Auto or Manual sea clutter filter |
| `SEA_VALUE` | Yes | Yes | Manual sea clutter 0–100% |
| `RAIN_MODE` | Yes | Yes | Rain clutter filter Off or Manual |
| `RAIN_VALUE` | Yes | Yes | Manual rain clutter 0–100% |
| `INTERFERENCE_REJECTION` | Yes | Yes | Filter other radars (Off, Level 1–5) |
| `DOPPLER_MODE` | Yes | Yes | Doppler target highlighting On/Off |
| `DOPPLER_ACTIVE` | No | Yes | Whether Doppler is actually running (may be suspended at long range) |
| `PRESET_MODE` | Yes | Yes | Harbour / Coastal / Offshore / Weather preset |
| `TARGET_EXPANSION` | Yes | Yes | Expand target size for easier spotting |
| `BEARING_ALIGNMENT` | Yes | Yes | Correct compass offset (±180°) |
| `TIMED_TRANSMIT` | Yes | Yes | Auto standby/transmit cycling |
| `NAV_DATA` | Yes | No | Send heading/SOG/COG to radar for MARPA/Doppler |
| `MARPA_DESIGNATE` | Yes | No | Tell radar to start tracking a target |
| `MARPA_CLEAR` | Yes | No | Stop tracking a target |
| `GUARD_ZONE_1/2` | Yes | Yes | Set up alarm zones (range + angle sector) |
| `MAIN_SOFTWARE_VERSION` | No | Yes | Radar firmware version (notified on connect) |
| `CUSTOM_RANGES` | No | Yes | List of range values (nm) for each range index |

---

## 10. Concepts Glossary

**Bearing** — A compass direction expressed in degrees (0–360, clockwise from north) or as
an index (0–2047 for this radar, where 2048 steps = 360°).

**CPA (Closest Point of Approach)** — For a tracked target, the minimum distance it will
get to your vessel if both continue on their current courses.

**Doppler** — A technique that detects whether a target is moving relative to the radar by
measuring a tiny frequency shift in the returning echo (the same effect that makes an
ambulance siren change pitch as it passes you). Requires the radar to know your own vessel's
speed to separate your motion from the target's motion.

**FMCW (Frequency Modulated Continuous Wave)** — The signal type the Quantum uses. Instead
of short pulses, it continuously transmits a signal whose frequency sweeps up and down.
Range is determined by comparing the frequency of the outgoing signal to the returning echo.
FMCW allows the Quantum to detect very close targets and use less power than pulse radars.

**GIL (Global Interpreter Lock)** — A Python mechanism that prevents multiple threads from
executing Python code simultaneously. The radar DLL delivers callbacks on its own thread, so
the C++ bridge must acquire the GIL before calling any Python function.

**Guard Zone** — A defined area (an arc-shaped sector at a specified range and bearing) that
triggers an alarm when any echo appears inside it.

**Instrumented Range** — The maximum distance the radar is currently set to display. The
`instrumented_range` field on each spoke gives you this value in metres. Sample index 1023
on that spoke corresponds to this distance.

**MARPA (Mini Automatic Radar Plotting Aid)** — Automatic tracking of up to 10 (or more,
depending on firmware) manually or automatically designated targets. The radar continuously
updates each target's range, bearing, speed, and course.

**PPI (Plan Position Indicator)** — The circular radar display. Your vessel is at the centre.
Objects appear at their correct bearing and range. North is typically at the top.

**Polar coordinates** — A way of describing a position using distance and angle rather than
x/y. Radar data is naturally in polar form (range + bearing). Displaying it in polar form
gives a flat rectangular strip; converting to Cartesian (x/y) gives the familiar circle.

**Serial number** — A unique 32-bit number identifying this specific radar unit. Most API
calls require it so the library knows which radar you mean (you could theoretically have
multiple radars).

**Spoke** — One radial "slice" of a radar scan: a 1D array of intensity samples representing
all the echoes detected in one specific direction during one rotation step.

**TCPA (Time to Closest Point of Approach)** — How many seconds until a tracked target
reaches its closest point to your vessel.

---

## 11. Network Setup

The radar requires:

1. **A dedicated Ethernet adapter** on the PC connected directly to the radar (or via
   a switch with no other DHCP servers present).
2. **Static IP on the adapter**: Set it to something in `10.0.0.1`–`10.31.255.254`,
   e.g. `10.0.0.1`, subnet mask `255.224.0.0`.
3. **A DHCP server** running on that adapter. [tftpd32](http://tftpd32.jounin.net/) is
   a lightweight option. Configure the DHCP pool to hand out addresses like `10.0.0.2`–
   `10.0.0.50`. The radar will take one (typically `10.0.0.3`).
4. **Windows Firewall**: Allow inbound UDP for the Python process, or add a rule:
   ```powershell
   New-NetFirewallRule -DisplayName "QuantumRadar Python" -Direction Inbound `
       -Program "$env:VIRTUAL_ENV\Scripts\python.exe" -Action Allow -Protocol UDP
   ```

**Verifying the radar is reachable:**
```powershell
ping 10.0.0.3        # should reply
arp -a               # should show 10.0.0.3 with a MAC address
```

---

## 12. Build Instructions

The `quantum_bridge.pyd` extension is built from `src/quantum_bridge.cpp` using CMake
and Visual Studio Build Tools. A pre-built `.pyd` is included, so you only need to rebuild
if you modify the C++ source.

### Prerequisites

- Visual Studio Build Tools 2022 (or later) with the C++ workload
- Python 3.12 in a virtual environment (`.venv`)
- pybind11: `pip install pybind11`

### Steps

```powershell
# 1. Activate the virtual environment
.\.venv\Scripts\Activate.ps1

# 2. Configure (run once, or after changing CMakeLists.txt)
Remove-Item -Recurse -Force build -ErrorAction SilentlyContinue
cmake -B build -G "Visual Studio 18 2026" -A x64

# 3. Build
cmake --build build --config Release

# 4. Copy the new .pyd to the project root
copy build\Release\quantum_bridge*.pyd .
```

The `.pyd` file is a regular Windows DLL renamed with a `.pyd` extension. Python loads it
the same way it loads any `.dll`. The `QuantumLib.dll` must be in the same directory.

### Debug Logging

To enable SDK-level debug output, create `QuantumLibDebug.cfg` in the project root:

```
Filename: QDebug.txt
Output: 3
Level: 2
```

Levels: 1=errors only, 2=network/socket events, 3=commands sent, 5=system packets,
6=all radar packets. The log file `QDebug.txt` will be written alongside the DLL.
