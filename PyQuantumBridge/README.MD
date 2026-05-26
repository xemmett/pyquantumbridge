Make sure radar is powered, connected to computer through ethernet on a powered switch.

Use tftpd32 to set up a DHCP server:

Enter the settings page and enter these values:
IP pool start address 10.0.0.2
Size of pool: 50

Mask (Opt 1): 255.254.0.0

Save that.

Next set your ethernet port to same subnet:
win + r
enter:   ncpa.cpl
ethernet < properties < Internet Protocol Version 4
Use following IP address: 
  10.0.0.1
  255.254.0.0

Now power cycle the radar
A new IP address should appear in the tftp32 window under DHCP tab, any other devices on the same subnet will too, there should be at least one. - It will take a few seconds
run windows cmd: ping <IP Address>

Now attempt to run .\QuantumSDK\Output\Release_Win32\QuantumClientDialog.exe

If connection to radar is successful and you have the raymarine authentication USB Key plugged into your computer, you will see settings and dialog and other text data.

Send 0,1,1 as a command to start the radar and start retrieving spokes, (0,1,0 to stop it), you will see acknowledgement in the notifications list.

Next use .\QuantumSDK\Output\Release_Win32\QuantumClientPPI.exe, to view images of what the radar sees, if it is a blank white page, send 0,1,1 again from QuantumClientDialog.exe (You can run multiple programs asynchronously)

If these two programs are successful then move onto the python code.
  
# Project Directory
setup.py                   ← alternative pip build
python/quantum/__init__.py ← high-level Python API
python/quantum/scanner.py  ← ScanBuffer (accumulates spokes → image)
examples/basic_connect.py  ← connect, transmit, print spokes
examples/radar_image.py    ← live matplotlib PPI display
examples/marpa_tracking.py ← MARPA target table

To build and run:

  # 1. Install Python deps
  pip install pybind11 numpy scipy matplotlib

  # 2. Build the C++ extension (requires MSVC x64 — run from Developer PowerShell)
  cd C:\Raymarine\QuantumSDK\PyQuantumBridge
  cmake -B build -A x64
  or
   cmake -B build -G "Visual Studio 18 2026" -A x64
  or
    .\.venv\Scripts\Activate.ps1

    # Install pybind11
    pip install pybind11

    # Confirm it's found
    python -c "import pybind11; print(pybind11.get_cmake_dir())"

    Then use that printed path in CMake:

    Remove-Item -Recurse -Force build
    cmake -B build -G "Visual Studio 18 2026" -A x64 -Dpybind11_DIR="<path printed above>"
  cmake --build build --config Release
  copy build\Release\quantum_bridge*.pyd .
  copy "C:\Raymarine\QuantumSDK\Output\Release_x64\QuantumLib.dll" "C:\Raymarine\QuantumSDK\PyQuantumBridge\"

  # 3. Run examples
  python examples\basic_connect.py     # connect + print spoke data
  python examples\radar_image.py       # live radar PPI image
  python examples\marpa_tracking.py    # MARPA target table

  Key design notes:
  - All callbacks acquire the Python GIL (they arrive on the DLL's thread)
  - Spoke data, SettingData, MarpaData, and AlarmData passed to Python are owned copies — safe to store
   beyond the callback
  - spoke.to_numpy() returns a (data_length,) uint8 numpy array; ScanBuffer.get_scan() returns the full
   (2048, 1024) polar image; ScanBuffer.to_cartesian(size=512) gives a square Cartesian remap