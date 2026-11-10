"""Unit tests for the performance benchmark verifier without Unreal installed."""

import json
from pathlib import Path
import tempfile
import unittest

import ue_perf


VALID_REPORT = {
    "samples": 720,
    "min_ms": 11.234,
    "max_ms": 19.876,
    "avg_ms": 13.456,
    "median_ms": 13.200,
    "p95_ms": 16.500,
    "p99_ms": 18.200,
    "target_median_ms": 16.7,
    "target_p95_ms": 22.0,
    "passed": True,
}


class PerfReportValidationTest(unittest.TestCase):
    def test_accepts_valid_passing_report(self):
        with tempfile.TemporaryDirectory() as folder:
            report = Path(folder) / "perf-report.json"
            report.write_text(json.dumps(VALID_REPORT), encoding="utf-8")
            data = ue_perf.validate_perf_report(report)
            self.assertEqual(data["samples"], 720)
            self.assertTrue(data["passed"])

    def test_rejects_missing_file(self):
        with tempfile.TemporaryDirectory() as folder:
            report = Path(folder) / "perf-report.json"
            with self.assertRaises(FileNotFoundError):
                ue_perf.validate_perf_report(report)

    def test_rejects_malformed_json(self):
        with tempfile.TemporaryDirectory() as folder:
            report = Path(folder) / "perf-report.json"
            report.write_text("{ incomplete", encoding="utf-8")
            with self.assertRaises(ValueError):
                ue_perf.validate_perf_report(report)

    def test_rejects_missing_keys(self):
        for key in ("samples", "min_ms", "max_ms", "avg_ms", "median_ms", "p95_ms", "p99_ms", "passed"):
            with tempfile.TemporaryDirectory() as folder:
                report = Path(folder) / "perf-report.json"
                copy = dict(VALID_REPORT)
                del copy[key]
                report.write_text(json.dumps(copy), encoding="utf-8")
                with self.assertRaises(ValueError):
                    ue_perf.validate_perf_report(report)

    def test_rejects_median_exceeding_budget(self):
        with tempfile.TemporaryDirectory() as folder:
            report = Path(folder) / "perf-report.json"
            copy = dict(VALID_REPORT, median_ms=17.5)
            report.write_text(json.dumps(copy), encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "Median frame time 17.50 ms exceeds target"):
                ue_perf.validate_perf_report(report)

    def test_rejects_p95_exceeding_budget(self):
        with tempfile.TemporaryDirectory() as folder:
            report = Path(folder) / "perf-report.json"
            copy = dict(VALID_REPORT, p95_ms=23.5)
            report.write_text(json.dumps(copy), encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "95th percentile frame time 23.50 ms exceeds target"):
                ue_perf.validate_perf_report(report)

    def test_rejects_failed_flag(self):
        with tempfile.TemporaryDirectory() as folder:
            report = Path(folder) / "perf-report.json"
            copy = dict(VALID_REPORT, passed=False)
            report.write_text(json.dumps(copy), encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "flagged failed criteria"):
                ue_perf.validate_perf_report(report)

    def test_locates_report_from_log_marker(self):
        with tempfile.TemporaryDirectory() as folder:
            report_file = Path(folder) / "custom-perf-report.json"
            report_file.write_text(json.dumps(VALID_REPORT), encoding="utf-8")
            log = f"LogTemp: Display: CROSSOVER_MATCH_PERF_READY: Report saved to {report_file}\n"
            located = ue_perf.locate_generated_report(log, Path(folder))
            self.assertEqual(located, report_file)


if __name__ == "__main__":
    unittest.main()
