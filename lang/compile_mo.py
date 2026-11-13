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
        content = catalog.read()
    # Small, reviewed accessibility additions stay in plaintext while the
    # large upstream Vietnamese catalog remains compressed. Merge by gettext
    # identity, not by replacing English words at runtime.
    language = source.name.split(".", 1)[0]
    overlay = source.parent / "accessibility" / (language + ".po")
    if overlay.is_file():
        executable = Path(msgfmt)
        msgcat_name = "msgcat.exe" if executable.suffix == ".exe" else "msgcat"
        msgcat = str(executable.with_name(msgcat_name)) if executable.parent != Path(".") else msgcat_name
        try:
            content = subprocess.run([msgcat, "--use-first", str(overlay), "-"],
                                     input=content, stdout=subprocess.PIPE, check=True).stdout
        except FileNotFoundError:
            # A missing msgcat used to surface only as a bare traceback, which
            # hid that the accessibility overlay (vehicle strings included)
            # could not be merged -- and once cost a release CI run (CI-REL).
            # Fail loudly with the cause and the remedy instead.
            print(f"Error: '{msgcat}' not found. Install gettext (msgcat) so the "
                  f"accessibility overlay '{overlay}' can be merged into "
                  f"'{output}'. Without it the build cannot produce a "
                  "correct .mo file.", file=sys.stderr)
            return 1
    subprocess.run([msgfmt, "-f", "-o", str(output), "-"],
                   input=content, check=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
