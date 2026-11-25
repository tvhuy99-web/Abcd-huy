#!/usr/bin/env python3
"""Migrate pinned community item JSON after verifying/applying text overlays.

Transform top-level item declarations and formatting of exact Vietnamese
overlay text. Do not alter item_group subtypes, nested effect/recipe types,
identifiers, translated wording, placeholders or weights.
Never apply these rules to saves or the source archive used to verify overlays.
"""
import argparse
from copy import deepcopy
import json
from pathlib import Path
import tempfile
import re

if __package__:
    from .apply_vi_overlays import OVERLAYS, TOKEN, load_overlay, target_for_pointer
else:
    from apply_vi_overlays import OVERLAYS, TOKEN, load_overlay, target_for_pointer

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
    # The current tool slot owns tool_ammo, not the old generic ammo field.
    # Do not reinterpret ammo belonging to a gun, magazine or other slot.
    if "TOOL" in slots and set(slots) <= {"TOOL", "ARMOR"} and "ammo" in obj:
        legacy = obj.pop("ammo")
        legacy = [legacy] if isinstance(legacy, str) else legacy
        if not isinstance(legacy, list) or not all(isinstance(x, str) for x in legacy):
            raise ValueError("Invalid legacy tool ammo")
        if "tool_ammo" in obj and obj["tool_ammo"] != legacy:
            raise ValueError("Conflicting tool ammo fields")
        obj["tool_ammo"] = legacy
    # These two pinned MST transform targets inherit the corresponding
    # upstream tinderbox states. The lit parent has SPAWN_ACTIVE, and
    # item_transformation derives activity from the target, not the actor.
    targets = {"tinderbox_clay": ("tinderbox_clay_on", True),
               "tinderbox_clay_on": ("tinderbox_clay", False)}
    if obj.get("id") in targets:
        target, active = targets[obj["id"]]
        actions = obj.get("use_action", [])
        for action in actions if isinstance(actions, list) else [actions]:
            if (isinstance(action, dict) and action.get("type") == "transform"
                    and action.get("target") == target and "active" in action):
                if action["active"] is not active:
                    raise ValueError("Unexpected legacy tinderbox activity")
                del action["active"]
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


def normalize_text_style(value):
    """Apply only the punctuation fixes used by the upstream text lint.

    Format tags and placeholders must remain byte-for-byte unchanged.
    English checker uses an ASCII word length of 3 before '.' and 1
    before '!' or '?'. Vietnamese translation wording stays untouched.
    """
    parts = []
    end = 0
    for token in TOKEN.finditer(value):
        parts.append((False, value[end:token.start()]))
        parts.append((True, token.group()))
        end = token.end()
    parts.append((False, value[end:]))
    def fix(text):
        text = text.replace("loại !!VUI NHẤT!!", "loại VUI NHẤT!!")
        text = text.replace("...", "…")
        text = re.sub(r"(?<=[A-Za-z0-9-]{3})\.(?= [^ \n])", ". ", text)
        text = re.sub(r"(?<=[A-Za-z0-9-])([!?]+) (?=[^ \n])", r"\1  ", text)
        return text
    # A format token after a sentence is still the next non-space text
    # for the native lint (for example "vitae. %s!"). Supply that context
    # to the unprotected segment without modifying the actual token.
    result = "".join(
        text if protected else
        (fix(text + "X")[:-1] if index + 1 < len(parts) and parts[index + 1][0]
         else fix(text))
        for index, (protected, text) in enumerate(parts))
    if TOKEN.findall(result) != TOKEN.findall(value):
        raise ValueError("Text style normalization changed protected tokens")
    return result


