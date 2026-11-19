#!/usr/bin/env python3
"""Install five optional, revision-pinned CDDA community mods.

Run only while the game is closed. The program never enables world mods,
overwrites installed directories, or modifies saved worlds. Secronom has
no verified redistribution license, so it requires an explicit local-only flag.
"""
import argparse
import io
import json
from pathlib import Path, PurePosixPath
import re
import shutil
import stat
import tempfile
import urllib.request
import zipfile

CATALOG = Path(__file__).with_name("curated_mods.json")
MAX_ARCHIVE = 64 * 1024 * 1024
MAX_FILE = 24 * 1024 * 1024
MAX_EXPANDED = 140 * 1024 * 1024
MOD_ID = re.compile(r"^[a-z][a-z0-9_]*$")


def read_catalog(path=CATALOG):
    raw = json.loads(Path(path).read_text(encoding="utf-8"))
    assert raw["schema_version"] == 1
    mods = raw["mods"]
    assert len(mods) == 5
    assert len({m["id"] for m in mods}) == 5
    for mod in mods:
        if (not MOD_ID.fullmatch(mod["id"])
                or not re.fullmatch(r"[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+", mod["repository"])
                or not re.fullmatch(r"[0-9a-f]{40}", mod["revision"])
                or not re.fullmatch(r"[A-Za-z0-9_.-]+", mod["destination"])
                or not re.fullmatch(r"[A-Za-z0-9_.-]*", mod["source_subdir"])):
            raise ValueError("Unsafe curated mod descriptor")
    return mods


def download_zip(mod, opener=urllib.request.urlopen):
    url = ("https://codeload.github.com/" + mod["repository"]
           + "/zip/" + mod["revision"])
    request = urllib.request.Request(url, headers={"User-Agent": "Abcd-huy-mods/1.0"})
    with opener(request, timeout=90) as response:
        blob = response.read(MAX_ARCHIVE + 1)
    if len(blob) > MAX_ARCHIVE:
        raise ValueError("Source archive exceeds maximum download size")
    return blob


def extract_mod(blob, mod, out_dir):
    """Only extract the selected mod directory; never trust ZIP paths."""
    out_dir = Path(out_dir)
    prefix = mod["source_subdir"]
    seen = set()
    expanded = 0
    metadata = None
    with zipfile.ZipFile(io.BytesIO(blob)) as archive:
        for entry in archive.infolist():
            raw = entry.filename
            path = PurePosixPath(raw)
            parts = path.parts
            if (not parts or path.is_absolute() or "\\" in raw
                    or any(part in (".", "..") for part in parts)):
                raise ValueError("Unsafe ZIP entry: " + raw)
            # The first component is GitHub's top-level archive directory.
            if len(parts) < 2:
                continue
            inner_parts = parts[1:]
            if inner_parts in (("LICENSE",), ("LICENSE.txt",)):
                metadata = entry if not prefix else (metadata or entry)
            if prefix:
                if inner_parts[0] != prefix or len(inner_parts) < 2:
                    continue
                rel_parts = inner_parts[1:]
            else:
                rel_parts = inner_parts
            if entry.is_dir():
                continue
            mode = (entry.external_attr >> 16) & 0xffff
            if stat.S_ISLNK(mode):
                raise ValueError("Symbolic links are forbidden in mod archives")
            relative = PurePosixPath(*rel_parts)
            if (relative.as_posix() in seen or entry.file_size > MAX_FILE):
                raise ValueError("Duplicate or oversized mod file: " + relative.as_posix())
            seen.add(relative.as_posix())
            expanded += entry.file_size
            if expanded > MAX_EXPANDED:
                raise ValueError("Unpacked mod is too large")
            dest = out_dir.joinpath(*relative.parts)
            dest.parent.mkdir(parents=True, exist_ok=True)
            with archive.open(entry) as source, dest.open("xb") as output:
                shutil.copyfileobj(source, output, 1024 * 128)
        if metadata is not None and prefix:
            license_name = PurePosixPath(metadata.filename).name
            license_path = out_dir / license_name
            if not license_path.exists():
                with archive.open(metadata) as source, license_path.open("xb") as output:
                    shutil.copyfileobj(source, output, 1024 * 128)
    if not seen:
        raise ValueError("Selected source directory contains no files")
    verify_modinfo(out_dir, mod)


