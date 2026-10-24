#!/usr/bin/env bash
set -euo pipefail

task_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)"
task_serial="${ANDROID_SERIAL:-emulator-5554}"
task_evidence="$task_root/build-evidence/android-reader-ui"
mkdir -p "$task_evidence"

collect_log() {
    adb -s "$task_serial" logcat -d > "$task_evidence/logcat.txt" 2>&1 || true
}
trap collect_log EXIT

# Use the same adb as the emulator action, avoiding a second ddmlib/UTP device
# connection during first boot. Every reader instrumentation class must run.
timeout 120 adb -s "$task_serial" wait-for-device
task_ready=false
for task_attempt in {1..30}; do
    if adb -s "$task_serial" shell cmd package path android > "$task_evidence/package-service.txt" 2>&1; then
        task_ready=true
        break
    fi
    adb reconnect offline >/dev/null 2>&1 || true
    sleep 2
done
if [[ "$task_ready" != true ]]; then
    echo 'Emulator package service did not become ready' >&2
    exit 1
fi

shopt -s nullglob
task_apps=( "$task_root"/android/app/build/outputs/apk/accessibility/debug/*.apk )
task_tests=( "$task_root"/android/app/build/outputs/apk/androidTest/accessibility/debug/*.apk )
if (( ${#task_apps[@]} != 1 || ${#task_tests[@]} != 1 )); then
    echo 'Expected one x86_64 UI APK and one test APK' >&2
    exit 1
fi
adb -s "$task_serial" install -r "${task_apps[0]}"
adb -s "$task_serial" install -r "${task_tests[0]}"
adb -s "$task_serial" shell am instrument -w -r \
    -e package com.cleverraven.cataclysmdda \
    com.cleverraven.cataclysmdda.accessibility.test/androidx.test.runner.AndroidJUnitRunner \
    | tee "$task_evidence/instrumentation.txt"

python3 - "$task_evidence/instrumentation.txt" "$task_root" <<'PY'
import pathlib
import re
import sys
result = pathlib.Path(sys.argv[1]).read_text()
test_root = pathlib.Path(sys.argv[2]) / 'android/app/src/androidTest/java'
required = set()
minimum = 0
for path in test_root.rglob('*Test.java'):
    text = path.read_text()
    count = len(re.findall(r'@Test\b', text))
    if count:
        package = re.search(r'package\s+([\w.]+)\s*;', text).group(1)
        required.add(package + '.' + path.stem)
        minimum += count
summary = re.search(r'(?m)^OK \((\d+) tests?\)', result)
ran = set(re.findall(r'INSTRUMENTATION_STATUS: class=(\S+)', result))
if not minimum or not summary or int(summary.group(1)) < minimum or not required.issubset(ran):
    raise SystemExit('Not all reader instrumentation classes/tests ran and passed')
if any(marker in result for marker in ('FAILURES!!!', 'INSTRUMENTATION_FAILED', 'Process crashed')):
    raise SystemExit('Instrumentation reported a failure')
print(f'Reader UI smoke: {summary.group(1)} tests in {len(required)} classes ran and passed')
PY
