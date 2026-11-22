#!/usr/bin/env python3
"""Migrate pinned community item JSON after verifying/applying text overlays.

Only top-level item declarations are transformed. Do not alter item_group
subtypes, nested effect/recipe types, identifiers, translations or weights.
Never apply these rules to saves or the source archive used to verify overlays.
"""
import argparse
from copy import deepcopy
import json
from pathlib import Path
import tempfile

FOLDERS = ("CataclysmPlusPlus", "Arcana_StrangeAeons", "Blazemod", "MST_Extra")
SLOTS = frozenset((
    "ARMOR", "TOOL", "PET_ARMOR", "GUN", "GUNMOD", "AMMO", "MAGAZINE",
    "COMESTIBLE", "BOOK", "BIONIC_ITEM", "TOOLMOD", "ENGINE", "WHEEL",
    "SEED", "BREWABLE", "COMPOSTABLE", "MILLING", "ARTIFACT",
))
LEGACY_TYPES = {slot: (slot,) for slot in SLOTS}
LEGACY_TYPES.update({"GENERIC": (), "TOOL_ARMOR": ("TOOL", "ARMOR")})
ALCOHOL = frozenset(("ALCOHOL", "ALCOHOL_WEAK", "ALCOHOL_STRONG"))


def slot_list(value):
    values = [value] if isinstance(value, str) else value
    if not isinstance(values, list) or any(
            not isinstance(slot, str) or slot.upper() not in LEGACY_TYPES for slot in values):
        raise ValueError("Unknown item subtype: " + repr(value))
    # GENERIC is the old base item, not a runtime slot. TOOL_ARMOR
    # likewise represents two independent slots, not a slot of its own.
    # Pinned legacy mods also spell known slots in lowercase. Normalize
    # the enum spelling only; unknown slots still fail validation.
    return list(dict.fromkeys(slot for name in values for slot in LEGACY_TYPES[name.upper()]))


def rename_subtypes(obj):
    if "subtype" not in obj:
        return
    old = slot_list(obj["subtype"])
    if "subtypes" in obj and slot_list(obj["subtypes"]) != old:
        raise ValueError("Conflicting subtype and subtypes")
    obj["subtypes"] = old
    del obj["subtype"]


def add_alcohol_flag(obj, flag):
    # Extend inherited flags instead of replacing a parent's entire flag set.
    if "flags" in obj or "copy-from" not in obj:
        target = obj
    else:
        target = obj.setdefault("extend", {})
    if not isinstance(target, dict):
        raise ValueError("Invalid flags extension")
    flags = target.setdefault("flags", [])
    if not isinstance(flags, list):
        raise ValueError("Expected an array of item flags")
    if any(isinstance(f, str) and f in ALCOHOL - {flag} for f in flags):
        raise ValueError("Conflicting alcohol strengths")
    if flag not in flags:
        flags.append(flag)


def migrate_item(original):
    if not isinstance(original, dict):
        return original
    kind = original.get("type")
    if not isinstance(kind, str) or (kind != "ITEM" and kind not in LEGACY_TYPES):
        return original
    obj = deepcopy(original)
    rename_subtypes(obj)
    slots = slot_list(obj["subtypes"]) if "subtypes" in obj else []
    if kind != "ITEM":
        obj["type"] = "ITEM"
        for slot in LEGACY_TYPES[kind]:
            if slot not in slots:
                slots.append(slot)
    # Older GENERIC items could have an armor slot in a nested object.
    # The current deserializer reads those exact fields from the item itself.
    if "armor_data" in obj:
        armor = obj.pop("armor_data")
        if not isinstance(armor, dict):
            raise ValueError("Expected armor_data object")
        for key, value in armor.items():
            if key in obj and obj[key] != value:
                raise ValueError("Conflicting armor field: " + key)
            obj[key] = value
        if "ARMOR" not in slots:
            slots.append("ARMOR")
    if slots or "subtypes" in obj:
        obj["subtypes"] = slots
    for operation in ("extend", "delete"):
        if isinstance(obj.get(operation), dict):
            rename_subtypes(obj[operation])
            if "subtypes" in obj[operation]:
                obj[operation]["subtypes"] = slot_list(obj[operation]["subtypes"])
    # Upstream 8351742905 moved the old alcohol use actions to consumption
    # flags with the same strengths. Preserve every unrelated use action.
    actions = obj.get("use_action")
    action_list = actions if isinstance(actions, list) else [actions]
    old_alcohol = [a for a in action_list if isinstance(a, str) and a in ALCOHOL]
    if old_alcohol:
        if len(set(old_alcohol)) != 1:
            raise ValueError("Multiple legacy alcohol strengths")
        add_alcohol_flag(obj, old_alcohol[0])
        remaining = [a for a in action_list if not isinstance(a, str) or a not in ALCOHOL]
        if remaining:
            obj["use_action"] = remaining if isinstance(actions, list) else remaining[0]
        else:
            del obj["use_action"]
    return obj


def migrate_document(document, path):
    multiple = isinstance(document, list)
    objects = document if multiple else [document]
    migrated = []
    count = 0
    errors = []
    for obj in objects:
        try:
            new = migrate_item(obj)
        except ValueError as exc:
            ident = obj.get("id", obj.get("abstract", "?"))
            errors.append(f"{path}: {ident}: {exc}")
            continue
        count += new != obj
        migrated.append(new)
    if errors:
        raise ValueError("\n".join(errors))
    return (migrated if multiple else migrated[0]), count


def migrate_mods(target, folders=FOLDERS, check=False):
    target = Path(target)
    prepared = []
    errors = []
    for name in folders:
        folder = target / name
        if not (folder / "modinfo.json").is_file():
            raise FileNotFoundError(f"Missing mod: {folder}")
        for path in sorted(folder.rglob("*.json")):
            if path.is_symlink() or any(p.is_symlink() for p in path.parents
                                       if p != target and target in p.parents):
                raise ValueError(f"Refusing symbolic link: {path}")
            try:
                original = json.loads(path.read_text(encoding="utf-8-sig"))
                new, count = migrate_document(original, path)
            except ValueError as exc:
                errors.append(str(exc))
                continue
            if count:
                prepared.append((path, new, count))
    if errors:
        raise ValueError("Community migration errors:\n" + "\n".join(errors))
    # Validate every document before rewriting any; original source pins and
    # translation manifests remain unchanged. Per-file atomic replacement.
    if check and prepared:
        raise ValueError("Unmigrated item JSON: " +
                         ", ".join(str(path) for path, _, _ in prepared))
    total = 0
    for path, new, count in prepared:
        with tempfile.NamedTemporaryFile(mode="w", dir=path.parent, encoding="utf-8",
                                         delete=False) as output:
            temporary = Path(output.name)
            try:
                json.dump(new, output, ensure_ascii=False, indent=2)
                output.write("\n")
            except BaseException:
                temporary.unlink(missing_ok=True)
                raise
        try:
            temporary.replace(path)
        finally:
            temporary.unlink(missing_ok=True)
        print(f"{path}: migrated {count} item declarations")
        total += count
    print(f"Community item migration: {total} declarations in {len(prepared)} files")
    return total


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--target", type=Path, default=Path("data/mods"))
    parser.add_argument("--check", action="store_true",
                        help="Fail if further known migrations are needed")
    args = parser.parse_args()
    migrate_mods(args.target, check=args.check)


if __name__ == "__main__":
    main()
