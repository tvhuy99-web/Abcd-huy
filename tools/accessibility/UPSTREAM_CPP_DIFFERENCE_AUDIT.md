# Upstream C++ difference audit (2026-11-20)

This audit compares **file contents**, not Git ancestry.

- Base used for the three-way comparison: `CleverRaven/Cataclysm-DDA@66bdc3bf813b343fbd8d5cb18e4fa85e5a12a4d1`.
- Upstream target: `CleverRaven/Cataclysm-DDA@e422f76622480bcc53fd26b2553f401648ede8db`.
- Fork snapshot before this audit: `tvhuy99-web/Abcd-huy@ce883fdaff226d09eab09ccd8283d78ce86724f5`, branch `test/upstream-full-merge-2026-11-19`.

## Source-tree three-way results

| Category | Files | Action |
| --- | ---: | --- |
| Byte-for-byte identical to upstream | 1,085 | No action |
| Changed in fork, unchanged in upstream since base | 148 | **Keep fork changes**; do not overwrite accessibility/UI work |
| Changed in both fork and upstream | 6 | Audit the upstream delta against fork |
| Upstream changed; fork retained baseline | 1 | Apply the change |
| New upstream translation unit, fork also customized | 1 | Keep upstream logic plus Android profiling |
| Present upstream but absent in fork | 0 | No missing source files |

The six files changed on both sides are `character.h`, `creature.cpp`, `map.cpp`, `mattack_actors.cpp`, `melee.cpp`, and `monster.cpp`. The upstream changes in the first five concern combined dodge/block melee defense or the map-damage code split. Their behavioral code was already present in the fork; map-damage functions were moved to `map_damage.cpp`, and nested-item reconciliation was updated while preserving the fork's uniform/empty-submap optimizations. `monster.cpp` also retains the block/parry/dodge reporting. Source-level regression guards cover these integration points.

`map.h` lacked only the upstream TODO comment above `hit_with_acid`, restored in this audit. The new upstream `map_cache.cpp` had already been integrated; the fork retains additional Android diagnostic timing hooks.

## What these numbers do **not** establish

- Identical baseline/current upstream content in 148 files means *upstream made no changes to those files in this interval*. It does not mean the fork customizations should be removed.
- Presence of expected source statements does not prove functional equivalence in every case. Native tests, save/load exercises and physical-device TalkBack/radar tests remain important.
- GitHub may still show diverged commit history and merge conflicts; this audit measures code content, not the readiness of PR #19.
- The categorization is a point-in-time snapshot. Refresh it when upstream changes.

Existing checks: `test_upstream_map_damage_split.py`, `test_upstream_map_reconcile.py`, native smoke workflow, Android accessibility workflow. Added check: `test_upstream_combined_block_merge.py`.
