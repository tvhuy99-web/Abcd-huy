#!/usr/bin/env python3
"""Apply Vietnamese text-only overlays to three pinned CDDA mods.

Only string values change. Game logic, identifiers, arrays and numeric
fields always come from the original pinned upstream revision.
"""
import argparse
import base64
from collections import Counter
import hashlib
import json
import lzma
from pathlib import Path
import re

HERE = Path(__file__).resolve().parent
OVERLAYS = {
    "cataplusplus": {
        "folder": "CataclysmPlusPlus", "id": "Cata++",
        "revision": "46982a63da25b064c8d0fdee9b68c23dccc1f671",
        "file": "translations/cataplusplus_vi_overlay.b64",
        "sha256": "59da33109f3acab10694c960ee2772ee1c18ce3d71fa3100c1a0a97f81422603",
    },
    "mst_extra": {
        "folder": "MST_Extra", "id": "MST_Extra",
        "revision": "5f9dbfef550d5c140865ce5f6a32c40eb34c07ca",
        "file": "translations/mst_extra_vi_overlay.b64",
        "sha256": "9ac7e72cb5ec710dcb92228cc1c4903098c69eeb5a0a8465c7d4d629bc662c68",
    },
    "blazemod": {
        "folder": "Blazemod", "id": "blazemod",
        "revision": "14530cc7ebfaac6c85586e7d7728635f8fda7d6d",
        "file": "translations/blazemod_vi_overlay.b64",
        "sha256": "d7837131504a9112c165efba75df6518f69a685d88d232b57a6d62cfb133f6c6",
    },
}
TOKEN = re.compile(r"%(?:\d+\$)?[+\-#0 ]*(?:\d+)?(?:\.\d+)?[sdif]|<[A-Za-z_/][^>]*>")


def load_overlay(kind):
    info = OVERLAYS[kind]
    content = (HERE / info["file"]).read_text(encoding="ascii").strip()
    if hashlib.sha256(content.encode("ascii")).hexdigest() != info["sha256"]:
        raise ValueError("Overlay checksum mismatch: " + kind)
    compressed = base64.b64decode(content, validate=True)
    decoder = lzma.LZMADecompressor()
    decoded = decoder.decompress(compressed, max_length=250001)
    if len(decoded) > 250000 or not decoder.eof or decoder.unused_data:
        raise ValueError("Invalid or oversized translation overlay: " + kind)
    doc = json.loads(decoded)
    if doc["format"] != 1 or doc["mod"] != kind or doc["upstream_revision"] != info["revision"]:
        raise ValueError("Translation source revision mismatch: " + kind)
    if not isinstance(doc["files"], dict) or len(doc["files"]) > 100:
        raise ValueError("Invalid overlay manifest")
    return doc["files"]


def target_for_pointer(obj, pointer):
    if not pointer.startswith("/") or len(pointer) > 200:
        raise ValueError("Invalid translation pointer")
    parts = [p.replace("~1", "/").replace("~0", "~") for p in pointer[1:].split("/")]
    if parts[-1] in {"id", "type", "copy-from", "path", "dependencies", "ctxt"}:
        raise ValueError("Refusing to change technical field: " + pointer)
    parent = obj
    for key in parts[:-1]:
        parent = parent[int(key)] if isinstance(parent, list) else parent[key]
    key = int(parts[-1]) if isinstance(parent, list) else parts[-1]
    return parent, key


def apply(kind, data_mods, dry_run=False):
    info = OVERLAYS[kind]
    folder = (Path(data_mods) / info["folder"]).resolve()
    metadata = json.loads((folder / "modinfo.json").read_text(encoding="utf-8-sig"))
    entries = metadata if isinstance(metadata, list) else [metadata]
    if sum(isinstance(x, dict) and x.get("type") == "MOD_INFO"
           and x.get("id") == info["id"] for x in entries) != 1:
        raise ValueError("Unexpected CDDA mod ID: " + kind)
    prepared = []
    strings = 0
    for name, changes in sorted(load_overlay(kind).items()):
        relative = Path(name)
        if (not name.endswith(".json") or relative.is_absolute()
                or "\\" in name or ".." in relative.parts):
            raise ValueError("Unsafe overlay path")
        path = (folder / relative).resolve()
        if folder not in path.parents or not path.is_file():
            raise FileNotFoundError("Original JSON missing: " + name)
        obj = json.loads(path.read_text(encoding="utf-8-sig"))
        for pointer, translated in changes:
            if not isinstance(translated, str) or len(translated) > 8192:
                raise ValueError("Invalid localized text: " + pointer)
            parent, key = target_for_pointer(obj, pointer)
            old = parent[key]
            if not isinstance(old, str):
                raise ValueError("Non-string original: " + name + pointer)
            if Counter(TOKEN.findall(old)) != Counter(TOKEN.findall(translated)):
                raise ValueError("Broken placeholder: " + name + pointer)
            parent[key] = translated
            strings += 1
        prepared.append((path, json.dumps(obj, ensure_ascii=False, indent=2) + "\n"))
    if not dry_run:
        for path, data in prepared:
            temporary = path.with_name(path.name + ".vi-tmp")
            try:
                temporary.write_text(data, encoding="utf-8")
                temporary.replace(path)
            finally:
                if temporary.exists():
                    temporary.unlink()
    return len(prepared), strings


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--target", type=Path, default=Path("data/mods"))
    parser.add_argument("--mods", nargs="+", choices=sorted(OVERLAYS), default=list(OVERLAYS))
    parser.add_argument("--dry-run", action="store_true")
    opts = parser.parse_args()
    for name in opts.mods:
        files, strings = apply(name, opts.target, opts.dry_run)
        print(f"{name}: {'validated' if opts.dry_run else 'localized'} {files} files / {strings} strings")


if __name__ == "__main__":
    main()
