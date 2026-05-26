"""
Alternative build script — use if you prefer pip over cmake.

Usage:
    pip install pybind11 numpy scipy matplotlib
    python setup.py build_ext --inplace

After building, copy QuantumLib.dll from Output/Release_x64/ next to the
generated quantum_bridge*.pyd before importing.
"""
import os
from pathlib import Path
from pybind11.setup_helpers import Pybind11Extension, build_ext
from setuptools import setup

HERE   = Path(__file__).parent
SDK    = HERE.parent
INC    = str(SDK / "QuantumLib" / "QuantumLib" / "Include")
LIBDIR = str(SDK / "Output" / "Release_x64")

if not (Path(LIBDIR) / "QuantumLib.lib").exists():
    raise FileNotFoundError(
        f"QuantumLib.lib not found in {LIBDIR}.\n"
        "Build the QuantumLib project in Visual Studio (Release x64) first."
    )

ext = Pybind11Extension(
    "quantum_bridge",
    sources=["src/quantum_bridge.cpp"],
    include_dirs=[INC],
    library_dirs=[LIBDIR],
    libraries=["QuantumLib"],
    extra_compile_args=["/wd4251"],   # suppress DLL-interface warnings
    cxx_std=14,
)

setup(
    name="quantum-radar",
    version="1.0.0",
    description="Python bridge for Raymarine Quantum Radar SDK",
    ext_modules=[ext],
    cmdclass={"build_ext": build_ext},
    python_requires=">=3.8",
    install_requires=["numpy", "scipy", "matplotlib"],
)
