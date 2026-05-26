"""
ScanBuffer — accumulates radar spokes into a 2-D polar image array.

The Quantum radar emits 2048 spokes per full rotation, each spoke containing
up to 1024 uint8 intensity samples (distance bins from the antenna outward).
Stacking spokes by bearing produces a 2048×1024 polar image where:
  - axis 0 = bearing (0..2047,  multiply by 360/2048 ≈ 0.176° per step)
  - axis 1 = range   (0..1023,  bin 0 is nearest, 1023 is farthest)

When Doppler mode is active, values 254 and 255 have special meaning:
  - 254 = Doppler receding target  (use quantum_bridge.DOPPLER_RECEDING)
  - 255 = Doppler approaching target (use quantum_bridge.DOPPLER_APPROACHING)
"""

import threading
import numpy as np

try:
    from scipy.ndimage import map_coordinates
    _SCIPY_AVAILABLE = True
except ImportError:
    _SCIPY_AVAILABLE = False


class ScanBuffer:
    """Thread-safe accumulator for radar spoke data.

    Typical usage inside an INotify subclass::

        class MyRadar(INotify):
            def __init__(self):
                super().__init__()
                self.buf = ScanBuffer()

            def spoke_data_received(self, serial, spoke):
                self.buf.add_spoke(spoke)
    """

    def __init__(self, spokes_per_scan: int = 2048, samples_per_spoke: int = 1024):
        self._spokes = spokes_per_scan
        self._samples = samples_per_spoke
        self._buf = np.zeros((spokes_per_scan, samples_per_spoke), dtype=np.uint8)
        self._completed: np.ndarray | None = None
        self._last_bearing = -1
        self._instrumented_range: int = 0
        self._lock = threading.Lock()

    def add_spoke(self, spoke) -> None:
        """Add one SpokeData into the accumulator.

        Detects scan completion when the bearing wraps from near-2047 back
        toward 0, then snapshots the buffer as the latest complete scan.
        """
        data = spoke.to_numpy()
        length = len(data)
        bearing = spoke.bearing

        with self._lock:
            self._buf[bearing, :length] = data
            if bearing < self._last_bearing:
                self._completed = self._buf.copy()
            self._last_bearing = bearing
            if spoke.instrumented_range:
                self._instrumented_range = spoke.instrumented_range

    def get_scan(self) -> np.ndarray | None:
        """Return the latest complete polar scan, or None before the first full rotation.

        Shape: (spokes_per_scan, samples_per_spoke) = (2048, 1024), dtype uint8.
        """
        with self._lock:
            return self._completed

    def get_instrumented_range(self) -> int:
        """Return the most recent instrumented_range reported by the radar (metres).

        Zero until the first spoke arrives.
        """
        with self._lock:
            return self._instrumented_range

    def to_cartesian(self, size: int = 512, interp_order: int = 1) -> np.ndarray | None:
        """Remap the latest polar scan to a square Cartesian image.

        Parameters
        ----------
        size:
            Output image will be size × size pixels. The radar is centred in
            the middle; the full range extends to the edges.
        interp_order:
            Interpolation order passed to scipy.ndimage.map_coordinates
            (0=nearest, 1=bilinear, 3=cubic). Ignored if scipy is unavailable.

        Returns
        -------
        numpy.ndarray of shape (size, size) uint8, or None if no scan yet.
        Raises RuntimeError if scipy is not installed.
        """
        scan = self.get_scan()
        if scan is None:
            return None

        if not _SCIPY_AVAILABLE:
            raise RuntimeError(
                "scipy is required for to_cartesian(). "
                "Install it with: pip install scipy"
            )

        spokes, samples = scan.shape
        half = size / 2.0

        # Build a grid of (x, y) pixel coordinates centred on the image
        ys, xs = np.mgrid[0:size, 0:size]
        dx = (xs - half) / half   # -1..1
        dy = (half - ys) / half   # -1..1 (y flipped: screen top = north)

        # Convert Cartesian to polar
        r = np.sqrt(dx**2 + dy**2)          # 0..sqrt(2) at corners, clip to 1
        theta = np.arctan2(dx, dy)          # angle from north, clockwise
        theta = (theta % (2 * np.pi))      # 0..2π

        # Map to spoke index (0..spokes-1) and sample index (0..samples-1)
        spoke_coords   = theta / (2 * np.pi) * spokes
        sample_coords  = r * samples

        # Pixels outside the radar circle get value 0
        mask = r > 1.0
        sample_coords  = np.clip(sample_coords, 0, samples - 1)

        coords = np.array([spoke_coords.ravel(), sample_coords.ravel()])
        result = map_coordinates(
            scan, coords,
            order=interp_order, mode='wrap', prefilter=False
        ).reshape(size, size).astype(np.uint8)

        result[mask] = 0
        return result

    @property
    def spokes_per_scan(self) -> int:
        return self._spokes

    @property
    def samples_per_spoke(self) -> int:
        return self._samples
