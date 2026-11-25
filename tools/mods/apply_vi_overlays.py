#!/usr/bin/env python3
"""Apply Vietnamese text-only overlays to four pinned CDDA mods.

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
    "arcana": {
        "folder": "Arcana_StrangeAeons", "id": "Arcana",
        "revision": "080d8e9e454d2179bc98eb07de075f74e58bcbd1",
        "file": "translations/arcana_vi_overlay.b64",
        "sha256": "a2fedcba612d63fd78fbb91b99d9121ff36e6e4dcf5ee9c550b33c4740bd3ac9",
        "max_bytes": 1_500_000, "max_files": 339,
        "source_blobs": "translations/arcana_upstream_blob_shas.json",
        "expected_strings": 6794,
    },
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
TOKEN = re.compile(r"(?<![0-9])%(?:\d+\$)?[+\-#0 ]*(?:\d+)?(?:\.\d+)?[sdif](?![A-Za-z])|<[A-Za-z_/][^>]*>")


def load_overlay(kind):
    info = OVERLAYS[kind]
    content = (HERE / info["file"]).read_text(encoding="ascii").strip()
    if hashlib.sha256(content.encode("ascii")).hexdigest() != info["sha256"]:
        raise ValueError("Overlay checksum mismatch: " + kind)
    compressed = base64.b64decode(content, validate=True)
    decoder = lzma.LZMADecompressor()
    limit = info.get("max_bytes", 250000)
    decoded = decoder.decompress(compressed, max_length=limit + 1)
    if len(decoded) > limit or not decoder.eof or decoder.unused_data:
        raise ValueError("Invalid or oversized translation overlay: " + kind)
    doc = json.loads(decoded)
    if doc["format"] != 1 or doc["mod"] != kind or doc["upstream_revision"] != info["revision"]:
        raise ValueError("Translation source revision mismatch: " + kind)
    if not isinstance(doc["files"], dict) or len(doc["files"]) > info.get("max_files", 100):
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


# Effect messages are [display text, engine message type] pairs.
# Their second member is not a translatable string.
def is_effect_message_type_pointer(pointer):
    return re.search(r"/(?:decay_messages|apply_message)/\d+/1$", pointer) is not None


def allowed_arcana_pointer(name, pointer):
    """Prevent accidentally translating IDs, map layouts or mod logic."""
    if is_effect_message_type_pointer(pointer):
        return False
    raw_parts = pointer.lstrip("/").split("/")
    parts = [x.replace("~1", "/").replace("~0", "~") for x in raw_parts]
    labels = [x for x in parts if not x.isdigit()]
    if not labels:
        return False
    leaf = labels[-1]
    if leaf in {"id", "type", "path", "copy-from", "dependencies",
                "ctxt", "//", "rows", "symbol", "category", "material",
                "target", "effect", "group", "item", "ammo", "skill"}:
        return False
    if leaf == "sound":
        return name == "monsters/speech.json"
    if leaf in {"pull", "open", "close", "fail"}:
        return name == "furniture_and_terrain/gates.json"
    if "dialogue" in labels and leaf in {
        "offer", "accepted", "rejected", "advice",
        "inquire", "success", "success_lie", "failure",
    }:
        return True
    allowed = {
        "name", "str", "str_pl", "str_sp", "description", "desc", "text",
        "dynamic_line", "msg", "message", "done_message", "apply_message",
        "remove_message", "u_message", "npc_message", "friendly_msg",
        "hostile_msg", "sound_msg", "sound_message", "use_message",
        "warning", "signage", "prompt", "revert_msg", "need_fire_msg",
        "need_charges_msg", "need_charge_msg", "not_ready_msg",
        "menu_text", "success_message", "failure_message",
        "failure_explanation", "snippet", "snippet_category", "label",
        "title", "greeting", "gendered_line", "description_short",
        "name_suffix", "mutagen_message", "memorial_message", "info",
        "initiate", "decay_messages", "messages", "miss_messages",
        "job_description", "start_name", "bash_dmg_verb", "cut_dmg_verb",
        "targeting_sound",
    }
    return (any(x in allowed or x.startswith(("msg_", "message_")) or
                x.endswith(("_msg", "_message")) for x in labels)
            or (len(parts) >= 3 and parts[-3] == "modes" and
                parts[-1] == "1"))


def apply(kind, data_mods, dry_run=False):
    info = OVERLAYS[kind]
    folder = (Path(data_mods) / info["folder"]).resolve()
    metadata = json.loads((folder / "modinfo.json").read_text(encoding="utf-8-sig"))
    entries = metadata if isinstance(metadata, list) else [metadata]
    if sum(isinstance(x, dict) and x.get("type") == "MOD_INFO"
           and x.get("id") == info["id"] for x in entries) != 1:
        raise ValueError("Unexpected CDDA mod ID: " + kind)
    original_blobs = None
    if info.get("source_blobs"):
        pin = json.loads((HERE / info["source_blobs"]).read_text(encoding="utf-8"))
        if pin["revision"] != info["revision"] or len(pin["files"]) != 339:
            raise ValueError("Arcana source blob manifest is inconsistent")
        original_blobs = pin["files"]
    prepared = []
    strings = 0
    skipped_types = 0
    for name, changes in sorted(load_overlay(kind).items()):
        relative = Path(name)
        if (not name.endswith(".json") or relative.is_absolute()
                or "\\" in name or ".." in relative.parts):
            raise ValueError("Unsafe overlay path")
        path = (folder / relative).resolve()
        if folder not in path.parents or not path.is_file():
            raise FileNotFoundError("Original JSON missing: " + name)
        raw = path.read_bytes()
        if original_blobs is not None:
            sha = hashlib.sha1(b"blob " + str(len(raw)).encode("ascii")
                               + b"\x00" + raw).hexdigest()
            if original_blobs.get(name) != sha:
                raise ValueError("Arcana upstream JSON does not match pinned revision: " + name)
        obj = json.loads(raw.decode("utf-8-sig"))
        seen_pointers = set()
        for pointer, translated in changes:
            if pointer in seen_pointers:
                raise ValueError("Repeated translation pointer: " + name + pointer)
            seen_pointers.add(pointer)
            if not isinstance(translated, str) or len(translated) > 8192:
                raise ValueError("Invalid localized text: " + pointer)
            parent, key = target_for_pointer(obj, pointer)
            old = parent[key]
            if not isinstance(old, str):
                raise ValueError("Non-string original: " + name + pointer)
            # Older translation inputs may include bad/good/neutral/mixed
            # in a tuple or effect rating. Keep the exact pinned source
            # value, not a guessed Vietnamese-to-English mapping.
            if is_effect_message_type_pointer(pointer) or (
                    isinstance(parent, dict) and key == "rating"
                    and parent.get("type") == "effect_type"):
                if old not in {"good", "neutral", "bad", "mixed"}:
                    raise ValueError("Invalid upstream effect message type: " + name + pointer)
                skipped_types += 1
                continue
            if kind == "arcana" and not allowed_arcana_pointer(name, pointer):
                raise ValueError("Arcana overlay attempted to modify a technical field: " + name + pointer)
            old_tokens = TOKEN.findall(old)
            new_tokens = TOKEN.findall(translated)
            if Counter(old_tokens) != Counter(new_tokens):
                raise ValueError("Broken placeholder: " + name + pointer)
            old_unnumbered = [t for t in old_tokens if t.startswith("%") and "$" not in t]
            new_unnumbered = [t for t in new_tokens if t.startswith("%") and "$" not in t]
            if old_unnumbered != new_unnumbered:
                raise ValueError("Reordered unnumbered placeholder: " + name + pointer)
            parent[key] = translated
            strings += 1
        prepared.append((path, json.dumps(obj, ensure_ascii=False, indent=2) + "\n"))
    if (info.get("expected_strings") is not None
            and strings + skipped_types != info["expected_strings"]):
        raise ValueError("Incorrect number of Arcana translation entries")
    if skipped_types:
        print(f"{kind}: preserved {skipped_types} original effect message types")
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
