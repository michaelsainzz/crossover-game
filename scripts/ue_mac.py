"""Build, test, package, and smoke-launch Crossover with an installed Mac Unreal Engine."""

import argparse
import json
import os
from pathlib import Path
import platform
import plistlib
import re
import shlex
import signal
import subprocess
import sys
import tempfile


ROOT = Path(__file__).resolve().parents[1]
TEST = "Crossover.Smoke"
READY_MARKER = re.compile(r"(?m)(?:^|: )CROSSOVER_SMOKE_READY[ \t\r]*$")


def validate_report(path):
    report = json.loads(path.read_text(encoding="utf-8-sig"))
    if not isinstance(report, dict):
        raise ValueError("Automation report must be a JSON object.")
    expected_counts = {
        "succeeded": 1,
        "succeededWithWarnings": 0,
        "failed": 0,
        "notRun": 0,
        "inProcess": 0,
    }
    for name, expected in expected_counts.items():
        count = report.get(name)
        if type(count) is not int or count != expected:
            raise ValueError(f"Automation report {name}={count!r}, expected {expected}.")

    tests = report.get("tests")
    if not isinstance(tests, list) or len(tests) != 1 or not isinstance(tests[0], dict):
        raise ValueError("Automation report must contain exactly one test result.")
    result = tests[0]
    for name, expected in (("fullTestPath", TEST), ("state", "Success"), ("errors", 0), ("warnings", 0)):
        value = result.get(name)
        if type(value) is not type(expected) or value != expected:
            raise ValueError(f"Automation test {name}={value!r}, expected {expected!r}.")
    print(f"Verified {TEST}: one successful test with no warnings or errors.")


def run_step(command, log, timeout):
    command = [str(argument) for argument in command]
    print(f"Running {shlex.join(command)}\nLog: {log}", flush=True)
    with log.open("x", encoding="utf-8") as output:
        output.write(f"$ {shlex.join(command)}\n")
        output.flush()
        with subprocess.Popen(
            command, cwd=ROOT, stdin=subprocess.DEVNULL, stdout=output,
            stderr=subprocess.STDOUT, start_new_session=True,
        ) as process:
            try:
                status = process.wait(timeout=timeout)
            except (subprocess.TimeoutExpired, KeyboardInterrupt) as error:
                # UAT and build scripts start child processes of their own.
                try:
                    os.killpg(process.pid, signal.SIGKILL)
                except ProcessLookupError:
                    pass
                if isinstance(error, KeyboardInterrupt):
                    raise
                raise ValueError(f"Timed out after {timeout}s. See {log}.") from error
        output.write(f"\nExit status: {status}\n")
    if status:
        raise ValueError(f"Exited with status {status}. See {log}.")


def executable(path):
    if not path.is_file() or not os.access(path, os.X_OK):
        raise ValueError(f"Executable missing or unavailable: {path}")
    return path


def packaged_executable(archive):
    bundles = sorted(bundle for bundle in archive.rglob("Crossover.app") if bundle.is_dir())
    if len(bundles) != 1:
        raise ValueError(f"Expected one Crossover.app in {archive}, found {len(bundles)}.")
    contents = bundles[0] / "Contents"
    with (contents / "Info.plist").open("rb") as plist:
        metadata = plistlib.load(plist)
    name = metadata.get("CFBundleExecutable") if isinstance(metadata, dict) else None
    if not isinstance(name, str) or Path(name).name != name or name in ("", ".", ".."):
        raise ValueError("Crossover.app has no valid CFBundleExecutable.")
    return executable(contents / "MacOS" / name)


def verify(ue_root, timeout):
    if platform.system() != "Darwin":
        raise ValueError("Mac verification needs macOS. Use --check-report for report-only validation.")
    if not ue_root:
        raise ValueError("Set UE_ROOT or pass --ue-root to the Unreal installation containing Engine/.")
    engine = Path(ue_root).expanduser().resolve() / "Engine"
    batch = engine / "Build" / "BatchFiles"
    build = executable(batch / "Mac" / "Build.sh")
    editor = executable(engine / "Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor")
    uat = executable(batch / "RunUAT.sh")
    project = ROOT / "Crossover.uproject"
    if not project.is_file():
        raise ValueError(f"Missing Unreal project: {project}")

    runs = ROOT / "Saved" / "MacVerification"
    runs.mkdir(parents=True, exist_ok=True)
    artifacts = Path(tempfile.mkdtemp(prefix="run-", dir=runs))
    print(f"Evidence directory: {artifacts}", flush=True)
    version = (engine / "Build" / "Build.version").read_text(encoding="utf-8")
    (artifacts / "engine-version.json").write_text(version, encoding="utf-8")
    if (ROOT / ".git").exists():
        run_step(["git", "log", "-1", "--format=%H"], artifacts / "commit.log", 30)
        run_step(["git", "status", "--short"], artifacts / "checkout.log", 30)
    run_step(["xcodebuild", "-version"], artifacts / "xcode.log", 30)
    run_step(["xcrun", "--sdk", "macosx", "--show-sdk-version"], artifacts / "sdk.log", 30)

    run_step([build, "CrossoverEditor", "Mac", "Development", f"-Project={project}"],
             artifacts / "build.log", timeout)

    report = artifacts / "report"
    run_step([
        editor, project, "-Unattended", "-NullRHI", "-NoSound", "-NoSplash",
        f"-ExecCmds=Automation RunTest {TEST};Quit", f"-ReportExportPath={report}",
        f"-AbsLog={artifacts / 'editor.log'}",
    ], artifacts / "automation.log", timeout)
    validate_report(report / "index.json")

    run_step(["xcrun", "--sdk", "macosx", "metal", "--version"], artifacts / "metal.log", 30)
    archive = artifacts / "archive"
    run_step([
        uat, "BuildCookRun", f"-project={project}", "-platform=Mac", "-clientconfig=Development",
        f"-clientarchitecture={platform.machine()}", "-unattended", "-nop4", "-build", "-cook", "-stage",
        "-pak", "-package", "-archive", f"-archivedirectory={archive}",
    ], artifacts / "package.log", timeout)

    game_log = artifacts / "game.log"
    run_step([
        packaged_executable(archive), "-CrossoverSmoke", "-Unattended", "-NullRHI",
        "-NoSound", f"-AbsLog={game_log}",
    ], artifacts / "launch.log", min(timeout, 120))
    if not READY_MARKER.search(game_log.read_text(encoding="utf-8-sig", errors="replace")):
        raise ValueError(f"Packaged game did not report sandbox readiness. See {game_log}.")
    print(f"Mac build, automation, package, and headless launch passed. Evidence: {artifacts}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--ue-root", default=os.environ.get("UE_ROOT"), help="Directory containing Engine/")
    parser.add_argument("--step-timeout", type=int, default=1800, help="Maximum seconds per Unreal step")
    parser.add_argument("--check-report", type=Path, help="Validate an existing Unreal index.json only")
    args = parser.parse_args()
    if args.step_timeout <= 0:
        parser.error("--step-timeout must be positive")
    try:
        if args.check_report:
            validate_report(args.check_report)
        else:
            verify(args.ue_root, args.step_timeout)
    except (OSError, ValueError, plistlib.InvalidFileException) as error:
        print(f"Mac verification failed: {error}", file=sys.stderr)
        return 1
    except KeyboardInterrupt:
        return 130
    return 0


if __name__ == "__main__":
    sys.exit(main())
