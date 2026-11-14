"""Summarize PresentMon 1.x-schema inter-present intervals (not GPU times).

Usage:
    python benchmarks/scripts/normalize_presentmon.py capture.csv --process-id 1234 --output-dir results/run/analysis

Python 3.9+, standard library only. Required CSV columns: Application, ProcessID,
SwapChainAddress, TimeInSeconds, MsBetweenPresents (legacy msBetweenPresents
is also accepted). Use PresentMon --v1_metrics.
--swap-chain explicitly selects a stream when a PID has multiple swap chains.
--metadata optionally embeds capture metadata. Output is a NEW directory with
frames.csv and summary.json. Raw input is read only and its SHA-256 is recorded.
Existing output, missing columns, mixed applications, ambiguous streams, reversed
timestamps, invalid intervals, or no usable intervals fail with exit code 1.
The first row of the selected stream is excluded: its preceding present is outside
the selected sample. Later intervals must be finite and positive; no outlier trim.
"""

import argparse
import csv
import hashlib
import json
import math
from pathlib import Path


def percentile(ordered, fraction):
    """Linear interpolation at (n-1)*fraction (Hyndman-Fan type 7)."""
    position = (len(ordered) - 1) * fraction
    low = math.floor(position)
    high = math.ceil(position)
    return ordered[low] + (ordered[high] - ordered[low]) * (position - low)


def summarize(intervals):
    """Compute the seven M0 metrics from K finite positive millisecond intervals."""
    if not intervals or any(not math.isfinite(t) or t <= 0 for t in intervals):
        raise ValueError("expected at least one finite, positive frame interval")
    total = math.fsum(intervals)
    if not math.isfinite(total):
        raise ValueError("interval sum overflow")
    ordered = sorted(intervals)
    count = len(intervals)
    return {
        "average_fps": 1000.0 * count / total,
        "mean_frame_time_ms": total / count,
        "median_frame_time_ms": percentile(ordered, 0.50),
        "p95_frame_time_ms": percentile(ordered, 0.95),
        "p99_frame_time_ms": percentile(ordered, 0.99),
        "frame_count": count,
        "measurement_duration_s": total / 1000.0,
    }


def read_frames(path, process_id, swap_chain=None):
    """Select exactly one PID/swap chain; retain CSV row numbers for raw-data audit."""
    required = {"Application", "ProcessID", "SwapChainAddress", "TimeInSeconds", "MsBetweenPresents"}
    with path.open(encoding="utf-8-sig", newline="") as source:
        reader = csv.DictReader(source)
        # PresentMon 1.x used a lower-case 'ms'; accept only this known alias.
        if reader.fieldnames and "msBetweenPresents" in reader.fieldnames:
            if "MsBetweenPresents" in reader.fieldnames:
                raise ValueError("duplicate interval columns")
            reader.fieldnames = ["MsBetweenPresents" if name == "msBetweenPresents" else name
                                 for name in reader.fieldnames]
        if not required.issubset(reader.fieldnames or []):
            raise ValueError("missing required PresentMon v1 columns: " +
                             ", ".join(sorted(required - set(reader.fieldnames or []))))
        if len(reader.fieldnames) != len(set(reader.fieldnames)):
            raise ValueError("duplicate CSV column names")
        selected = []
        for line, row in enumerate(reader, start=2):
            if (row.get("ProcessID") or "").strip() != str(process_id):
                continue
            if None in row or any(row.get(field) is None for field in required):
                raise ValueError(f"malformed CSV row {line}")
            if swap_chain is None or row.get("SwapChainAddress") == swap_chain:
                selected.append((line, row))
    if len(selected) < 2:
        raise ValueError("need at least two presents for the selected PID/swap chain")
    streams = {row["SwapChainAddress"] for _, row in selected}
    applications = {row["Application"] for _, row in selected}
    if len(streams) != 1 or not next(iter(streams)):
        raise ValueError("ambiguous swap chains; specify --swap-chain")
    if len(applications) != 1 or not next(iter(applications)):
        raise ValueError("missing or mixed application identity")
    frames = []
    previous_time = None
    for line, row in selected:
        timestamp = float(row["TimeInSeconds"])
        if not math.isfinite(timestamp) or timestamp < 0:
            raise ValueError(f"invalid TimeInSeconds at CSV row {line}")
        if previous_time is not None:
            if timestamp <= previous_time:
                raise ValueError(f"non-increasing TimeInSeconds at CSV row {line}")
            interval = float(row["MsBetweenPresents"])
            if not math.isfinite(interval) or interval <= 0:
                raise ValueError(f"invalid MsBetweenPresents at CSV row {line}")
            frames.append({"source_row": line, "time_s": timestamp, "frame_time_ms": interval})
        previous_time = timestamp
    # These are presentation descriptors, not additional performance metrics.
    fields = ["Runtime", "PresentRuntime", "SyncInterval", "PresentFlags", "AllowsTearing", "PresentMode"]
    observed = {field: sorted({row.get(field) or "unknown" for _, row in selected})
                for field in fields if any(field in row for _, row in selected)}
    return frames, {
        "application": next(iter(applications)), "process_id": process_id,
        "swap_chain": next(iter(streams)), "selected_present_rows": len(selected),
        "excluded_first_present_rows": 1, "observed_presentation": observed,
        "first_present_time_s": float(selected[0][1]["TimeInSeconds"]),
        "last_present_time_s": previous_time,
    }


def normalize(source, output, process_id, swap_chain=None, metadata_path=None):
    """Validate everything before creating outputs; refuse any existing directory."""
    if output.exists():
        raise FileExistsError(f"refusing to overwrite output: {output}")
    frames, selection = read_frames(source, process_id, swap_chain)
    metadata = None
    if metadata_path is not None:
        metadata = json.loads(metadata_path.read_text(encoding="utf-8-sig"))
    summary = {
        "schema_version": 1,
        "metric_set": "M0",
        "interval_definition": "MsBetweenPresents: consecutive presents, not GPU execution or display time",
        "percentile_definition": "linear interpolation at (K-1)*p, Hyndman-Fan type 7",
        "frame_count_definition": "K accepted intervals; K+1 selected present events",
        "duration_definition": "sum of accepted intervals / 1000; not requested capture wall time",
        "comparison_status": "non_comparable_pending_presentation_and_protocol_review",
        "raw_csv": str(source.resolve()),
        "raw_sha256": hashlib.sha256(source.read_bytes()).hexdigest(),
        "selection": selection,
        "metrics": summarize([frame["frame_time_ms"] for frame in frames]),
        "capture_metadata": metadata,
    }
    output.mkdir(parents=True, exist_ok=False)
    with (output / "frames.csv").open("x", encoding="utf-8", newline="") as target:
        writer = csv.DictWriter(target, fieldnames=["source_row", "time_s", "frame_time_ms"])
        writer.writeheader()
        writer.writerows(frames)
    with (output / "summary.json").open("x", encoding="utf-8") as target:
        json.dump(summary, target, indent=2, allow_nan=False)
        target.write("\n")
    return summary


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("csv", type=Path)
    parser.add_argument("--process-id", type=int, required=True)
    parser.add_argument("--swap-chain")
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument("--metadata", type=Path)
    args = parser.parse_args()
    try:
        result = normalize(args.csv, args.output_dir, args.process_id, args.swap_chain, args.metadata)
    except (OSError, ValueError, TypeError, csv.Error) as error:
        print(f"FAIL: {error}")
        return 1
    print(json.dumps(result["metrics"], indent=2))
    print("PASS: normalized; comparison requires presentation/protocol review")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
