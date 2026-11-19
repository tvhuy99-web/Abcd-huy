# Vietnamese localization overlays for four optional CDDA community mods

The project user supplied Vietnamese ZIPs for Cataclysm++, Arcana: Strange Aeons,
Blazemod and MST Extra on 19 November 2026. Each ZIP is a *translation input*,
not an override of CDDA gameplay mechanics.

| Bundled CDDA mod | Pinned upstream commit | Selected Vietnamese text fields | JSON files |
|---|---|---:|---:|
| Cataclysm++ | `46982a63da25b064c8d0fdee9b68c23dccc1f671` | 1,656 | 52 |
| Arcana: Strange Aeons | `080d8e9e454d2179bc98eb07de075f74e58bcbd1` | 6,546 | 185 |
| Blazemod | `14530cc7ebfaac6c85586e7d7728635f8fda7d6d` | 703 | 50 |
| MST Extra | `5f9dbfef550d5c140865ce5f6a32c40eb34c07ca` | 235 | 23 |
| **Total** | | **9,140** | **310** |

The build first installs the original mods with `install_curated.py` and then
applies localized strings from the four base64-encoded LZMA JSON overlays using
`apply_vi_overlays.py`. This keeps original object IDs, dependencies, game
balance, recipes and numeric values. The script checks formatting tokens
(`%s`, `%d`, markup tags, etc.) and refuses technical-field edits.

**Extra Arcana safety:** `arcana_upstream_blob_shas.json` lists the original
Git blob identities for all 339 Arcana JSON files at the pinned source revision.
The overlay applicator verifies every Arcana file it touches is byte-for-byte
the exact upstream file, before writing any Vietnamese text. This guards
against silently applying translations to an incompatible mod update.

The translated strings were selected from the user-supplied localized ZIP.
Only Vietnamese-looking display text is included; technical values, map
layouts, identifiers, developer comments and engine lookup strings are
excluded. That is not proof that every English string has been translated
or that every Vietnamese phrase is stylistically perfect. Literal technical
tokens must remain untranslated, and AI-generated/fan translations should
be reviewed with the in-game voice reader.

The MST Extra and Cataclysm++ ZIPs included Bright Nights variants. Only
the CDDA directories were considered here. The Arcana ZIP's 339 JSON files
were all syntactically valid and its MOD_INFO ID is `Arcana`.

To validate and apply the overlays after installing the pinned upstream mods:

```sh
python3 tools/mods/install_curated.py --install cataplusplus arcana blazemod mst_extra --target data/mods
python3 tools/mods/apply_vi_overlays.py --target data/mods --dry-run
python3 tools/mods/apply_vi_overlays.py --target data/mods
python3 -m unittest discover -s tools/accessibility -p 'test_curated_mod_vi_overlays.py' -v
```

The four mods' upstream licenses and notices must be preserved as provided
by their maintainers. Cataclysm++ uses Unlicense; Arcana, Blazemod and MST Extra
use Creative Commons Attribution-ShareAlike 3.0.

**Validation remains necessary before publishing:** an Android candidate APK
must finish compiling and its contents must be inspected. The four mods must
also pass the exact game's loader in an isolated disposable world, and voice
reader output should be verified on an Android device. The draft pull request
must not be merged just because translation extraction succeeds.
