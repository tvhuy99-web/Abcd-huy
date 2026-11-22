# Five optional community mods for Abcd-huy

The curator manifest (curated_mods.json) identifies EXACT source revisions and their CDDA mod subfolders:

1. Cataclysm++: Noctifer-de-Mortem/nocts_cata_mod (DDA folder)
2. Arcana: Strange Aeons: Standing-Storm/cdda-arcana-mod
3. Secronom Zombies: Kenan2000/Secronom-Zombies (NOT LICENSED FOR REDISTRIBUTION)
4. Blazemod: Erin105/blazemod
5. More Survival Tools Extra: chaosvolt/MST_Extra_Mod (DDA folder)

The installer copies the actual mod content into the target directory without changing saves, enabling mods, or overwriting an existing mod folder. It retains the source licence file when one exists in the upstream archive. The original project and authors remain the source of the content; no ownership is claimed by Abcd-huy.

Commands (from the repository root, with Python 3.10+):
    
    python3 tools/mods/install_curated.py --list
    python3 tools/mods/install_curated.py --install cataplusplus arcana blazemod mst_extra --target data/mods
    python3 tools/mods/install_curated.py --install secronom --target /path/to/personal/user/mods --allow-local-unlicensed
    python3 -m unittest discover -s tools/accessibility -p 'test_curated_mod_installer.py' -v

IMPORTANT: Secronom has no verified license for redistribution. The local-only option is intended for the end user's own private installation. Do not add its contents to a public repo or distribute it in a public APK unless the rightsholder grants suitable permission.

Licenses:
- Cataclysm++: Unlicense, upstream repository LICENSE.
- Arcana, Blazemod and MST Extra: Creative Commons Attribution-ShareAlike 3.0, upstream LICENSE.txt.
- Secronom: unspecified / permission not yet verified.

Each extra mod is optional. These sources are not automatically added to data/mods/default.json, so no existing or new world enables them unexpectedly. The existing world mod-selection screen loads mods from data/mods and the per-user mods directory.

Compatibility caveat: the pinned external mod data has NOT YET passed the game engine's full JSON and gameplay loader for the exact Abcd-huy revision. Installing it and successfully creating and reloading a disposable world must precede inclusion in a public APK. In particular, modified items, monsters, effects, vehicles and EOCs may conflict with the rapidly changing experimental CDDA code.

Vietnamese translation: translate text fields after the data has passed loading checks. Preserve object IDs and JSON keys. Translation may require the game's gettext extraction/localization build and game-specific context; never translate raw JSON blindly.

## Vietnamese community translation overlays

The candidate/test APK builds download the pinned upstream CDDA mods first, then
apply Vietnamese text-only overlays from `tools/mods/translations/`. All game
IDs, item mechanics, recipes, mod dependencies and numerical parameters remain
those of the pinned source. These translations are community-supplied and still
require native-game loading and voice/accessibility acceptance testing.

- **MST Extra** and **Blazemod**: user-provided Vietnamese text overlays.
- **Cataclysm++**: 1,656 Vietnamese strings in 52 JSON files from the
  `nocts_cata_mod_DDA` folder at commit
  `46982a63da25b064c8d0fdee9b68c23dccc1f671`. The separate
  Bright Nights version in the uploaded ZIP is deliberately excluded.
- **Arcana**: 6,794 Vietnamese strings across 185 JSON files, checked against pinned upstream file identities. The overlay changes only user-facing text; gameplay compatibility still needs testing.

To check and apply the three overlays to installed pinned mods:

```sh
python3 tools/mods/apply_vi_overlays.py --target data/mods --dry-run
python3 tools/mods/apply_vi_overlays.py --target data/mods
```

The dry run rejects missing JSON pointer targets, altered technical fields,
corrupt overlay checksums, changed source revisions and broken format/colour
tags. Do not translate JSON property names or IDs. Changes in gameplay data
or additional upstream mod revisions require a new compatibility review.

## Compatibility migration for the upstream integration branch

After the pinned Vietnamese overlays are verified and applied, candidate builds
run a separate item-schema migration against the four packaged mod folders:

```sh
python3 tools/mods/migrate_item_schema.py --target data/mods
python3 tools/mods/migrate_item_schema.py --target data/mods --check
```

The migration preserves item IDs, localized text and numeric fields. It converts
legacy item types and singular subtype declarations, retains old nested armor
properties, and moves the removed alcohol use actions to the corresponding
upstream consumption flags. It does not rewrite saves or change enabled mods.
The checker is idempotent and rejects conflicting fields rather than discarding
them. This is a targeted schema migration, not proof that every mod mechanic
is compatible. Native loader checks and on-device acceptance are still required.
