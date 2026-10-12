# English-only first stage

This fork currently offers English as its sole game language. English text is
already present in the C++ source and JSON data, so no core `.po` or `.mo`
catalog is needed.

- Non-English Android string resources, localized names and title art are removed.
- The language setting defaults to `en`; saved preferences and device locales
  cannot switch this stage to another language.
- The GitHub translations job preserves the artifact interface used by releases,
  but does not contact Transifex or download translated catalogs.
- Make, CMake and Android builds do not compile core translation catalogs.
- Android packaging and its installer file list exclude `.po`, `.mo` and `.pot`.
- Translation parser tests use a small English fixture, which is excluded from APKs.

The translation engine and extraction tools remain available for a later
Vietnamese stage. This change does not rewrite Git history; earlier translations
can still be recovered from existing commits and upstream releases.
