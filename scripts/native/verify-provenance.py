"""Check owned C++ source provenance; upstream code keeps its own attribution."""
from pathlib import Path
import json
import re
import sys

root = Path(__file__).resolve().parents[2]
records = {}
for path in (root / "docs/ai/changes").glob("*.json"):
    record = json.loads(path.read_text(encoding="utf-8"))
    for key in ("change_id", "started_at_utc", "provider", "model", "identity_basis"):
        if not record.get(key):
            raise SystemExit(f"Missing {key}: {path.relative_to(root)}")
    records[record["change_id"]] = record
errors = []
for path in (root / "native").rglob("*"):
    if path.suffix not in (".cpp", ".h") or "third_party" in path.parts:
        continue
    text = path.read_text(encoding="utf-8")
    match = re.search(r"AI-Change:\s*(\S+)", text[:1200])
    if "SPDX-License-Identifier: MIT" not in text[:1200] or not match or match[1] not in records:
        errors.append(str(path.relative_to(root)))
if errors:
    print("Missing/invalid provenance:", *errors, sep="\n")
    sys.exit(1)
print(f"Provenance links verified against {len(records)} change record(s). Self-reported attribution is not cryptographic proof.")
