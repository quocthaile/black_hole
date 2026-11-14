"""Run synthetic M0 statistics, CSV and integrity tests (no GPU/PresentMon).

Usage: python -m unittest discover -s benchmarks/scripts -p 'test_*.py' -v
Python standard library only; temporary fixtures are automatically removed.
These fixtures test mathematics/error handling, not actual simulation performance.
"""

import csv
import hashlib
from pathlib import Path
import tempfile
import unittest

from normalize_presentmon import normalize, read_frames, summarize
from verify_baseline import git_blob_sha1, verify


class MetricsTests(unittest.TestCase):
    def test_fps_uses_total_time(self):
        result = summarize([10, 30])
        self.assertEqual(result["average_fps"], 50)
        self.assertEqual(result["mean_frame_time_ms"], 20)
        self.assertEqual(result["frame_count"], 2)
        self.assertEqual(result["measurement_duration_s"], 0.04)

    def test_percentiles(self):
        result = summarize([40, 10, 30, 20])
        self.assertEqual(result["median_frame_time_ms"], 25)
        self.assertAlmostEqual(result["p95_frame_time_ms"], 38.5)
        self.assertAlmostEqual(result["p99_frame_time_ms"], 39.7)
        self.assertEqual(summarize([7])["p99_frame_time_ms"], 7)
        self.assertEqual(summarize([10, 20, 30])["median_frame_time_ms"], 20)

    def test_invalid_intervals(self):
        for values in ([], [0], [-1], [float("nan")], [float("inf")]):
            with self.subTest(values=values), self.assertRaises(ValueError):
                summarize(values)


class CsvTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.raw = self.root / "raw.csv"

    def write_csv(self, rows=None, column="msBetweenPresents"):
        if rows is None:
            rows = [["test.exe", 42, "0", 0, 0],
                    ["test.exe", 42, "0", 0.01, 10],
                    ["test.exe", 42, "0", 0.04, 30]]
        with self.raw.open("w", encoding="utf-8-sig", newline="") as target:
            writer = csv.writer(target)
            writer.writerow(["Application", "ProcessID", "SwapChainAddress", "TimeInSeconds", column])
            writer.writerows(rows)

    def test_legacy_and_current_header_and_raw_preservation(self):
        for index, column in enumerate(("msBetweenPresents", "MsBetweenPresents")):
            self.write_csv(column=column)
            before = self.raw.read_bytes()
            result = normalize(self.raw, self.root / f"analysis-{index}", 42)
            self.assertEqual(result["metrics"]["average_fps"], 50)
            self.assertEqual(result["selection"]["excluded_first_present_rows"], 1)
            self.assertEqual(self.raw.read_bytes(), before)
            self.assertEqual(result["raw_sha256"], hashlib.sha256(before).hexdigest())

    def test_overwrite_protection(self):
        self.write_csv()
        output = self.root / "analysis"
        normalize(self.raw, output, 42)
        before = (output / "summary.json").read_bytes()
        with self.assertRaises(FileExistsError):
            normalize(self.raw, output, 42)
        self.assertEqual((output / "summary.json").read_bytes(), before)

    def test_missing_header_or_pid(self):
        self.raw.write_text("Application,CPUFrameTime\nx,12\n")
        with self.assertRaises(ValueError):
            read_frames(self.raw, 42)
        self.write_csv()
        with self.assertRaises(ValueError):
            read_frames(self.raw, 43)

    def test_bad_interval_and_time(self):
        for value in ("", "NA", "nan", "inf", "0", "-1"):
            self.write_csv([["test.exe", 42, "0", 0, 0], ["test.exe", 42, "0", 1, value]])
            with self.subTest(value=value), self.assertRaises(ValueError):
                read_frames(self.raw, 42)
        self.write_csv([["test.exe", 42, "0", 1, 0], ["test.exe", 42, "0", 0, 10]])
        with self.assertRaises(ValueError):
            read_frames(self.raw, 42)

    def test_ambiguous_stream_requires_selection(self):
        self.write_csv([["test.exe", 42, "a", 0, 0], ["test.exe", 42, "b", 0.01, 10],
                        ["test.exe", 42, "a", 0.02, 20], ["other.exe", 99, "c", 0.03, 30]])
        with self.assertRaises(ValueError):
            read_frames(self.raw, 42)
        frames, selection = read_frames(self.raw, 42, "a")
        self.assertEqual(len(frames), 1)
        self.assertEqual(selection["application"], "test.exe")

    def test_invalid_input_creates_no_output(self):
        self.write_csv(rows=[])
        output = self.root / "analysis"
        with self.assertRaises(ValueError):
            normalize(self.raw, output, 42)
        self.assertFalse(output.exists())

    def test_truncated_rows_and_duplicate_columns(self):
        self.write_csv([["test.exe", 42, "0", 0, 0], ["test.exe", 42, "0", 1]])
        with self.assertRaises(ValueError):
            read_frames(self.raw, 42)
        self.write_csv()
        original = self.raw.read_text(encoding="utf-8-sig")
        self.raw.write_text(original.replace("TimeInSeconds,", "ProcessID,TimeInSeconds,"))
        with self.assertRaises(ValueError):
            read_frames(self.raw, 42)


class IntegrityTests(unittest.TestCase):
    def test_line_endings_changes_missing_and_extra_files(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            folder = root / "src/baseline"
            folder.mkdir(parents=True)
            path = folder / "a.cpp"
            original = b"original\n"
            entry = {"path": "src/baseline/a.cpp", "lf_git_blob_sha1": git_blob_sha1(original),
                     "lf_sha256": hashlib.sha256(original).hexdigest()}
            manifest = {"schema_version": 1, "line_endings": "CRLF_to_LF_only",
                        "baseline_files": [entry], "development_shaders": []}
            path.write_bytes(b"original\r\n")
            self.assertEqual(verify(root, manifest), [])
            path.write_bytes(b"modified\n")
            self.assertIn("changed contents", verify(root, manifest)[0])
            path.unlink()
            self.assertIn("missing file", verify(root, manifest)[0])
            path.write_bytes(original)
            (folder / "extra.cpp").write_bytes(original)
            self.assertIn("unexpected baseline file", verify(root, manifest)[0])


if __name__ == "__main__":
    unittest.main()
