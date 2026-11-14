"""Verify the immutable B0 directory and the three locked development shaders.

Usage (Python 3.9+, standard library only, no Git executable required):
    python benchmarks/scripts/verify_baseline.py
    python benchmarks/scripts/verify_baseline.py --root . --manifest path/to/manifest.json

Paths default relative to this script, not the caller's working directory. Reads
the JSON lock and source bytes; writes nothing. Prints PASS/FAIL, exits 0/1.
Missing/extra files, changed contents, symlinks and malformed locks fail. Only
CRLF -> LF conversion is allowed for Windows checkouts; no whitespace stripping.
The manifest retains exact original Git blob SHA-1 and raw blob SHA-256 evidence.
"""

import argparse
import hashlib
import json
from pathlib import Path


def git_blob_sha1(data):
    """Compute Git's blob identity, including the type/byte-length header."""
    return hashlib.sha1(b"blob " + str(len(data)).encode("ascii") + b"\0" + data).hexdigest()


def verify(root, manifest):
    """Return errors; never update the lock to match changed source."""
    if manifest["schema_version"] != 1 or manifest["line_endings"] != "CRLF_to_LF_only":
        raise ValueError("unsupported manifest schema or line-ending policy")
    errors = []
    records = manifest["baseline_files"] + manifest["development_shaders"]
    expected = {entry["path"] for entry in manifest["baseline_files"]}
    if len({entry["path"] for entry in records}) != len(records) or not expected:
        raise ValueError("empty or duplicate lock entries")
    actual = {p.relative_to(root).as_posix() for p in (root / "src/baseline").rglob("*")
              if p.is_file() or p.is_symlink()}
    for path in sorted(actual - expected):
        errors.append(f"unexpected baseline file: {path}")
    for entry in records:
        relative = Path(entry["path"])
        if relative.is_absolute() or ".." in relative.parts:
            raise ValueError("lock paths must stay inside the repository")
        path = root / relative
        if path.is_symlink() or not path.is_file():
            errors.append(f"missing file or symlink: {entry['path']}")
            continue
        data = path.read_bytes().replace(b"\r\n", b"\n")
        if (git_blob_sha1(data) != entry["lf_git_blob_sha1"] or
                hashlib.sha256(data).hexdigest() != entry["lf_sha256"]):
            errors.append(f"changed contents: {entry['path']}")
    return errors


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument("--manifest", type=Path)
    args = parser.parse_args()
    root = args.root.resolve()
    manifest_path = args.manifest or root / "benchmarks/baseline_manifest.json"
    try:
        manifest = json.loads(manifest_path.read_text(encoding="utf-8-sig"))
        errors = verify(root, manifest)
    except (OSError, ValueError, KeyError, TypeError) as error:
        errors = [str(error)]
    if errors:
        print("FAIL: baseline integrity")
        for error in errors:
            print(f"  {error}")
        return 1
    print("PASS: all 7 baseline files and 3 development shaders match the locked blobs")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
