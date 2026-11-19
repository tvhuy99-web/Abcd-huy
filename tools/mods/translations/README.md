# Vietnamese translation overlays for two CDDA mods

Two text-only Vietnamese translations were supplied by the Abcd-huy project user
as `MST_Extra_Mod_VI.zip` and `blazemod_VI.zip` on 19 November 2026.

| Installed mod | Upstream pinned revision | Translated strings | JSON files with translations |
|---|---|---:|---:|
| MST Extra (CDDA only) | `5f9dbfef550d5c140865ce5f6a32c40eb34c07ca` | 235 | 23 |
| Blazemod | `14530cc7ebfaac6c85586e7d7728635f8fda7d6d` | 703 | 50 |

**Only localized UTF-8 text fields are replaced.** The build first installs the
original pinned mods using `install_curated.py`. Then `apply_vi_overlays.py`
applies the verified text, checks ZIP-derived overlay checksums and preserved
formatting placeholders, and refuses to change identifier fields. This keeps
original gameplay data, object IDs, dependencies, recipes, and numeric values.

The uploaded MST Extra archive also contained `MST_Extra_BN` (Bright Nights).
That submod is intentionally excluded: Abcd-huy is a CDDA-based project.

The overlay files are base64-encoded LZMA JSON tables of localized JSON-pointer
string values. They were checked against the exact exported text for both ZIPs.
They are not executable, and may be audited by decoding them with Python's
`base64`, `lzma`, and `json` modules.

To check and apply locally after installing the pinned upstream files:

```sh
python3 tools/mods/apply_vi_overlays.py --target data/mods --dry-run
python3 tools/mods/apply_vi_overlays.py --target data/mods
python3 -m unittest discover -s tools/accessibility -p 'test_curated_mod_vi_overlays.py' -v
```

The translated content derives from the upstream projects licensed under
**Creative Commons Attribution-ShareAlike 3.0**. Preserve their original
copyright notices and LICENSE.txt files when distributing the translation.
This does not imply endorsement by either upstream project.

**Status:** JSON syntax, mod IDs, selected structural comparisons and
translation-only overlays were checked. A full game-engine mod load and
physical Android voice-reader acceptance test are still required before merge.
The overlays represent the Vietnamese-accented text identified in supplied ZIPs;
this is not a certification of 100% translation completeness.
