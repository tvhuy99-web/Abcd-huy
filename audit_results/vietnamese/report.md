# Vietnamese vs English: translation audit

Source: freshly extracted English gettext template from this commit.
Comparison uses exact gettext (context, msgid) keys.

| Measure | Count |
|---|---:|
| Current English keys | 116272 |
| Current keys with usable Vietnamese | 116272 |
| Coverage (%) | 100.0 |
| English keys missing from Vietnamese catalog | 0 |
| Present but empty | 0 |
| Marked fuzzy (needs review) | 0 |
| Overall not ready for translation | 0 |
| Old catalog keys not in current English | 0 |
| Identical to English (may be proper names) | 2717 |
| Plural translations requiring review | 0 |

## By source type

| Type | English | Usable VI | Missing |
|---|---:|---:|---:|
| C++/UI | 12534 | 12534 | 0 |
| JSON/game content | 103738 | 103738 | 0 |

## Missing from Vietnamese catalog (sample)

None.

## Empty translations (sample)

None.

## Fuzzy translations (sample)

None.

## Outdated keys (sample)

None.

## Same as English (examples, not necessarily errors) (sample)

- **English:** `%d/%d %s`
  - Source: `src/achievement.cpp:506`
- **English:** `%1$s %2$d (%4$d%%) -> %3$d`
  - Source: `src/activity_actor.cpp:2625`
  - Context: `reading progress`
- **English:** `%s %s %s.`
  - Source: `src/activity_actor.cpp:4064`
- **English:** `%1$s x%2$d`
  - Source: `src/activity_actor.cpp:8823`
- **English:** `%s %s `
  - Source: `src/activity_handlers.cpp:590`
- **English:** `%s (%d)`
  - Source: `src/activity_handlers.cpp:626, src/activity_handlers.cpp:630`
- **English:** `<empty>`
  - Source: `src/armor_layers.cpp:811`
- **English:** `<color_light_green>%1$s</color> %2$s`
  - Source: `src/auto_note.cpp:381`
- **English:** `%1$s:%2$s, `
  - Source: `src/auto_note.cpp:516, src/overmap_ui.cpp:1359`
  - Context: `note color`
- **English:** `[B]`
  - Source: `src/basecamp.cpp:54`
  - Context: `base camp: base`
- **English:** `VRRRRMP!`
  - Source: `src/bionics.cpp:925`
- **English:** `100 %%`
  - Source: `src/bionics.cpp:3170`
- **English:** `90 %%`
  - Source: `src/bionics.cpp:3171`
- **English:** `70 %%`
  - Source: `src/bionics.cpp:3172`
- **English:** `50 %%`
  - Source: `src/bionics.cpp:3173`

## Gettext format / catalog validation

Exit code: 0

```text
116272 translated messages.

```

Note: A translation key present in the catalog is not proof of linguistic quality;
this audit measures gettext completeness and compilation diagnostics.
