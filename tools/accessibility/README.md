# Accessibility integration inventory

From the repository root:

```sh
python3 -m unittest discover -s tools/accessibility -p 'test_*.py' -v
python3 tools/accessibility/inventory.py --output /tmp/a11y-inventory.json
bash tools/accessibility/run_core_tests.sh
bash tools/accessibility/run_speech_tests.sh
```

The report records source candidates, their locations and literal/dynamic
arguments. It never marks an entry accessible. It is a heuristic scanner, not a
C++ parser: overloads, aliases, generated screens, variable-built contexts and
compile-time branches need manual review. Counts are call sites, not unique
commands, runtime screens or an accessibility coverage percentage.

The Android accessibility workflow independently builds the pinned baseline and
the candidate as ARM64 debug APKs. An APK artifact is build evidence, not device
or blind-user acceptance. Use the plan and status under `doc/accessibility/`.

The core runner builds the production context primitive with the repository's
Catch tests without linking gameplay. The speech runner checks the production
session gate, then runs the actual Android speech driver with deterministic
Handler/TTS test doubles under `fakes/`. These doubles are never packaged in the
app and do not emulate Android, Binder, a real TTS service or audio output. A
separate CI job compiles the driver against the real Android API 35 jar. Real
engine, device, JNI and user journeys remain separate acceptance gates.
