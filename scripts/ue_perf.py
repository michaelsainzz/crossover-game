"""Automated 3v3 match performance baseline runner and validator for Crossover."""

import argparse
import json
import os
from pathlib import Path
import platform
import re
import shutil
import subprocess
import sys


ROOT = Path(__file__).resolve().parents[1]
PERF_READY_MARKER = re.compile(r"CROSSOVER_MATCH_PERF_READY(?::\s*Report saved to\s*(.+))?")
PERF_SUMMARY_MARKER = re.compile(r"CROSSOVER_MATCH_PERF_SUMMARY:\s*(.+)")


def validate_perf_report(report_path: Path, max_median: float = 16.7, max_p95: float = 22.0) -> dict:
    if not report_path.is_file():
        raise FileNotFoundError(f"Missing performance report: {report_path}")
    try:
        data = json.loads(report_path.read_text(encoding="utf-8"))
    except json.JSONDecodeError as error:
        raise ValueError(f"Invalid performance report JSON: {error}") from error

    required_keys = ("samples", "min_ms", "max_ms", "avg_ms", "median_ms", "p95_ms", "p99_ms", "passed")
    for key in required_keys:
        if key not in data:
            raise ValueError(f"Performance report missing key: {key}")

    samples = data["samples"]
    if not isinstance(samples, int) or samples <= 0:
        raise ValueError(f"Expected positive sample count, got: {samples}")

    median = float(data["median_ms"])
    p95 = float(data["p95_ms"])
    passed = bool(data["passed"])

    if median > max_median:
        raise ValueError(f"Median frame time {median:.2f} ms exceeds target {max_median:.2f} ms")
    if p95 > max_p95:
        raise ValueError(f"95th percentile frame time {p95:.2f} ms exceeds target {max_p95:.2f} ms")
    if not passed:
        raise ValueError("Performance report flagged failed criteria")

    return data


def find_latest_packaged_app(runs_dir: Path) -> Path:
    if not runs_dir.is_dir():
        raise FileNotFoundError(f"Runs directory does not exist: {runs_dir}")
    candidates = sorted(runs_dir.glob("run-*/archive/Crossover.app"), key=lambda p: p.stat().st_mtime, reverse=True)
    if not candidates:
        raise FileNotFoundError(f"No packaged Crossover.app found in {runs_dir}")
    return candidates[0]


def get_app_binary(app_path: Path) -> Path:
    binary = app_path / "Contents" / "MacOS" / app_path.stem
    if not binary.is_file():
        raise ValueError(f"Packaged executable missing: {binary}")
    return binary


def locate_generated_report(log_text: str, project_dir: Path) -> Path:
    match = PERF_READY_MARKER.search(log_text)
    if match and match.group(1):
        candidate = Path(match.group(1).strip())
        if candidate.is_file():
            return candidate

    standard_path = project_dir / "Saved" / "Profiling" / "perf-report.json"
    if standard_path.is_file():
        return standard_path

    # Check sandboxed macOS App Container directory
    home = Path.home()
    container_pattern = "Library/Containers/*/Data/Library/Application Support/Epic/Crossover/Saved/Profiling/perf-report.json"
    container_candidates = sorted(home.glob(container_pattern), key=lambda p: p.stat().st_mtime, reverse=True)
    if container_candidates and container_candidates[0].is_file():
        return container_candidates[0]

    raise FileNotFoundError(f"Missing performance report at {standard_path} or sandbox container path")


def run_perf_benchmark(
    app_binary: Path,
    output_dir: Path,
    samples: int = 720,
    warmup: int = 120,
    null_rhi: bool = False,
    timeout: int = 180,
) -> dict:
    output_dir.mkdir(parents=True, exist_ok=True)
    log_file = output_dir / "perf-launch.log"

    cmd = [
        str(app_binary),
        "-CrossoverMatchPerf",
        "-Unattended",
        "-NoSound",
        f"-PerfWarmup={warmup}",
        f"-PerfSamples={samples}",
    ]
    if null_rhi:
        cmd.append("-NullRHI")

    print(f"Launching performance benchmark: {' '.join(cmd)}", flush=True)
    with log_file.open("wb") as stream:
        process = subprocess.run(cmd, stdout=stream, stderr=subprocess.STDOUT, timeout=timeout)
        if process.returncode != 0:
            raise ValueError(f"Performance run exited with status {process.returncode}. See {log_file}")

    log_text = log_file.read_text(encoding="utf-8-sig", errors="replace")
    if not PERF_READY_MARKER.search(log_text):
        raise ValueError(f"Process completed but did not log CROSSOVER_MATCH_PERF_READY marker. See {log_file}")

    source_report = locate_generated_report(log_text, ROOT)
    target_report = output_dir / "perf-report.json"
    if source_report != target_report:
        shutil.copy2(source_report, target_report)

    data = validate_perf_report(target_report)
    return data


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--app", type=Path, help="Path to packaged Crossover.app")
    parser.add_argument("--check-report", type=Path, help="Validate an existing perf-report.json only")
    parser.add_argument("--samples", type=int, default=720, help="Number of active match frames to sample")
    parser.add_argument("--warmup", type=int, default=120, help="Warmup frames before sampling")
    parser.add_argument("--null-rhi", action="store_true", help="Run with -NullRHI (headless fallback)")
    parser.add_argument("--timeout", type=int, default=180, help="Maximum run seconds")
    args = parser.parse_args()

    try:
        if args.check_report:
            data = validate_perf_report(args.check_report)
            print(f"Performance report valid: {data}")
            return 0

        runs_dir = ROOT / "Saved" / "MacVerification"
        app_path = args.app or find_latest_packaged_app(runs_dir)
        binary = get_app_binary(app_path)
        profiling_dir = ROOT / "Saved" / "Profiling"
        data = run_perf_benchmark(
            binary,
            profiling_dir,
            samples=args.samples,
            warmup=args.warmup,
            null_rhi=args.null_rhi,
            timeout=args.timeout,
        )
        print("Performance baseline check PASSED:")
        print(f"  Samples: {data['samples']}")
        print(f"  Min:     {data['min_ms']:.2f} ms")
        print(f"  Max:     {data['max_ms']:.2f} ms")
        print(f"  Avg:     {data['avg_ms']:.2f} ms")
        print(f"  Median:  {data['median_ms']:.2f} ms (target <= 16.70 ms)")
        print(f"  P95:     {data['p95_ms']:.2f} ms (target <= 22.00 ms)")
        print(f"  P99:     {data['p99_ms']:.2f} ms")
        return 0
    except (OSError, ValueError) as error:
        print(f"Performance benchmark failed: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