def verify_modinfo(path, mod):
    info_path = Path(path) / "modinfo.json"
    if not info_path.is_file():
        raise ValueError("modinfo.json not found for " + mod["id"])
    contents = json.loads(info_path.read_text(encoding="utf-8-sig"))
    objects = contents if isinstance(contents, list) else [contents]
    matches = [obj for obj in objects if isinstance(obj, dict)
               and obj.get("type") == "MOD_INFO" and obj.get("id") == mod["mod_id"]]
    if len(matches) != 1:
        raise ValueError("Unexpected MOD_INFO id for " + mod["id"])
    dependencies = matches[0].get("dependencies", [])
    if not isinstance(dependencies, list) or not all(
            required in dependencies for required in mod["required_mod_ids"]):
        raise ValueError("Unexpected declared dependencies for " + mod["id"])
    return matches[0]


def install(mod, target, allow_local_unlicensed=False, downloader=download_zip):
    if not mod["redistributable"] and not allow_local_unlicensed:
        raise ValueError("Secronom is local-use only; pass --allow-local-unlicensed "
                         "when installing for your own use. Do not repackage it publicly.")
    target = Path(target)
    target.mkdir(parents=True, exist_ok=True)
    dest = target / mod["destination"]
    if dest.exists() or dest.is_symlink():
        raise FileExistsError("Refusing to overwrite " + str(dest))
    # A staging directory in the same filesystem allows an atomic rename.
    with tempfile.TemporaryDirectory(prefix=".curated-mod-", dir=target) as temporary:
        staged = Path(temporary) / "payload"
        staged.mkdir()
        extract_mod(downloader(mod), mod, staged)
        # Save provenance alongside the mod. Do not change the modinfo itself.
        (staged / "CURATED_SOURCE.txt").write_text(
            mod["repository"] + "\n" + mod["revision"] + "\n"
            + "License: " + mod["license"] + "\n", encoding="utf-8")
        if dest.exists() or dest.is_symlink():
            raise FileExistsError("Refusing to overwrite " + str(dest))
        staged.rename(dest)
    return dest


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--list", action="store_true", help="Show five pinned community mods")
    parser.add_argument("--install", nargs="+", metavar="MOD",
                        help="Install selected IDs or 'all' (without enabling any mod)")
    parser.add_argument("--target", type=Path, default=Path("data/mods"),
                        help="Destination mod directory, e.g. data/mods or user mods/")
    parser.add_argument("--allow-local-unlicensed", action="store_true",
                        help="Permit local-only Secronom download; NEVER package publicly")
    options = parser.parse_args(argv)
    mods = read_catalog()
    selected = {mod["id"]: mod for mod in mods}
    if options.list or options.install is None:
        for mod in mods:
            distribution = "redistributable" if mod["redistributable"] else "LOCAL-ONLY"
            print(f'{mod["id"]:16} {mod["name"]}: {distribution} ({mod["license"]})')
        if options.install is None:
            return 0
    ids = list(selected) if options.install == ["all"] else options.install
    if len(ids) != len(set(ids)):
        parser.error("A mod was selected more than once")
    for mod_id in ids:
        if mod_id not in selected:
            parser.error("Unknown mod: " + mod_id)
    if any(not selected[name]["redistributable"] for name in ids) and not options.allow_local_unlicensed:
        parser.error("Secronom requires --allow-local-unlicensed for personal installation")
    for mod_id in ids:
        mod = selected[mod_id]
        print("Installing", mod["name"], "from", mod["revision"])
        print("Installed in", install(mod, options.target, options.allow_local_unlicensed))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
