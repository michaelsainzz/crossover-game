"""Regression checks for the Mac verification gate without an Unreal install."""

import copy
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

import ue_mac


PASSING_REPORT = {
    "succeeded": 1,
    "succeededWithWarnings": 0,
    "failed": 0,
    "notRun": 0,
    "inProcess": 0,
    "tests": [{"fullTestPath": "Crossover.Smoke", "state": "Success", "errors": 0, "warnings": 0}],
}


class MacVerificationTest(unittest.TestCase):
    def test_accepts_exact_passing_smoke(self):
        with tempfile.TemporaryDirectory() as folder:
            report = Path(folder) / "index.json"
            report.write_text(json.dumps(PASSING_REPORT), encoding="utf-8")
            ue_mac.validate_report(report)

    def test_rejects_absent_or_incomplete_report(self):
        with tempfile.TemporaryDirectory() as folder:
            report = Path(folder) / "index.json"
            with self.assertRaises(FileNotFoundError):
                ue_mac.validate_report(report)
            for contents in ("{", "[]", "{}"):
                report.write_text(contents, encoding="utf-8")
                with self.subTest(contents=contents), self.assertRaises(ValueError):
                    ue_mac.validate_report(report)

    def test_rejects_failed_or_unexpected_smoke(self):
        variants = [
            {**PASSING_REPORT, "succeeded": 0, "tests": []},
            {**PASSING_REPORT, "failed": 1},
            {**PASSING_REPORT, "notRun": 1},
            {**PASSING_REPORT, "inProcess": 1},
            {**PASSING_REPORT, "succeeded": True},
            {**PASSING_REPORT, "tests": PASSING_REPORT["tests"] * 2},
        ]
        for name, value in (
            ("fullTestPath", "Crossover.Smoke.Unrelated"),
            ("state", "Fail"),
            ("state", "Skipped"),
            ("errors", 1),
            ("warnings", 1),
        ):
            report = copy.deepcopy(PASSING_REPORT)
            report["tests"][0][name] = value
            variants.append(report)

        with tempfile.TemporaryDirectory() as folder:
            report = Path(folder) / "index.json"
            for variant in variants:
                report.write_text(json.dumps(variant), encoding="utf-8")
                with self.subTest(report=variant), self.assertRaises(ValueError):
                    ue_mac.validate_report(report)

    def test_requires_installed_engine(self):
        with tempfile.TemporaryDirectory() as folder, patch.object(ue_mac.platform, "system", return_value="Darwin"):
            with self.assertRaisesRegex(ValueError, "Set UE_ROOT"):
                ue_mac.verify(None, 1)
            with self.assertRaisesRegex(ValueError, "Build.sh"):
                ue_mac.verify(folder, 1)

    def test_timeout_and_nonzero_exit_are_failures(self):
        with tempfile.TemporaryDirectory() as folder:
            logs = Path(folder)
            with self.assertRaisesRegex(ValueError, "status 7"):
                ue_mac.run_step([sys.executable, "-c", "raise SystemExit(7)"], logs / "exit.log", 10)
            with self.assertRaisesRegex(ValueError, "Timed out"):
                ue_mac.run_step([sys.executable, "-c", "import time; time.sleep(10)"], logs / "timeout.log", 0.1)


if __name__ == "__main__":
    unittest.main()
