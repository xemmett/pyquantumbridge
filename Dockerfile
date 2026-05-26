# ── Stage 1: build the quantum_bridge C++ extension ──────────────────────────
#
# IMPORTANT: QuantumLib.lib is a Windows MSVC static library (COFF/PE format).
# The Linux g++ linker CANNOT use it. Before running `docker compose build`,
# you must place a Linux-compatible static archive at:
#
#   pyquantumbridge/Output/Release_x64/libQuantumLib.a
#
# Obtain this from Raymarine (request a Linux SDK build) or compile the
# QuantumLib sources on Linux with g++ and archive with:
#   ar rcs libQuantumLib.a <object files>
#
# If you only need the FLIR camera services, comment out the quantum-radar
# and radar-dhcp services in docker-compose.yml and run:
#   docker compose up flir-api flir-ui
#
FROM python:3.11-slim AS builder

RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential \
        cmake \
        git \
        python3-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /build

RUN pip install --no-cache-dir "pybind11[global]" numpy scipy matplotlib

# Build context = ./pyquantumbridge, so both PyQuantumBridge/ and Output/ are available
COPY QuantumLib/        /build/QuantumLib/
COPY PyQuantumBridge/   /build/PyQuantumBridge/
COPY Output/            /build/Output/

# Validate that a Linux-compatible library is present and fail with a clear message
RUN set -e; \
    LIB=/build/Output/Release_x64/libQuantumLib.a; \
    if [ ! -f "$LIB" ]; then \
        echo ""; \
        echo "ERROR ──────────────────────────────────────────────────────────"; \
        echo "  libQuantumLib.a not found at:"; \
        echo "    pyquantumbridge/Output/Release_x64/libQuantumLib.a"; \
        echo ""; \
        echo "  QuantumLib.lib is a Windows MSVC library and cannot be used"; \
        echo "  on Linux. See README.md > 'Obtaining QuantumLib for Linux'."; \
        echo "  To skip the radar service: comment out quantum-radar and"; \
        echo "  radar-dhcp in docker-compose.yml, then run:"; \
        echo "    docker compose up flir-api flir-ui"; \
        echo "────────────────────────────────────────────────────────────────"; \
        echo ""; \
        exit 1; \
    fi

# Patch CMakeLists.txt for Linux — three MSVC-specific issues:
#   1. QuantumLib.lib  → libQuantumLib.a  (COFF → ELF archive)
#   2. QuantumLib.dll  → libQuantumLib.so (DLL copy post-build becomes a no-op)
#   3. /wd4251         → removed          (MSVC-only warning flag, rejected by g++)
WORKDIR /build/PyQuantumBridge
RUN sed -i \
        -e 's|QuantumLib\.lib|libQuantumLib.a|g' \
        -e 's|QuantumLib\.dll|libQuantumLib.so|g' \
        -e 's|/wd4251||g' \
        CMakeLists.txt && \
    cmake -S . -B _build \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_CXX_FLAGS="-w" \
    && cmake --build _build --parallel "$(nproc)"

# ── Stage 2: runtime ──────────────────────────────────────────────────────────
FROM python:3.11-slim AS runtime

# python3-tk + X11 libraries required for Tkinter GUI forwarded via VcXsrv
RUN apt-get update && apt-get install -y --no-install-recommends \
        python3-tk \
        tk \
        libx11-6 \
        libxext6 \
        libxrender1 \
        libfontconfig1 \
        libglib2.0-0 \
    && rm -rf /var/lib/apt/lists/*

RUN pip install --no-cache-dir numpy scipy matplotlib

WORKDIR /app

# Copy the compiled .so extension from the builder stage
COPY --from=builder /build/PyQuantumBridge/_build/quantum_bridge*.so /app/
# Copy the full Python source tree
COPY --from=builder /build/PyQuantumBridge/ /app/

ENV PYTHONUNBUFFERED=1
# DISPLAY is injected by docker-compose.yml (defaults to host.docker.internal:0.0)

CMD ["python", "main.py"]
