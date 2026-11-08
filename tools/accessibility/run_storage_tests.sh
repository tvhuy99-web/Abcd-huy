#!/usr/bin/env bash
set -euo pipefail
task_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)"
task_build="$(mktemp -d)"
trap 'rm -rf -- "$task_build"' EXIT
if command -v javac >/dev/null 2>&1; then
    task_compiler=(javac)
else
    task_compiler=(java --module jdk.compiler/com.sun.tools.javac.Main)
fi
task_java="$task_root/android/app/src/main/java/com/cleverraven/cataclysmdda"
"${task_compiler[@]}" -d "$task_build" \
    "$task_java/ReaderFileTransaction.java" "$task_java/ReaderGameSession.java" \
    "$task_java/ReaderBackupArchive.java" \
    "$task_root/tools/accessibility/java/com/cleverraven/cataclysmdda/ReaderStorageTest.java"
java -cp "$task_build" com.cleverraven.cataclysmdda.ReaderStorageTest
