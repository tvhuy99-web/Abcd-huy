#!/usr/bin/env python3
"""Compile a gettext catalog, accepting plain or gzip-compressed PO input."""

import gzip
import subprocess
import sys
from pathlib import Path


def main() -> int:
    if len(sys.argv) not in (3, 4):
        print("Usage: compile_mo.py INPUT.po[.gz] OUTPUT.mo [MSGFMT]", file=sys.stderr)
        return 2

    source = Path(sys.argv[1])
    output = Path(sys.argv[2])
    msgfmt = sys.argv[3] if len(sys.argv) == 4 else "msgfmt"
    opener = gzip.open if source.suffix == ".gz" else open

    output.parent.mkdir(parents=True, exist_ok=True)
    with opener(source, "rb") as catalog:
        subprocess.run([msgfmt, "-f", "-o", str(output), "-"],
                       input=catalog.read(), check=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
