import sys
import csv
from datetime import datetime
from pathlib import Path
import numpy as np

data_dir = Path(sys.argv[1]) if len(sys.argv) > 1 else sorted(Path(".").glob("data_????????_??????"))[-1]

for npz_file in sorted(data_dir.glob("*.npz"), key=lambda p: p.stat().st_mtime, reverse=True):
    print(f"Processing {npz_file.name}...")
    d = np.load(npz_file)

    timestamps        = [datetime.fromtimestamp(float(t)).strftime("%Y-%m-%d %H:%M:%S.%f") for t in d["timestamps"]]
    bearings          = d["bearings"].tolist()
    instrumented_ranges = d["instrumented_ranges"].tolist() if "instrumented_ranges" in d.files else [""] * len(bearings)
    channels          = d["channels"].tolist()
    data_lengths      = d["data_lengths"].tolist()
    spoke_data        = d["spoke_data"].tolist()

    header = ["timestamp", "bearing", "instrumented_range", "channel", "data_length"] + [f"s{i}" for i in range(d["spoke_data"].shape[1])]

    csv_file = npz_file.with_suffix(".csv")
    with csv_file.open("w", newline="") as f:
        w = csv.writer(f)
        w.writerow(header)
        w.writerows(
            [ts, b, ir, ch, dl] + row
            for ts, b, ir, ch, dl, row in zip(timestamps, bearings, instrumented_ranges, channels, data_lengths, spoke_data)
        )

    print(f"  -> {csv_file.name}  ({len(bearings)} rows, {len(header)} columns)")
