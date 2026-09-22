# SPDX-License-Identifier: MIT
# AI-Change: 2026-09-22-resume-native (OpenAI / GPT-6 Astra Pro)
"""Verify a Windows preview manifest and run isolated clean-PATH startup smokes.

Python is a development/test dependency only. No model or network calls occur.
All generated evidence is additive under artifacts/; existing profiles are untouched.
"""
import argparse
import ctypes
import hashlib
import json
import os
from pathlib import Path
import statistics
import subprocess
import tempfile


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("package", type=Path)
    parser.add_argument("--runs", type=int, default=5)
    args = parser.parse_args()
    if os.name != "nt" or not 1 <= args.runs <= 20:
        parser.error("Windows only; runs must be between 1 and 20")
    package = args.package.resolve(strict=True)
    manifest = json.loads((package / "package-manifest.json").read_text(encoding="utf-8-sig"))
    for entry in manifest["files"]:
        path = (package / entry["path"]).resolve(strict=True)
        if not path.is_relative_to(package):
            raise RuntimeError("Manifest path escapes package")
        with path.open("rb") as stream:
            digest = hashlib.file_digest(stream, "sha256").hexdigest()
        if digest != entry["sha256"] or path.stat().st_size != entry["bytes"]:
            raise RuntimeError("Manifest mismatch: " + entry["path"])
    artifacts = Path(__file__).resolve().parents[2] / "artifacts"
    artifacts.mkdir(exist_ok=True)
    evidence = Path(tempfile.mkdtemp(prefix="package-smoke-", dir=artifacts))
    env = os.environ.copy()
    env["PATH"] = os.environ["SystemRoot"] + "\\System32;" + os.environ["SystemRoot"]
    for key in ("QT_PLUGIN_PATH", "QTDIR", "QT_QPA_PLATFORM_PLUGIN_PATH"):
        env.pop(key, None)
    ctypes.windll.kernel32.SetErrorMode(0x0001 | 0x8000)
    samples = []
    for index in range(args.runs):
        output = evidence / str(index)
        output.mkdir()
        result = subprocess.run(
            [str(package / "mterm.exe"), "--workspace", str(output),
             "--data-dir", str(output / "profile"), "--smoke-dir", str(output / "smoke")],
            cwd=package, env=env, capture_output=True, timeout=20, check=False,
        )
        (output / "stderr.txt").write_bytes(result.stderr)
        if result.returncode:
            raise RuntimeError(f"Standalone app exited {result.returncode}; evidence: {output}")
        record = json.loads((output / "smoke/native-smoke.json").read_text())
        if not (output / "smoke/native-window.png").is_file():
            raise RuntimeError("Screenshot missing")
        samples.append(record)
    summary = {
        "runs": len(samples), "packageBytes": manifest["totalBytes"],
        "firstPaintMedianMs": statistics.median(s["firstPaintMs"] for s in samples),
        "workspaceReadyMedianMs": statistics.median(s["workspaceReadyMs"] for s in samples),
        "workingSetMedianBytes": statistics.median(s["processFootprint"]["workingSetBytes"] for s in samples),
        "privateMedianBytes": statistics.median(s["processFootprint"]["privateBytes"] for s in samples),
        "method": "Inside-main timers, empty isolated workspaces, no development Qt on PATH; not a CLI or cold-start comparison",
    }
    (evidence / "samples.json").write_text(json.dumps({"summary": summary, "samples": samples}, indent=2), encoding="utf-8")
    print(json.dumps(summary, indent=2))
    print("Evidence:", evidence)


if __name__ == "__main__":
    main()