def normalize_localized_document(document, changes):
    """Normalize exact overlay text values, never technical or untranslated data.

    The native item/monster name schema supports str_sp for names whose
    singular/plural are identical. Vietnamese does not append English 's'.
    Keep explicitly different translations, and never convert descriptions
    or other singular-only translation objects to str_sp.
    """
    if not changes:
        return document
    result = deepcopy(document)
    name_entries = set()
    for pointer, translated in changes:
        # The same migration runs again with --check; a normalized name
        # can have str_sp instead of the old pointer's str/str_pl, and
        # migrated armor_data can have moved to the top level.
        try:
            parent, key = target_for_pointer(result, pointer)
            current = parent[key]
        except (KeyError, IndexError, TypeError, ValueError):
            continue
        if not isinstance(current, str) or current != translated:
            continue
        if re.search(r"/(?:decay_messages|apply_message)/\d+/1$", pointer):
            continue
        parent[key] = normalize_text_style(current)
        match = re.fullmatch(r"/(\d+)/name(?:/str(?:_pl|_sp)?)?", pointer)
        if match:
            name_entries.add(int(match.group(1)))
        elif re.fullmatch(r"/name(?:/str(?:_pl|_sp)?)?", pointer):
            name_entries.add(0)
    objects = result if isinstance(result, list) else [result]
    for index in name_entries:
        if index >= len(objects):
            continue
        obj = objects[index]
        if not isinstance(obj, dict) or obj.get("type") not in (
                {"ITEM", "MONSTER"} | LEGACY_TYPES.keys()):
            continue
        name = obj.get("name")
        if isinstance(name, str):
            obj["name"] = {"str_sp": name}
        elif isinstance(name, dict) and "str" in name and (
                "str_pl" not in name or name["str"] == name["str_pl"]):
            name["str_sp"] = name.pop("str")
            name.pop("str_pl", None)
    return result


def normalize_pinned_source_text(document, folder, relative):
    """Known native-lint failures in untranslated pinned source fields."""
    if folder != "CataclysmPlusPlus":
        return document
    if relative not in ("modinfo.json", "Npc/c_bgss.json"):
        return document
    result = deepcopy(document)
    objects = result if isinstance(result, list) else [result]
    if relative == "modinfo.json":
        for obj in objects:
            if isinstance(obj, dict) and obj.get("type") == "MOD_INFO" and isinstance(
                    obj.get("description"), str):
                obj["description"] = normalize_text_style(obj["description"])
    else:
        # Only the literal dialogue response reported by the native loader.
        def visit(value):
            if isinstance(value, dict):
                if value.get("text") == "...":
                    value["text"] = "…"
                for child in value.values():
                    visit(child)
            elif isinstance(value, list):
                for child in value:
                    visit(child)
        visit(result)
    return result


def migrate_mst_recipes(document, relative):
    """Port the pinned MST recipes to the current DDA requirements and yields."""
    if not relative.startswith("recipes/"):
        return document
    result = deepcopy(document)
    quantities = {"pitch_wood": 10, "bearing_lead": 30, "oil_garlic": 6}
    for obj in result if isinstance(result, list) else [result]:
        if not isinstance(obj, dict) or obj.get("type") != "recipe":
            continue
        ident = obj.get("result")
        # Preserve the previous implicit default charge count reported by
        # the real loader; do not change an explicit recipe yield.
        if ident in quantities and "charges" not in obj:
            obj["charges"] = quantities[ident]
        if ident in ("pemmican", "pemmican_meat"):
            # recipe::load replaces inline requirements rather than inheriting
            # omitted tools and qualities. Preserve the current DDA parent
            # requirements when MST overrides only the component alternatives.
            # These values match data/json/recipes/food/meat_dishes.json
            # at the integrated upstream revision 2917d666.
            if obj.get("copy-from") == ident:
                heat = 40 if ident == "pemmican" else 36
                obj.setdefault("tools", [[["surface_heat", heat, "LIST"]]])
                obj.setdefault("qualities", [{"id": "CUT", "level": 2}])
            for alternatives in obj.get("components", []):
                for component in alternatives:
                    if (isinstance(component, list) and len(component) == 3
                            and component[0] == "edible_tallow_lard" and component[2] == "LIST"):
                        component[0] = "edible_tallow"
    return result


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
        overlay_kind = next((k for k, v in OVERLAYS.items() if v["folder"] == name), None)
        overlay = load_overlay(overlay_kind) if overlay_kind else {}
        for path in sorted(folder.rglob("*.json")):
            if path.is_symlink() or any(p.is_symlink() for p in path.parents
                                       if p != target and target in p.parents):
                raise ValueError(f"Refusing symbolic link: {path}")
            try:
                original = json.loads(path.read_text(encoding="utf-8-sig"))
                localized = normalize_localized_document(
                    original, overlay.get(path.relative_to(folder).as_posix(), ()))
                relative = path.relative_to(folder).as_posix()
                localized = normalize_pinned_source_text(localized, name, relative)
                if name == "MST_Extra":
                    localized = migrate_mst_recipes(localized, relative)
                new, _ = migrate_document(localized, path)
                # Count both schema and formatting changes relative to the
                # original, including non-item localized declarations.
                if isinstance(original, list):
                    count = sum(before != after for before, after in zip(original, new))
                else:
                    count = int(original != new)
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
    print(f"Community compatibility migration: {total} declarations in {len(prepared)} files")
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
